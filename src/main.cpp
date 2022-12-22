//////////////////////////////////////////////////////////////////////
//////////////////// TallyWAN by Martin Mittrenga ////////////////////
//////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <U8g2lib.h>
#include <LoRa.h>
#include <Adafruit_NeoPixel.h>
#include <Pangodream_18650_CL.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif

#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

const int csPin = 18;         // LoRa radio chip select
const int resetPin = 23;      // LoRa radio reset
const int irqPin = 26;        // Change for your board; must be a hardware interrupt pin

String outgoing;              // Outgoing message
String message;
String mode = "discover";
String name = "TallyWAN";      // Device Name
String allSync = "";

char buf_tx[12];
char buf_rx[12];
char buf_sync[3];
char buf_name[9];
char buf_localAddress[5];
char buf_mode[9];
char buf_rxAdr[5];
char buf_txAdr[5];
char buf_bV[5];
char buf_bL[4];

bool clkState;
bool last_clkState;
bool initSuccess = LOW;
bool initSuccess2 = LOW;
bool initBattery = HIGH;

int expiredControlTime = 120000;
int defaultBrightness = 100;
int waitOffer = random(500) + 4000;                                ///////////////CCCHHHAAANNNGGGEEE//////////////

byte msgCount = 0;            // Count of outgoing messages
byte localAddress = 0xdd;     // Address of this device            ///////////////CCCHHHAAANNNGGGEEE//////////////
String string_localAddress = "dd";                                 ///////////////CCCHHHAAANNNGGGEEE//////////////
byte destination = 0xaa;      // Destination to send to              
String string_destinationAddress = "aa";                                 
long lastOfferTime = 0;       // Last send time
long lastAcknowledgeTime = 0;
long lastClockTime = 0;
long lastInitSuccess = 0;
long lastGetBattery = 0;
long lastExpiredControlTime = 0;
long lastTestTime = 0;
long lastDisplayPrint = 0;
long lastControlTime = 0;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 22, /* data=*/ 21);   // ESP32 Thing, HW I2C with pin remapping

#define RELAI_PIN           14
#define LED_PIN             12
#define LED_PIN_INTERNAL    25
#define LED_COUNT            3
#define ADC_PIN             35
#define CONV_FACTOR        1.8
#define READS               20

Pangodream_18650_CL BL(ADC_PIN, CONV_FACTOR, READS);

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

uint32_t red = strip.Color(255, 0, 0);
uint32_t green = strip.Color(0, 255, 0);
uint32_t blue = strip.Color(0, 0, 255);
uint32_t yellow = strip.Color(255, 50, 0);
uint32_t nocolor = strip.Color(0, 0, 0);

//////////////////////////////////////////////////////////////////////

void sendMessage(String outgoing) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}

//////////////////////////////////////////////////////////////////////

void onReceive(int packetSize, String *ptr_rx_adr, String *ptr_tx_adr, String *ptr_incoming, String *ptr_rssi, String *ptr_snr) {
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  String incoming = "";

  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }

  if (incomingLength != incoming.length()) {   // check length for error
    Serial.println("error: message length does not match length");
    return;                             // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0xff) {
    Serial.println("This message is not for me.");
    return;                             // skip rest of function
  }

  *ptr_rx_adr = String(recipient, HEX);
  *ptr_tx_adr = String(sender, HEX);
  *ptr_incoming = incoming;
  *ptr_rssi = String(LoRa.packetRssi());
  *ptr_snr = String(LoRa.packetSnr());

  return;

}

//////////////////////////////////////////////////////////////////////

void printDisplay(String tx, String rx, String txAdr) {   // tx Transmit Message,  rx Receive Message,   txAdr Receive Address

  sprintf(buf_tx, "%s", tx);
  sprintf(buf_rx, "%s", rx);
  sprintf(buf_name, "%s", name);
  sprintf(buf_sync, "%s", allSync);    
  sprintf(buf_localAddress, "%x", localAddress);    // byte
  sprintf(buf_mode, "%s", mode);                    // string
  sprintf(buf_rxAdr, "%x", destination);            // byte
  sprintf(buf_txAdr, "%s", txAdr);
  
  if ((millis() - lastGetBattery > 5000) || (initBattery == HIGH)) {
    snprintf(buf_bV, 5, "%f", BL.getBatteryVolts());
    snprintf(buf_bL, 4, "%d", BL.getBatteryChargeLevel());
    initBattery = LOW;
    lastGetBattery = millis();
  }

  u8g2.clearBuffer();					      // clear the internal memory
  u8g2.setFont(u8g2_font_6x13_tf);
  u8g2.drawStr(0,10,buf_name);	    // write something to the internal memory
  u8g2.drawStr(62,10,buf_bV);
  u8g2.drawStr(88,10,"V");
  u8g2.drawStr(100,10,buf_bL);
  u8g2.drawStr(121,10,"%");
  u8g2.drawStr(0,22,"Adr:");
  u8g2.drawStr(30,22,buf_localAddress);
  u8g2.drawStr(62,22,buf_mode);
  u8g2.drawStr(0,34,"TxD:");
  u8g2.drawStr(30,34,buf_tx);
  u8g2.drawStr(117,34,buf_rxAdr);
  u8g2.drawStr(0,46,"RxD:");
  u8g2.drawStr(30,46,buf_rx);
  u8g2.drawStr(117,46,buf_txAdr);
  u8g2.drawStr(0,58,"Syn:");
  u8g2.drawStr(30,58,buf_sync);
  u8g2.sendBuffer();

}

