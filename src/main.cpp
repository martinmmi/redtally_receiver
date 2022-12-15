//////////////////////////////////////////////////////////////////////
//////////////////// TallyWAN by Martin Mittrenga ////////////////////
//////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <U8g2lib.h>
#include <LoRa.h>
#include <Adafruit_NeoPixel.h>

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
String mode = "discover";
String name = "TallyWAN";      // Device Name
String allSync = "";

char buf_tx[12];
char buf_rx[12];
char buf_sync[4];
char buf_name[12];
char buf_localAddress[4];
char buf_mode[8];
char buf_rxAdr[4];
char buf_txAdr[4];

bool clkState;
bool last_clkState;
bool initSuccess = LOW;
bool initSuccess2 = LOW;

byte msgCount = 0;            // Count of outgoing messages
byte localAddress = 0xcc;     // Address of this device              ///////////////CCCHHHAAANNNGGGEEE//////////////
String string_localAddress = "0xcc";                                 ///////////////CCCHHHAAANNNGGGEEE//////////////
char char_localAddress[8] = "0xcc";                                  ///////////////CCCHHHAAANNNGGGEEE//////////////
char char_off[8] = "off-";
byte destination = 0xaa;      // Destination to send to

long lastOfferTime = 0;       // Last send time
long lastClockTime = 0;
long lastInitSuccess = 0;
int interval = 2000;          // Interval between sends

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 22, /* data=*/ 21);   // ESP32 Thing, HW I2C with pin remapping

#define RELAI_PIN           14
#define LED_PIN             12
#define LED_PIN_INTERNAL    25
#define LED_COUNT            3

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

uint32_t red = strip.Color(255, 0, 0);
uint32_t green = strip.Color(0, 255, 0);
uint32_t blue = strip.Color(0, 0, 255);
uint32_t yellow = strip.Color(255, 50, 0);
uint32_t nocolor = strip.Color(0, 0, 0);

int defaultBrightness = 100;

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

  u8g2.clearBuffer();					      // clear the internal memory
  u8g2.setFont(u8g2_font_6x13_tf);
  u8g2.drawStr(0,10,buf_name);	    // write something to the internal memory
  u8g2.drawStr(0,22,"Adr:");
  u8g2.drawStr(30,22,"0x");
  u8g2.drawStr(42,22,buf_localAddress);
  u8g2.drawStr(62,22,buf_mode);
  u8g2.drawStr(0,34,"TxD:");
  u8g2.drawStr(30,34,buf_tx);
  u8g2.drawStr(100,34,"0x");
  u8g2.drawStr(112,34,buf_rxAdr);
  u8g2.drawStr(0,46,"RxD:");
  u8g2.drawStr(30,46,buf_rx);
  u8g2.drawStr(100,46,"0x");
  u8g2.drawStr(112,46,buf_txAdr);
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
  if (clkState == 0 && millis() - lastClockTime < 1000) {clkState = 1;
  }
    
  if (clkState == 1 && millis() - lastClockTime >= 1000) {
    if(millis() - lastClockTime <= 2000) {clkState = 0;}
  }
    
  if (last_clkState != clkState) {
    last_clkState = clkState;
    if (clkState == 1) {strip.fill(color, 0, LED_COUNT);}
    if (clkState == 0) {strip.fill(nocolor, 0, LED_COUNT);}
  }

  if (millis() - lastClockTime >= 2000){lastClockTime = millis();}
  strip.show();
}

void tallyBlinkFast(uint32_t color) {
  // CLOCK
  if (clkState == 0 && millis() - lastClockTime < 200) {clkState = 1;
  }
    
  if (clkState == 1 && millis() - lastClockTime >= 200) {
    if(millis() - lastClockTime <= 400) {clkState = 0;}
  }
    
  if (last_clkState != clkState) {
    last_clkState = clkState;
    if (clkState == 1) {strip.fill(color, 0, LED_COUNT);}
    if (clkState == 0) {strip.fill(nocolor, 0, LED_COUNT);}
  }

  if (millis() - lastClockTime >= 400){lastClockTime = millis();}
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
    printDisplay("", incoming, tx_adr);
    tallyBlinkSlow(yellow);

    if (incoming == "dis-anyrec?") {
      lastOfferTime = millis();
      mode = "offer";
    }

  }

  // Offer Mode
  if (mode == "offer") {
    tallyBlinkFast(yellow);
    if (millis() - lastOfferTime > random(3500) + 1500) {     // Between 1.5 and 5 Secounds Wait with Offer
      digitalWrite(LED_PIN_INTERNAL, HIGH);
      String message = strcat(char_off, char_localAddress);           // Add string_localAddress to string_off                   
      sendMessage(message);                                               // Send a message      
      allSync = string_localAddress;
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
    printDisplay("", incoming, tx_adr);

    if (incoming == "req-0xbb-high") {
      tally(red);
      relai(HIGH);
    }
    if (incoming == "req-0xbb-low") {
      tally(nocolor);
      relai(LOW);
    }

    if (incoming == "req-0xcc-high") {
      tally(red);
      relai(HIGH);
    }
    if (incoming == "req-0xcc-low") {
      tally(nocolor);
      relai(LOW);
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

  }

  // Acknowledge Mode
  if ((mode == "acknowledge")) {
  
  }

}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////