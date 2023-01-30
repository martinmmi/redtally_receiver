//////////////////////////////////////////////////////////////////////
//////////////////// REDTALLY by Martin Mittrenga ////////////////////
//////////////////////////////////////////////////////////////////////
///////////////////////////// Receiver ///////////////////////////////
//////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <U8g2lib.h>
#include <LoRa.h>
#include <Adafruit_NeoPixel.h>
#include <Pangodream_18650_CL.h>
#include <Preferences.h>                //lib for flashstoreage
#include "bitmaps.h"


#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif

#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

const int csPin = 18;         // LoRa radio chip select
const int resetPin = 23;      // LoRa radio reset
const int irqPin = 26;        // Change for your board; must be a hardware interrupt pin

uint32_t cpu_frequency = 0;
uint32_t xtal_frequency = 0;
uint32_t apb_frequency = 0;

String mode = "discover";
String mode_s = "dis";
String name = "REDTALLY Receiver";      // Device Name
String version = "0.0a";                // Frimeware Version
String lost = "x";
String numb = "aa";
String rx_adr, tx_adr, incoming, outgoing, rssi, snr;
String reg_incoming, reg_tx_adr, reg_rssi, reg_snr;

String oledInit;
String stripInit;
String loraInit;
String outputInit;

char buf_tx[12];
char buf_rx[12];
char buf_sync[3];
char buf_version[5];
char buf_localAddress[5];
char buf_mode[4];
char buf_rxAdr[5];
char buf_txAdr[5];
char buf_bV[5];
char buf_bL[4];
char buf_oledInit[12];
char buf_stripInit[12];
char buf_loraInit[12];
char buf_outputInit[12];
char buf_lost[2];
char buf_rssi[4];
//char buf_snr[3];

///////////////////////////////////////////////
////////// CHANGE for each Receiver ///////////

byte localAddress = 0xbb;                 // Address of this device   
String string_localAddress = "bb";                                    
byte destination = 0xaa;                  // Destination to send to              
String string_destinationAddress = "aa";          

///////////////////////////////////////////////
///////////////////////////////////////////////

byte msgKey1 = 0x2a;                      // Key of outgoing messages
byte msgKey2 = 0x56;
byte msgCount = 0;                        // Count of outgoing messages
byte byte_rssi;
//byte byte_snr;
byte byte_bL;
byte esm;

long lastOfferTime = 0;                   // Last send time
long lastAcknowledgeTime = 0;
long lastControlTime = 0;
long lastClockTime = 0;
long lastInitSuccess = 0;
long lastGetBattery = 0;
long lastExpiredControlTime = 0;
long lastTestTime = 0;
long lastDisplayPrint = 0;
long lastEmpty = 0;
long lastDiscoverTime = 0;    // Last send time

int expiredControlTime = 480000;      // 8 minutes waiting for control signal, then turn offline
int expiredControlTimeSync = 0;       // New Value, if the first con signal is received + 10s Transition Waiting
int timeToWakeUp = 0;
int defaultBrightnessDisplay = 150;   // value from 1 to 255
int defaultBrightnessLed = 200;       // value from 1 to 255
int waitOffer = 0;                                   
int buf_rssi_int = 0;
//int buf_snr_int = 0;
int bL = 0;
int posXssi = 0;
int posYssi = 0;
int posXrssi = 0;
int posYrssi = 0;
int posXnumb = 0;
int posYnumb = 0;
int posXlost = 0;
int posYlost = 0;
double bV = 0;

bool clkState;
bool lastClkState;
bool initSuccess = LOW;
bool initSuccess2 = LOW;
bool connected = LOW;
bool connectedInit = LOW;
bool connectedState = LOW;
bool goOff = LOW;
bool initBattery = LOW;
bool batteryAttention = LOW;
bool batteryAttentionState = LOW;
bool ledInternalOn = HIGH;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 22, /* data=*/ 21);   // ESP32 Thing, HW I2C with pin remapping

