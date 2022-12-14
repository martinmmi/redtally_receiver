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

const int csPin = 18;          // LoRa radio chip select
const int resetPin = 23;       // LoRa radio reset
const int irqPin = 26;         // change for your board; must be a hardware interrupt pin

String outgoing;              // outgoing message
String mode;

char buf_sm[12];
char buf_rm[12];

bool clkState;
bool last_clkState;
bool initSuccess = LOW;

byte msgCount = 0;            // count of outgoing messages
byte localAddress = 0xcc;     // address of this device                                           //TALLYXXXXXXXXXX
byte destination = 0xaa;      // destination to send to
long lastOfferTime = 0;       // last send time
long lastClockTime = 0;
long lastInitSuccess = 0;
int interval = 2000;          // interval between sends

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

  // if message is for this device, or broadcast, print details:
  /*Serial.println("Received from: 0x" + String(sender, HEX));
  Serial.println("Sent to: 0x" + String(recipient, HEX));
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println("Message length: " + String(incomingLength));
  Serial.println("Message: " + incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println();*/

  *ptr_rx_adr = String(recipient, HEX);
  *ptr_tx_adr = String(sender, HEX);
  *ptr_incoming = incoming;
  *ptr_rssi = String(LoRa.packetRssi());
  *ptr_snr = String(LoRa.packetSnr());

  return;

}

//////////////////////////////////////////////////////////////////////

void printDisplay(String rm, String sm) {

  sprintf(buf_rm, "%s", rm);
  sprintf(buf_sm, "%s", sm);

  u8g2.clearBuffer();					                    // clear the internal memory
  u8g2.setFont(u8g2_font_6x13_tf);
  u8g2.drawStr(0,10,"tallyWAN_receiver");	    // write something to the internal memory
  u8g2.drawStr(0,25,"RxD:");
  u8g2.drawStr(35,25,buf_rm);
  u8g2.drawStr(0,40,"TxD:");
  u8g2.drawStr(35,40,buf_sm);
  u8g2.sendBuffer();

}

//////////////////////////////////////////////////////////////////////


void tally(uint32_t color) {
  strip.fill(color, 0, LED_COUNT);
  strip.show();
}

void tallyBlinkSlow(uint32_t color) {
  //CLOCK
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
  //CLOCK
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

  mode = "discover";

  Serial.println("");
  Serial.println("tallyWAN_receiver");

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
  u8g2.setFont(u8g2_font_6x13_tf);	
  u8g2.drawStr(0,10,"tallyWAN_receiver");
  u8g2.drawStr(0,25,"TxD:");
  u8g2.drawStr(0,40,"RxD:");
  u8g2.sendBuffer();

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

  //Discover Mode
  while (mode == "discover") {
    String rx_adr, tx_adr, incoming, rssi, snr;
    onReceive(LoRa.parsePacket(), &rx_adr, &tx_adr, &incoming, &rssi, &snr);    //Parse Packets and Read it
    printDisplay(incoming, "");
    tallyBlinkSlow(yellow);

    if (incoming == "dis-anyrec?") {
      lastOfferTime = millis();
      mode = "offer";
    }

  }

  //Offer Mode
  if (mode == "offer") {
    tallyBlinkFast(yellow);
    if (millis() - lastOfferTime > random(1000) + 2000) {     //Between 2 and 3 Secounds Wait with Offer
      digitalWrite(LED_PIN_INTERNAL, HIGH);
      String message = "off-tally2";           //Send a message                                                   //TALLYXXXXXXXXXX
      sendMessage(message);
      printDisplay("", message);
      Serial.println("TxD: " + message);
      digitalWrite(LED_PIN_INTERNAL, LOW);
      mode = "request";
    }
  }

  //Request Mode
  while (mode == "request") {
    String rx_adr, tx_adr, incoming, rssi, snr;
    onReceive(LoRa.parsePacket(), &rx_adr, &tx_adr, &incoming, &rssi, &snr);    //Parse Packets and Read it
    printDisplay(incoming, "");

    if (incoming == "req-tally1on") {
      tally(red);
      relai(HIGH);
    }
    if (incoming == "req-tally1off") {
      tally(nocolor);
      relai(LOW);
    }

    if (incoming == "req-tally2on") {
      tally(red);
      relai(HIGH);
    }
    if (incoming == "req-tally2off") {
      tally(nocolor);
      relai(LOW);
    }

    if (initSuccess == LOW) {
      tally(yellow);
      lastInitSuccess = millis();
      initSuccess = HIGH;
    }

    if (millis() - lastInitSuccess > 6000) {
      tally(nocolor);
    }

  }

  //Acknowledge Mode
  if ((mode == "acknowledge")) {
  
  }

}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////