//////////////////////////////////////////////////////////////////////


void tally(uint32_t color) {
  strip.fill(color, 0, LED_COUNT);
  strip.show();
}

void tallyBlinkSlow(uint32_t color) {
  // CLOCK
  if (clkState == 0 && millis() - lastClockTime < 800) {clkState = 1;
  }
    
  if (clkState == 1 && millis() - lastClockTime >= 800) {
    if(millis() - lastClockTime <= 1600) {clkState = 0;}
  }
    
  if (last_clkState != clkState) {
    last_clkState = clkState;
    if (clkState == 1) {strip.fill(color, 0, LED_COUNT);}
    if (clkState == 0) {strip.fill(nocolor, 0, LED_COUNT);}
  }

  if (millis() - lastClockTime >= 1600){lastClockTime = millis();}
  strip.show();
}

void tallyBlinkFast(uint32_t color) {
  // CLOCK
  if (clkState == 0 && millis() - lastClockTime < 150) {clkState = 1;
  }
    
  if (clkState == 1 && millis() - lastClockTime >= 150) {
    if(millis() - lastClockTime <= 300) {clkState = 0;}
  }
    
  if (last_clkState != clkState) {
    last_clkState = clkState;
    if (clkState == 1) {strip.fill(color, 0, LED_COUNT);}
    if (clkState == 0) {strip.fill(nocolor, 0, LED_COUNT);}
  }

  if (millis() - lastClockTime >= 300){lastClockTime = millis();}
  strip.show();
}

void relai(bool state) {
  digitalWrite(RELAI_PIN, state);
}

//////////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(9600);                   // initialize serial
  while (!Serial);

  Serial.println("");
  Serial.println(name);

  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(csPin, resetPin, irqPin); // set CS, reset, IRQ pin
  LoRa.setTxPower(17);  //2-20 default 17
  LoRa.setSpreadingFactor(7);    //6-12 default 7
  LoRa.setSignalBandwidth(125E3);   //7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, 250E3, and 500E3 default 125E3
  LoRa.setCodingRate4(5);   //5-8 default 5
  LoRa.setPreambleLength(8);    //6-65535 default 8
  LoRa.begin(868E6);  //set Frequenz 915E6 or 868E6

  if (!LoRa.begin(868E6)) {             // initialize ratio at 868 MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing
  }

  Serial.println("LoRa init succeeded.");

  u8g2.begin();
  u8g2.clearBuffer();
  printDisplay("", "", "");

  Serial.println("OLED init succeeded.");

  strip.begin();    
  strip.setBrightness(defaultBrightness);    
  strip.show();

  Serial.println("Strip init succeeded.");

  pinMode(LED_PIN_INTERNAL, OUTPUT);
  pinMode(RELAI_PIN, OUTPUT);

}

//////////////////////////////////////////////////////////////////////