#define RELAI_PIN           14
#define LED_PIN             12
#define LED_PIN_INTERNAL    25
#define LED_COUNT            3
#define ADC_PIN             35
#define CONV_FACTOR       1.75      //1.7 is fine for the right voltage
#define READS               20
#define loadWidth           50
#define loadHeight          50
#define logoWidth          128
#define logoHeight          64
#define loraWidth          128
#define loraHeight          64
#define batteryWidth        29
#define batteryHeight       15
#define signalWidth         21
#define signalHeight        18
#define lineWidth            2
#define lineHeight          10

Pangodream_18650_CL BL(ADC_PIN, CONV_FACTOR, READS);

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

uint32_t red = strip.Color(255, 0, 0);
uint32_t green = strip.Color(0, 255, 0);
uint32_t blue = strip.Color(0, 0, 255);
uint32_t yellow = strip.Color(255, 50, 0);
uint32_t nocolor = strip.Color(0, 0, 0);

//////////////////////////////////////////////////////////////////////

void printLogo(int color, int wait) {

  u8g2.setDrawColor(color);
  
  // logo 1
  u8g2.firstPage();
  do { u8g2.drawXBM(0, 0, logoWidth, logoHeight, logo1); }
  while (u8g2.nextPage());
  delay(wait);
  // logo 2
  u8g2.firstPage();
  do { u8g2.drawXBM(0, 0, logoWidth, logoHeight, logo2); }
  while (u8g2.nextPage());
  delay(wait);
  // logo 3
  u8g2.firstPage();
  do { u8g2.drawXBM(0, 0,logoWidth, logoHeight, logo3); }
  while (u8g2.nextPage());
  delay(wait);
  // logo 4
  u8g2.firstPage();
  do { u8g2.drawXBM(0, 0, logoWidth, logoHeight, logo4); }
  while (u8g2.nextPage());
  delay(wait);
  // logo 5
  u8g2.firstPage();
  do { u8g2.drawXBM(0, 0, logoWidth, logoHeight, logo5); }
  while (u8g2.nextPage());
  delay(wait);
  // logo 6
  u8g2.firstPage();
  do { u8g2.drawXBM(0, 0, logoWidth, logoHeight, logo6); }
  while (u8g2.nextPage());
  delay(wait);
  // logo 7
  u8g2.firstPage();
  do { u8g2.drawXBM(0, 0, logoWidth, logoHeight, logo7); }
  while (u8g2.nextPage());
  delay(wait);
  // logo 8
  u8g2.firstPage();
  do { u8g2.drawXBM(0, 0, logoWidth, logoHeight, logo8); }
  while (u8g2.nextPage());
  delay(wait);
  // logo 9
  u8g2.firstPage();
  do { u8g2.drawXBM(0, 0, logoWidth, logoHeight, logo9); }
  while (u8g2.nextPage());
  delay(wait);
  // logo 10
  u8g2.firstPage();
  do { u8g2.drawXBM(0, 0, logoWidth, logoHeight, logo10); }
  while (u8g2.nextPage());
  delay(wait);
}

void printLoad(int color, int wait, int count) {

  u8g2.setDrawColor(color);

  for (int i=0; i < count; i++) {
    // load 1
    u8g2.firstPage();
    do { u8g2.drawXBM(39, 12, loadWidth, loadHeight, load1); }
    while (u8g2.nextPage());
    delay(wait);
    // load 2
    u8g2.firstPage();
    do { u8g2.drawXBM(39, 12, loadWidth, loadHeight, load2); }
    while (u8g2.nextPage());
    delay(wait);
    // load 3
    u8g2.firstPage();
    do { u8g2.drawXBM(39, 12, loadWidth, loadHeight, load3); }
    while (u8g2.nextPage());
    delay(wait);
    // load 4
    u8g2.firstPage();
    do { u8g2.drawXBM(39, 12, loadWidth, loadHeight, load4); }
    while (u8g2.nextPage());
    delay(wait);
    // load 5
    u8g2.firstPage();
    do { u8g2.drawXBM(39, 12, loadWidth, loadHeight, load5); }
    while (u8g2.nextPage());
    delay(wait);
    // load 6
    u8g2.firstPage();
    do { u8g2.drawXBM(39, 12, loadWidth, loadHeight, load6); }
    while (u8g2.nextPage());
    delay(wait);
    // load 7
    u8g2.firstPage();
    do { u8g2.drawXBM(39, 12, loadWidth, loadHeight, load7); }
    while (u8g2.nextPage());
    delay(wait);
    // load 8
    u8g2.firstPage();
    do { u8g2.drawXBM(39, 12, loadWidth, loadHeight, load8); }
    while (u8g2.nextPage());
    delay(wait);
    }
}

void printLora(int color) {
  u8g2.setDrawColor(color);
  u8g2.firstPage();
  do { u8g2.drawXBM(0, 0, loraWidth, loraHeight, lora); }
  while (u8g2.nextPage());
}

//////////////////////////////////////////////////////////////////////

void sendMessage(String message) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgKey1);                  // add message KEY
  LoRa.write(msgKey2);                  // add message KEY
  LoRa.write(byte_rssi);
  //LoRa.write(byte_snr);
  LoRa.write(byte_bL);
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(message.length());         // add payload length
  LoRa.print(message);                  // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}

//////////////////////////////////////////////////////////////////////

void onReceive(int packetSize, String *ptr_rx_adr, String *ptr_tx_adr, byte *ptr_esm, String *ptr_incoming, String *ptr_rssi, String *ptr_snr) {
  if (packetSize == 0) return;          // if there's no packet, return

  //Clear the variables
  *ptr_rx_adr = "";
  *ptr_tx_adr = "";
  *ptr_incoming = "";
  *ptr_rssi = "";
  *ptr_snr = "";

  string_destinationAddress = "";
  rx_adr = "";
  outgoing = "";
  tx_adr = "";
  incoming = "";
  rssi = "";
  snr = "";

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgKey1 = LoRa.read();   // incoming msg KEY1
  byte incomingMsgKey2 = LoRa.read();   // incoming msg KEY2
  byte incomingEsm = LoRa.read();       // incoming energie save mode
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0xff) {
    Serial.println("This Message is not for me.");
    return;                             // skip rest of function
  }

  if ((incomingMsgKey1 != msgKey1 && incomingMsgKey2 != msgKey2) && (recipient == localAddress || recipient == 0xff)) {
    Serial.println("Error: Message key is false.");
    return;                             // skip rest of function
  }

  if ((incomingLength != incoming.length()) && (recipient == localAddress || recipient == 0xff)) {   // check length for error
    Serial.println("Error: Message length does not match length.");
    return;                             // skip rest of function
  }

  *ptr_rx_adr = String(recipient, HEX);
  *ptr_tx_adr = String(sender, HEX);
  *ptr_esm = incomingEsm;
  *ptr_incoming = incoming;
  *ptr_rssi = String(LoRa.packetRssi());
  *ptr_snr = String(LoRa.packetSnr());

  return;

}

//////////////////////////////////////////////////////////////////////

void emptyDisplay() {

  string_destinationAddress = "";

  rx_adr = "";
  outgoing = "";

  tx_adr = "";
  incoming = "";

  rssi = "";
  snr = "";

}

//////////////////////////////////////////////////////////////////////