void loop() {

  // Discover Mode
  while (mode == "discover") {
    String rx_adr, tx_adr, incoming, rssi, snr;
    onReceive(LoRa.parsePacket(), &rx_adr, &tx_adr, &incoming, &rssi, &snr);    // Parse Packets and Read it
    
    if ((incoming != "") || (millis() - lastTestTime > 1500)) {
      printDisplay("", incoming, tx_adr);
      lastTestTime = millis();
    }
    if (incoming != "") {
      Serial.println("RxD: " + incoming);
      Serial.print("RxD_Adr: "); Serial.print(rx_adr); Serial.print(" TxD_Adr: "); Serial.println(tx_adr);
    }

    tallyBlinkSlow(yellow);

    if ((incoming == "dis-anyrec?") && (tx_adr == "aa")) {
      lastOfferTime = millis();
      mode = "offer";
      break;
    }

  }

  // Offer Mode
  if (mode == "offer") {
    tallyBlinkFast(yellow);
    if (millis() - lastOfferTime > waitOffer) {      
      digitalWrite(LED_PIN_INTERNAL, HIGH);
      message = "off";                            
      sendMessage(message);                                    // Send a message      
      allSync = string_destinationAddress;
      printDisplay(message, "", "");
      Serial.println("TxD: " + message);
      digitalWrite(LED_PIN_INTERNAL, LOW);
      mode = "request";
    }
  }

  // Request Mode
  while (mode == "request") {
    String rx_adr, tx_adr, incoming, rssi, snr;
    onReceive(LoRa.parsePacket(), &rx_adr, &tx_adr, &incoming, &rssi, &snr);    // Parse Packets and Read it

    if ((incoming != "") || (millis() - lastTestTime > 1500)) {
        printDisplay("", incoming, tx_adr);
        lastTestTime = millis();
      }
      if (incoming != "") {
        Serial.println("RxD: " + incoming);
        Serial.print("RxD_Adr: "); Serial.print(rx_adr); Serial.print(" TxD_Adr: "); Serial.println(tx_adr);
      }

    if ((incoming == "req-high") && (rx_adr == "bb")) {
      tally(red);
      relai(HIGH);
      mode = "acknowledge";
      lastAcknowledgeTime = millis();
      break;
    }
    if ((incoming == "req-low") && (rx_adr == "bb")) {
      tally(nocolor);
      relai(LOW);
      mode = "acknowledge";
      lastAcknowledgeTime = millis();
      break;
    }

    if ((incoming == "req-high") && (rx_adr == "cc")) {
      tally(red);
      relai(HIGH);
      mode = "acknowledge";
      lastAcknowledgeTime = millis();
      break;
    }
    if ((incoming == "req-low") && (rx_adr == "cc")) {
      tally(nocolor);
      relai(LOW);
      mode = "acknowledge";
      lastAcknowledgeTime = millis();
      break;
    }

    if ((incoming == "req-high") && (rx_adr == "dd")) {
      tally(red);
      relai(HIGH);
      mode = "acknowledge";
      lastAcknowledgeTime = millis();
      break;
    }
    if ((incoming == "req-low") && (rx_adr == "dd")) {
      tally(nocolor);
      relai(LOW);
      mode = "acknowledge";
      lastAcknowledgeTime = millis();
      break;
    }

    if ((incoming == "req-high") && (rx_adr == "ee")) {
      tally(red);
      relai(HIGH);
      mode = "acknowledge";
      lastAcknowledgeTime = millis();
      break;
    }
    if ((incoming == "req-low") && (rx_adr == "ee")) {
      tally(nocolor);
      relai(LOW);
      mode = "acknowledge";
      lastAcknowledgeTime = millis();
      break;
    }

    if ((incoming == "con-anyrec?") && (rx_adr == "ff")) {
      mode = "control";
      lastControlTime = millis();
      break;
    }

    if ((incoming == "dis-anyrec?") && (rx_adr == "ff")) {
      mode = "discover";
      break;
    }

    if (initSuccess == LOW) {
      tally(yellow);
      lastInitSuccess = millis();
      initSuccess = HIGH;
    }

    if ((initSuccess2 == LOW) && (millis() - lastInitSuccess > 2000)) {
      tally(nocolor);
      initSuccess2 = HIGH;
    }

    if ((millis() - lastExpiredControlTime > expiredControlTime)) {           // Status Sync expired
      allSync = "";
      printDisplay("", "", "");
    }

  }

  // Acknowledge Mode
  if ((mode == "acknowledge") && (millis() - lastAcknowledgeTime > random(150) + 100)) {
    digitalWrite(LED_PIN_INTERNAL, HIGH);
    message = "ack";              
    sendMessage(message);                                    // Send a message      
    printDisplay(message, "", "");
    Serial.println("TxD: " + message);
    digitalWrite(LED_PIN_INTERNAL, LOW);
    mode = "request";
  }

  // Control Mode
  if ((mode == "control") && (millis() - lastControlTime > random(150) + 100)) {
    digitalWrite(LED_PIN_INTERNAL, HIGH);
    message = "con";
    sendMessage(message);                                    // Send a message      
    printDisplay(message, "", "");
    Serial.println("TxD: " + message);
    digitalWrite(LED_PIN_INTERNAL, LOW);
    lastExpiredControlTime = millis();
    mode = "request";
  }

  // Function Print Display if nothing work
  if (millis() - lastDisplayPrint > 10000) {
    printDisplay("", "", "");
    lastDisplayPrint = millis();
  }

}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////