void printDisplay() {   // tx Transmit Message,  rx Receive Message,   txAdr Receive Address

  sprintf(buf_tx, "%s", outgoing);
  sprintf(buf_rx, "%s", incoming);
  sprintf(buf_version, "%s", version);
  sprintf(buf_sync, "%s", numb);    
  sprintf(buf_localAddress, "%x", localAddress);         // byte
  sprintf(buf_mode, "%s", mode_s);                       // string
  sprintf(buf_rxAdr, "%s", string_destinationAddress);   // x = byte
  sprintf(buf_txAdr, "%s", tx_adr);
  sprintf(buf_lost, "%s", lost);
  sprintf(buf_rssi, "%s", reg_rssi);          //Register value string rssi convert into buffer char rssi
  //sprintf(buf_snr, "%s", reg_snr);            //Register value string snr convert into buffer char snr

  buf_rssi_int = atoi(buf_rssi);              //Convert char rssi in int rssi
  //buf_snr_int = atoi(buf_snr);                //Convert char snr in int snr
  
  byte_rssi = buf_rssi_int;                   //Transmit Snr and Rssi back to transmitter
  //byte_snr = buf_snr_int;


if ((millis() - lastGetBattery > 10000) || (initBattery == LOW)) {
    bV = BL.getBatteryVolts();
    bL = BL.getBatteryChargeLevel();
    snprintf(buf_bV, 5, "%f", bV);
    snprintf(buf_bL, 4, "%d", bL);

    byte_bL = bL;

    initBattery = HIGH;
    lastGetBattery = millis();
  }

  u8g2.clearBuffer();					      // clear the internal memory

  //Battery Level Indicator
  u8g2.setFont(u8g2_font_6x13_tf);
  u8g2.setDrawColor(1);
  u8g2.drawStr(67,12,buf_bL);
  u8g2.drawStr(87,12,"%");
  //u8g2.drawStr(67,25,buf_bV);       // write something to the internal memory
  //u8g2.drawStr(87,25,"V");

  //TxD and RxD Indicator
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setDrawColor(1);
  u8g2.drawStr(0,26,"TxD:");
  u8g2.drawStr(30,26,buf_tx);
  u8g2.drawStr(115,26,buf_rxAdr);
  u8g2.drawStr(0,36,"RxD:");
  u8g2.drawStr(30,36,buf_rx);
  u8g2.drawStr(115,36,buf_txAdr);

  //Address Indicator
  u8g2.setFont(u8g2_font_6x13_tf);
  u8g2.setDrawColor(1);
  u8g2.drawXBM(20, 3, lineWidth, lineHeight, line1);
  u8g2.setDrawColor(1);
  u8g2.drawStr(3,12,buf_localAddress);

  //Mode Indicator
  u8g2.setFont(u8g2_font_6x13_tf);
  u8g2.setDrawColor(1);
  u8g2.drawXBM(51, 3, lineWidth, lineHeight, line1);
  u8g2.setDrawColor(1);
  u8g2.drawStr(29,12,buf_mode);

  u8g2.setFont(u8g2_font_6x13_tf);
  u8g2.setDrawColor(0);

  //Battery Indicator
  if ((bL >= 0) && (bL <= 10)) {
    batteryAttention = HIGH;
  }
  if ((bL >= 11) && (bL <= 25)) {
    u8g2.drawXBM(99, 0, batteryWidth, batteryHeight, battery1);
    batteryAttention = LOW;
  }
  if ((bL >= 26) && (bL <= 50)) {
    u8g2.drawXBM(99, 0, batteryWidth, batteryHeight, battery2);
    batteryAttention = LOW;
    }
  if ((bL >= 51) && (bL <= 75)) {
    u8g2.drawXBM(99, 0, batteryWidth, batteryHeight, battery3);
    batteryAttention = LOW;
  }
  if ((bL >= 76) && (bL <= 100)) {
    u8g2.drawXBM(99, 0, batteryWidth, batteryHeight, battery4);
    batteryAttention = LOW;
  }

  //Battery Attention Indicator
  if ((batteryAttention == HIGH)) {
    batteryAttentionState = !batteryAttentionState;
    if ((batteryAttentionState == HIGH)) {
      u8g2.drawXBM(99, 0, batteryWidth, batteryHeight, battery0);
    }
    if ((batteryAttentionState == LOW)) {
      u8g2.drawXBM(99, 0, batteryWidth, batteryHeight, battery1);
    } 
  }

  //Signal Strength Indicator
  if (((connected == HIGH) || (connectedInit == HIGH)) && (buf_rssi_int <= -80) && ((reg_incoming == "dis-anyrec?") || (reg_incoming == "con-rec?"))) {
    u8g2.drawXBM(posXssi, posYssi, signalWidth, signalHeight, signal1);
  }
  if (((connected == HIGH) || (connectedInit == HIGH)) && (buf_rssi_int <= -60 ) && (buf_rssi_int >= -79) && ((reg_incoming == "dis-anyrec?") || (reg_incoming == "con-rec?"))) {
    u8g2.drawXBM(posXssi, posYssi, signalWidth, signalHeight, signal2);
  }
  if (((connected == HIGH) || (connectedInit == HIGH)) && (buf_rssi_int <= -40 ) && (buf_rssi_int >= -59) && ((reg_incoming == "dis-anyrec?") || (reg_incoming == "con-rec?"))) {
    u8g2.drawXBM(posXssi, posYssi, signalWidth, signalHeight, signal3);
  }
  if (((connected == HIGH) || (connectedInit == HIGH)) && (buf_rssi_int <= -20 ) && (buf_rssi_int >= -39) && ((reg_incoming == "dis-anyrec?") || (reg_incoming == "con-rec?"))) {
    u8g2.drawXBM(posXssi, posYssi, signalWidth, signalHeight, signal4);
  }
  if (((connected == HIGH) || (connectedInit == HIGH)) && (buf_rssi_int >= -19) && ((reg_incoming == "dis-anyrec?") || (reg_incoming == "con-rec?"))) {
    u8g2.drawXBM(posXssi, posYssi, signalWidth, signalHeight, signal5);
  }

  //Signal Lost Indicator
  if ((connected == LOW) && (connectedInit == HIGH)) {
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(posXrssi, posYrssi, buf_rssi);
    u8g2.drawStr(posXnumb, posYnumb, buf_sync);
    u8g2.setFont(u8g2_font_10x20_tf);
    u8g2.drawStr(posXlost, posYlost, buf_lost);
  }

  //Signal High Indicator
  if ((connected == HIGH) && (connectedInit == HIGH)) {
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(posXrssi, posYrssi, buf_rssi);
    u8g2.drawStr(posXnumb, posYnumb, buf_sync);
  }

  u8g2.sendBuffer();

  //Serial.println("");
  //Serial.print("Mode: "); Serial.println(mode);
  //Serial.print("Voltage: "); Serial.print(bV); Serial.println(" V");
  //Serial.print("Voltage Level: "); Serial.print(bL); Serial.println(" %");
  //Serial.print("TxD Adr: "); Serial.println(string_destinationAddress);
  //Serial.print("TxD: "); Serial.println(outgoing);
  //Serial.print("RxD Adr: "); Serial.println(tx_adr);
  //Serial.print("RxD: "); Serial.println(incoming);
  //Serial.print("Rssi: "); Serial.println(buf_rssi_int);
  //Serial.print("LastDiscoverTime: "); Serial.println(lastDiscoverTime);
  //Serial.print("Connected: "); Serial.println(connected);
  //Serial.print("Connected Init: "); Serial.println(connectedInit);
  //Serial.print("Connected State: "); Serial.println(connectedState);
  //Serial.print("CPU Frequency: "); Serial.print(cpu_frequency); Serial.println(" MHz");
  //Serial.print("XTAL Frequency: "); Serial.print(xtal_frequency); Serial.println(" MHz");
  //Serial.print("APB Frequency: "); Serial.print(apb_frequency); Serial.println(" Hz");

  lastDisplayPrint = millis();
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
    
  if (lastClkState != clkState) {
    lastClkState = clkState;
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
    
  if (lastClkState != clkState) {
    lastClkState = clkState;
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

void intTallys() {
  if (string_localAddress == "bb") {
    waitOffer = random(500) + 1500;
    expiredControlTimeSync = 210000;   
    timeToWakeUp = 150000;
    posXssi = 8;
    posYssi = 46;
    posXrssi = 0;
    posYrssi = 47;
    posXnumb = 6;
    posYnumb = 55;
    posXlost = 19;
    posYlost = 47;
  }
  if (string_localAddress == "cc") {
    waitOffer = random(500) + 3000;
    expiredControlTimeSync = 220000; 
    timeToWakeUp = 160000; 
    posXssi = 40;
    posYssi = 46;
    posXrssi = 32;
    posYrssi = 47;
    posXnumb = 38;
    posYnumb = 55;
    posXlost = 51;
    posYlost = 47;
  }
  if (string_localAddress == "dd") {
    waitOffer = random(500) + 4500;
    expiredControlTimeSync = 230000;
    timeToWakeUp = 170000; 
    posXssi = 72;
    posYssi = 46;
    posXrssi = 64;
    posYrssi = 47;
    posXnumb = 70;
    posYnumb = 55;
    posXlost = 83;
    posYlost = 47;
  }
  if (string_localAddress == "ee") {
    waitOffer = random(500) + 6000;
    expiredControlTimeSync = 240000;
    timeToWakeUp = 180000; 
    posXssi = 104;
    posYssi = 46;
    posXrssi = 96;
    posYrssi = 47;
    posXnumb = 102;
    posYnumb = 55;
    posXlost = 115;
    posYlost = 47;
  }
}

//////////////////////////////////////////////////////////////////////

void setup() {

  setCpuFrequencyMhz(80);               // Set CPU Frequenz 240, 160, 80, 40, 20, 10 Mhz
  
  cpu_frequency = getCpuFrequencyMhz();
  xtal_frequency = getXtalFrequencyMhz();
  apb_frequency = getApbFrequency();

  Serial.begin(115200);                   // initialize serial
  while (!Serial);

  Serial.println("");
  Serial.println(name);

  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setContrast(defaultBrightnessDisplay);                  
  //u8g2.setFlipMode(1);

  strip.begin();    
  strip.setBrightness(defaultBrightnessLed);    
  strip.show();

  //        Color, Delay, Runs
  printLogo(0, 50);
  delay(1000);

  printLoad(1, 60, 2);

  printLogo(0, 25);
  delay(500);

  printLoad(1, 60, 4);

  Serial.println("Version: " + version);
  sprintf(buf_version, "%s", version);
  u8g2.drawStr(99,60,buf_version);
  u8g2.sendBuffer();

  Serial.println("OLED init succeeded.");
  oledInit = "OLED init";
  sprintf(buf_oledInit, "%s", oledInit);
  u8g2.drawStr(0,15,buf_oledInit);
  u8g2.sendBuffer();
  delay(300);

  Serial.println("Strip init succeeded.");
  stripInit = "Strip init";
  sprintf(buf_stripInit, "%s", stripInit);
  u8g2.drawStr(0,25,buf_stripInit);
  u8g2.sendBuffer();
  delay(300);

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
    loraInit = "LoRa failed";
    sprintf(buf_loraInit, "%s", loraInit);   
    u8g2.drawStr(0,35,buf_loraInit);
    u8g2.sendBuffer();
    while (true);                       // if failed, do nothing
  }

  Serial.println("LoRa init succeeded.");
  loraInit = "LoRa init";
  sprintf(buf_loraInit, "%s", loraInit);   
  u8g2.drawStr(0,35,buf_loraInit);
  u8g2.sendBuffer();
  delay(300);

  pinMode(LED_PIN_INTERNAL, OUTPUT);
  pinMode(RELAI_PIN, OUTPUT);

  Serial.println("Outputs init succeeded.");
  outputInit = "Outputs init";
  sprintf(buf_outputInit, "%s", outputInit);   
  u8g2.drawStr(0,45,buf_outputInit);
  u8g2.sendBuffer();
  delay(500);

  printLora(1);
  delay(2500);

  intTallys();

  emptyDisplay();
  printDisplay();

}

//////////////////////////////////////////////////////////////////////

void loop() {

  // Discover Mode
  while (mode == "discover") {
    onReceive(LoRa.parsePacket(), &rx_adr, &tx_adr, &esm, &incoming, &rssi, &snr);    // Parse Packets and Read it

    tallyBlinkSlow(yellow);

    //Routine for discover new receivers at the start
    if ((incoming == "dis-anyrec?") && (tx_adr == "aa") && (rx_adr == "ff")) {
      Serial.println("RxD: " + incoming);
      lastOfferTime = millis();
      mode = "offer";
      mode_s = "off";
      reg_incoming = incoming;
      reg_tx_adr = tx_adr;
      reg_rssi = rssi;
      reg_snr = snr;
      printDisplay();
      emptyDisplay();
      break;
    }
    
    //Routine for rediscover receivers, which lost signal, empty battery or switched off
    if ((incoming == "con-rec?") && (tx_adr == "aa")) {
      Serial.println("RxD: " + incoming);
      mode = "control";
      mode_s = "con";
      reg_incoming = incoming;
      reg_tx_adr = tx_adr;
      reg_rssi = rssi;
      reg_snr = snr;
      expiredControlTime = expiredControlTimeSync;  // After the first con Message, the time for offline status is reduced 
      lastControlTime = millis();
      printDisplay();
      emptyDisplay();
      break;
    }

    // Function Print Display if nothing work
    if (millis() - lastDisplayPrint > 10000) {
      emptyDisplay();
      printDisplay();
    }
  }

  // Offer Mode
  if (mode == "offer") {
    tallyBlinkFast(yellow);
    if (millis() - lastOfferTime > waitOffer) {  
      if (ledInternalOn == HIGH) {digitalWrite(LED_PIN_INTERNAL, HIGH);}
      destination = 0xaa;
      string_destinationAddress = "aa"; 
      outgoing = "off";                            
      sendMessage(outgoing);                                    // Send a message      
      Serial.println("TxD: " + outgoing);
      digitalWrite(LED_PIN_INTERNAL, LOW);
      connected = HIGH;
      connectedInit = HIGH;
      mode = "request";
      mode_s = "req";
      printDisplay();
      emptyDisplay();
      lastEmpty = millis();
    }
  }

  // Request Mode
  while (mode == "request") {
    onReceive(LoRa.parsePacket(), &rx_adr, &tx_adr, &esm, &incoming, &rssi, &snr);    // Parse Packets and Read it

    if ((incoming == "req-high") && (rx_adr == "bb") && (connected == HIGH)) {
      Serial.println("RxD: " + incoming);
      tally(red);
      relai(HIGH);
      mode = "acknowledge";
      mode_s = "ack";
      lastAcknowledgeTime = millis();
      printDisplay();
      emptyDisplay();
      break;
    }
    if ((incoming == "req-low") && (rx_adr == "bb") && (connected == HIGH)) {
      Serial.println("RxD: " + incoming);
      tally(nocolor);
      relai(LOW);
      mode = "acknowledge";
      mode_s = "ack";
      lastAcknowledgeTime = millis();
      printDisplay();
      emptyDisplay();
      break;
    }

    if ((incoming == "req-high") && (rx_adr == "cc") && (connected == HIGH)) {
      Serial.println("RxD: " + incoming);
      tally(red);
      relai(HIGH);
      mode = "acknowledge";
      mode_s = "ack";
      lastAcknowledgeTime = millis();
      printDisplay();
      emptyDisplay();
      break;
    }
    if ((incoming == "req-low") && (rx_adr == "cc") && (connected == HIGH)) {
      Serial.println("RxD: " + incoming);
      tally(nocolor);
      relai(LOW);
      mode = "acknowledge";
      mode_s = "ack";
      lastAcknowledgeTime = millis();
      printDisplay();
      emptyDisplay();
      break;
    }

    if ((incoming == "req-high") && (rx_adr == "dd") && (connected == HIGH)) {
      Serial.println("RxD: " + incoming);
      tally(red);
      relai(HIGH);
      mode = "acknowledge";
      mode_s = "ack";
      lastAcknowledgeTime = millis();
      printDisplay();
      emptyDisplay();
      break;
    }
    if ((incoming == "req-low") && (rx_adr == "dd") && (connected == HIGH)) {
      Serial.println("RxD: " + incoming);
      tally(nocolor);
      relai(LOW);
      mode = "acknowledge";
      mode_s = "ack";
      lastAcknowledgeTime = millis();
      printDisplay();
      emptyDisplay();
      break;
    }

    if ((incoming == "req-high") && (rx_adr == "ee") && (connected == HIGH)) {
      Serial.println("RxD: " + incoming);
      tally(red);
      relai(HIGH);
      mode = "acknowledge";
      mode_s = "ack";
      lastAcknowledgeTime = millis();
      printDisplay();
      emptyDisplay();
      break;
    }
    if ((incoming == "req-low") && (rx_adr == "ee") && (connected == HIGH)) {
      Serial.println("RxD: " + incoming);
      tally(nocolor);
      relai(LOW);
      mode = "acknowledge";
      mode_s = "ack";
      lastAcknowledgeTime = millis();
      printDisplay();
      emptyDisplay();
      break;
    }
    
    //Routine for Control Online Status
    if ((incoming == "con-rec?") && (tx_adr == "aa")) {
      Serial.println("RxD: " + incoming);
      mode = "control";
      mode_s = "con";
      reg_incoming = incoming;
      reg_tx_adr = tx_adr;
      reg_rssi = rssi;  
      reg_snr = snr;
      expiredControlTime = expiredControlTimeSync;  // After the first con Message, the time for offline status is reduced
      lastControlTime = millis();
      printDisplay();
      emptyDisplay();
      break;
    }

    //Routine for Base Reset after 10 Minutes On-Time
    if ((incoming == "dis-anyrec?") && (tx_adr == "aa") && (rx_adr == "ff") && (initSuccess == HIGH) && (millis() - lastDiscoverTime > 600000)) {
      Serial.println("RxD: " + incoming);
      mode = "discover";
      mode_s = "dis";
      initSuccess = LOW;
      initSuccess2 = LOW;
      relai(LOW);
      tally(nocolor);
      lastDiscoverTime = millis();
      printDisplay();
      emptyDisplay();
      break;
    }

    if ((millis() - lastEmpty) > 2500) {
      emptyDisplay();
      printDisplay();
      lastEmpty = millis();
    }

    if (initSuccess == LOW) {
      tally(yellow);
      lastDiscoverTime = millis();
      lastInitSuccess = millis();
      lastExpiredControlTime = millis();
      initSuccess = HIGH;
    }

    if ((initSuccess2 == LOW) && (millis() - lastInitSuccess > 2000)) {
      tally(nocolor);
      initSuccess2 = HIGH;
    }

    if ((millis() - lastExpiredControlTime > expiredControlTime)) {           // Status Sync expired
      tally(yellow);
      relai(LOW);
      connectedState = HIGH;
      connected = LOW;
      printDisplay();
    }

    if ((connected == HIGH) && (connectedState == HIGH)) {           
      tally(nocolor);
      connectedState = LOW;
      connected = HIGH;
      printDisplay();
    }

    // Function Print Display if nothing work
    if ((millis() - lastDisplayPrint > 10000)) {
      emptyDisplay();
      printDisplay();
    }

    // Function to turn on the Display and led internal after energy saving time
    if ((millis() - lastExpiredControlTime > timeToWakeUp)) {
      ledInternalOn = HIGH;
      u8g2.setContrast(defaultBrightnessDisplay);  
      u8g2.sendBuffer();
    }

  }

  // Acknowledge Mode
  if ((mode == "acknowledge") && (millis() - lastAcknowledgeTime > random(150) + 150)) {
    if (ledInternalOn == HIGH) {digitalWrite(LED_PIN_INTERNAL, HIGH);}
    destination = 0xaa;
    string_destinationAddress = "aa"; 
    outgoing = "ack";              
    sendMessage(outgoing);                                    // Send a message      
    printDisplay();
    Serial.println("TxD: " + outgoing);
    digitalWrite(LED_PIN_INTERNAL, LOW);
    mode = "request";
    mode_s = "req";
    emptyDisplay();
  }

  // Control Mode
  if ((mode == "control") && (millis() - lastControlTime > random(150) + 150)) {
    if (ledInternalOn == HIGH) {digitalWrite(LED_PIN_INTERNAL, HIGH);}
    destination = 0xaa;
    string_destinationAddress = "aa"; 
    outgoing = "con";
    sendMessage(outgoing);                                    // Send a message     
    printDisplay();
    Serial.println("TxD: " + outgoing);
    digitalWrite(LED_PIN_INTERNAL, LOW);
    lastExpiredControlTime = millis();

    if ((connectedState == HIGH) && (connected = LOW)) {
      tally(nocolor);
    }

    connectedInit = HIGH;
    connected = HIGH;
    mode = "request";
    mode_s = "req";
    emptyDisplay();

    if (esm == 0x01) {
      Serial.println("SLEEP!");
      ledInternalOn = LOW;
      u8g2.setContrast(0);
      u8g2.sendBuffer();
      esp_sleep_enable_timer_wakeup(timeToWakeUp * 1000000);
      esp_light_sleep_start();
    }
  }

}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////