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

uint32_t cpu_frequency = 0;
uint32_t xtal_frequency = 0;
uint32_t apb_frequency = 0;

String mode = "discover";
String mode_s = "dis";
String name = "REDTALLY Receiver";      // Device Name
String version = "R0.01";                // Frimeware Version
String numb = "aa";
String rx_adr, tx_adr, incoming, outgoing, rssi;
String reg_incoming, reg_tx_adr, reg_rssi;

String oledInit;
String loraInit;

char buf_tx[12];
char buf_rx[12];
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
char buf_rssi[4];

///////////////////////////////////////////////
/////////// Setup Receiver Values /////////////
///////////////////////////////////////////////

byte localAddress = 0xdd;                 // Address of this device   
String string_localAddress = "dd";                                    
byte destination = 0xaa;                  // Destination to send to              
String string_destinationAddress = "aa";          

///////////////////////////////////////////////
///////////////////////////////////////////////
///////////////////////////////////////////////

byte msgKey1 = 0x2a;                      // Key of outgoing messages
byte msgKey2 = 0x56;
byte msgCount = 0;                        // Count of outgoing messages
byte byte_rssi;
byte byte_bL;
byte res;
byte esm;
byte txpower;

unsigned long lastOfferTime = 0;                   // Last send time
unsigned long lastAcknowledgeTime = 0;
unsigned long lastControlTime = 0;
unsigned long lastClockTime = 0;
unsigned long lastInitSuccess = 0;
unsigned long lastGetBattery = 0;
unsigned long lastExpiredControlTime = 0;
unsigned long lastTestTime = 0;
unsigned long lastDisplayPrint = 0;
unsigned long lastDiscoverTime = 0;    // Last send time

int expiredControlTime = 480000;      // 8 minutes waiting for control signal, then turn offline
int expiredControlTimeSync = 0;       // New Value, if the first con signal is received + 10s Transition Waiting
int timeToWakeUp = 0;
int defaultBrightnessDisplay = 255;   // value from 1 to 255
int defaultBrightnessLed = 255;       // value from 1 to 255
int waitOffer = 0;                                   
int buf_rssi_int = 0;
int bL = 0;
int posXssi = 0;
int posYssi = 0;
int posXrssi = 0;
int posYrssi = 0;
int posXnumb = 0;
int posYnumb = 0;
int posXlost = 0;
int posYlost = 0;
int loraTxPowerNew = 0;

///////////////////////////////////////////////
//////////// Setup LORA Values ////////////////
///////////////////////////////////////////////

int loraTxPower = 17;                   //2-20 default 17
int loraSpreadingFactor = 7;            //6-12 default  7
double loraSignalBandwidth = 125E3;     //7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, 250E3, and 500E3 default 125E3
int loraCodingRate = 5;                 //5-8 default 5
int loraPreambleLength = 8;             //6-65535 default 8
double loraFrequenz = 868E6;            //set Frequenz 915E6 or 868E6

///////////////////////////////////////////////
///////////////////////////////////////////////
///////////////////////////////////////////////

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
bool expired = LOW;

#define RELAI_PIN           14
#define LED_PIN             12
#define LED_COUNT            3

#define LED_PIN_INTERNAL    25        //Accumulator Function Declaration
#define ADC_PIN             35
#define CONV_FACTOR       1.85        //1.7 is fine for the right voltage
#define READS               20

#define loadWidth           50        //Bitmap Declaration
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

#define LORA_MISO           19        //Pinout Section
#define LORA_MOSI           27
#define LORA_SCLK            5
#define LORA_CS             18
#define LORA_RST            23
#define LORA_IRQ            26        //Must be a Hardware Interrupt Pin

#define DISPLAY_CLK         22
#define DISPLAY_DATA        21

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ DISPLAY_CLK, /* data=*/ DISPLAY_DATA);   // ESP32 Thing, HW I2C with pin remapping

Pangodream_18650_CL BL(ADC_PIN, CONV_FACTOR, READS);

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

uint32_t red = strip.Color(255, 0, 0);
uint32_t green = strip.Color(0, 255, 0);
uint32_t blue = strip.Color(0, 0, 255);
uint32_t yellow = strip.Color(255, 50, 0);
uint32_t nocolor = strip.Color(0, 0, 0);

Preferences eeprom;            //Initiate Flash Memory

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
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
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void sendMessage(String message) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgKey1);                  // add message KEY
  LoRa.write(msgKey2);                  // add message KEY
  LoRa.write(byte_rssi);
  LoRa.write(byte_bL);
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(message.length());         // add payload length
  LoRa.print(message);                  // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void onReceive(int packetSize, String *ptr_rx_adr, String *ptr_tx_adr, byte *ptr_res, byte *ptr_esm, byte *ptr_txpower, String *ptr_incoming, String *ptr_rssi) {
  if (packetSize == 0) return;          // if there's no packet, return

  //Clear the variables
  *ptr_rx_adr = "";
  *ptr_tx_adr = "";
  *ptr_incoming = "";
  *ptr_rssi = "";

  string_destinationAddress = "";
  rx_adr = "";
  outgoing = "";
  tx_adr = "";
  incoming = "";
  rssi = "";

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgKey1 = LoRa.read();   // incoming msg KEY1
  byte incomingMsgKey2 = LoRa.read();   // incoming msg KEY2
  byte incomingRes = LoRa.read();       // incoming reset
  byte incomingEsm = LoRa.read();       // incoming energie save mode
  byte incomingTxPower = LoRa.read();   // incoming txpower
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0xff) {
    Serial.println("Message not for me.");
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 30, "INFO: NOT FOR ME");
    u8g2.sendBuffer();
    return;                             // skip rest of function
  }

  if ((incomingMsgKey1 != msgKey1 && incomingMsgKey2 != msgKey2) && (recipient == localAddress || recipient == 0xff)) {
    Serial.println("Error: Message key false.");
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 30, "ERROR: MSG KEY");
    u8g2.sendBuffer();
    return;                             // skip rest of function
  }

  if ((incomingLength != incoming.length()) && (recipient == localAddress || recipient == 0xff)) {   // check length for error
    Serial.println("Error: Message length false.");
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 30, "ERROR: MSG LENGTH");
    u8g2.sendBuffer();
    return;                             // skip rest of function
  }

  *ptr_rx_adr = String(recipient, HEX);
  *ptr_tx_adr = String(sender, HEX);
  *ptr_res = incomingRes;
  *ptr_esm = incomingEsm;
  *ptr_txpower = (incomingTxPower);
  *ptr_incoming = incoming;
  *ptr_rssi = String(LoRa.packetRssi());

  return;

}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void clearValues() {

  string_destinationAddress = "";

  rx_adr = "";
  outgoing = "";

  tx_adr = "";
  incoming = "";

  rssi = "";

}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void printDisplay() {   // tx Transmit Message,  rx Receive Message,   txAdr Receive Address

  sprintf(buf_localAddress, "%x", localAddress);         // byte
  sprintf(buf_mode, "%s", mode_s);                       // string
  sprintf(buf_version, "%s", version);
  sprintf(buf_rssi, "%s", reg_rssi);          //Register value string rssi convert into buffer char rssi
  buf_rssi_int = atoi(buf_rssi);              //Convert char rssi in int rssi
  byte_rssi = buf_rssi_int;                   //Transmit Rssi back to transmitter

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

  //Battery Indicator
  u8g2.setFont(u8g2_font_6x13_tf);
  u8g2.setDrawColor(0);
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
    u8g2.drawStr(posXnumb, posYnumb, "aa");
    u8g2.setFont(u8g2_font_10x20_tf);
    u8g2.drawStr(posXlost, posYlost, "x");
  }

  //Signal High Indicator
  if ((connected == HIGH) && (connectedInit == HIGH)) {
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(posXrssi, posYrssi, buf_rssi);
    u8g2.drawStr(posXnumb, posYnumb, "aa");
  }

  u8g2.sendBuffer();

  lastDisplayPrint = millis();
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
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
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void intTallys() {
  if (string_localAddress == "bb") {
    waitOffer = random(500) + 1500;
    expiredControlTimeSync = 370000;   //6 Minutes + 10 Secounds Backup if signal turn off
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
    expiredControlTimeSync = 400000;   //6,5 Minutes + 10 Secounds Backup if signal turn off
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
    expiredControlTimeSync = 430000;   //7 Minutes + 10 Secounds Backup if signal turn off
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
    expiredControlTimeSync = 460000;    //7,5 Minutes + 10 Secounds Backup if signal turn off
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
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void setup() {

  setCpuFrequencyMhz(80);               // Set CPU Frequenz 240, 160, 80, 40, 20, 10 Mhz
  
  cpu_frequency = getCpuFrequencyMhz();
  xtal_frequency = getXtalFrequencyMhz();
  apb_frequency = getApbFrequency();

  Serial.begin(115200);                   // initialize serial
  while (!Serial);

//////////////////////////////////////////////////////////////////////

  eeprom.begin("configuration", false); 
  loraTxPower = eeprom.getInt("txpower", false); 
  Serial.print("loraTxPower: "); Serial.println(loraTxPower); 
  eeprom.end();

//////////////////////////////////////////////////////////////////////

  SPI.begin(LORA_SCLK, LORA_MISO, LORA_MOSI, LORA_CS);

  Serial.println("");
  Serial.println(name);
  Serial.println("Version: " + version);

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
  delay(500);

  sprintf(buf_version, "%s", version);
  u8g2.drawStr(99,60,buf_version);
  u8g2.sendBuffer();

  Serial.println("OLED init succeeded.");
  sprintf(buf_oledInit, "%s", "OLED init");
  u8g2.drawStr(0,10,buf_oledInit);
  u8g2.sendBuffer();
  delay(300);

  LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ);
  LoRa.setTxPower(loraTxPower);
  LoRa.setSpreadingFactor(loraSpreadingFactor);    
  LoRa.setSignalBandwidth(loraSignalBandwidth);
  LoRa.setCodingRate4(loraCodingRate);
  LoRa.setPreambleLength(loraPreambleLength);
  LoRa.begin(loraFrequenz);

  if (!LoRa.begin(loraFrequenz)) {                                        // initialize lora frequenz
      Serial.println("LORA init failed. Check your connections.");
      sprintf(buf_loraInit, "%s", "LORA failed");   
      u8g2.drawStr(0,20,buf_loraInit);
      u8g2.sendBuffer();
      delay(1000);
      while (true);                       // if failed, do nothing
  }
  if (LoRa.begin(loraFrequenz)) {    
    Serial.println("LORA init succeeded.");
    sprintf(buf_loraInit, "%s", "LORA init");   
    u8g2.drawStr(0,20,buf_loraInit);
    u8g2.sendBuffer();
    delay(1000);
  }

  pinMode(LED_PIN_INTERNAL, OUTPUT);
  pinMode(RELAI_PIN, OUTPUT);

  printLora(1);
  delay(2500);

  intTallys();
  printDisplay();

}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void loop() {

  // Discover Mode
  while (mode == "discover") {
    onReceive(LoRa.parsePacket(), &rx_adr, &tx_adr, &res, &esm, &txpower, &incoming, &rssi);    // Parse Packets and Read it

    tallyBlinkSlow(yellow);

    //Routine for discover new receivers at the start
    if ((incoming == "dis-anyrec?") && (tx_adr == "aa") && (rx_adr == "ff")) {
      Serial.println("LORA RxD: " + incoming);
      lastOfferTime = millis();
      mode = "offer";
      mode_s = "off";
      reg_incoming = incoming;
      reg_tx_adr = tx_adr;
      reg_rssi = rssi;
      printDisplay();
      clearValues();
      break;
    }
    
    //Routine for rediscover receivers, which lost signal, empty battery or switched off
    if ((incoming == "con-rec?") && (tx_adr == "aa")) {
      Serial.println("LORA RxD: " + incoming);
      mode = "control";
      mode_s = "con";
      reg_incoming = incoming;
      reg_tx_adr = tx_adr;
      reg_rssi = rssi;
      expiredControlTime = expiredControlTimeSync;  // After the first con Message, the time for offline status is reduced 
      lastControlTime = millis();
      printDisplay();
      clearValues();
      break;
    }

    // Function Print Display if nothing work
    if (millis() - lastDisplayPrint > 5000) {
      clearValues();
      printDisplay();
    }
  }

  // Offer Mode
  if (mode == "offer") {
    tallyBlinkFast(yellow);
    if (millis() - lastOfferTime > waitOffer) {  
      printDisplay();
      //digitalWrite(LED_PIN_INTERNAL, HIGH);
      destination = 0xaa;
      string_destinationAddress = "aa"; 
      outgoing = "off";                            
      sendMessage(outgoing);                                    // Send a message      
      Serial.println("LORA TxD: " + outgoing);
      //digitalWrite(LED_PIN_INTERNAL, LOW);
      connected = HIGH;
      connectedInit = HIGH;
      mode = "request";
      mode_s = "req";
      printDisplay();
      clearValues();
    }
  }

  // Request Mode
  while (mode == "request") {
    onReceive(LoRa.parsePacket(), &rx_adr, &tx_adr, &res, &esm, &txpower, &incoming, &rssi);    // Parse Packets and Read it

    if ((incoming == "req-high") && (rx_adr == "bb") && (connected == HIGH)) {
      Serial.println("LORA RxD: " + incoming);
      tally(red);
      relai(HIGH);
      mode = "acknowledge";
      mode_s = "ack";
      lastAcknowledgeTime = millis();
      clearValues();
      break;
    }
    if ((incoming == "req-low") && (rx_adr == "bb") && (connected == HIGH)) {
      Serial.println("LORA RxD: " + incoming);
      tally(nocolor);
      relai(LOW);
      mode = "acknowledge";
      mode_s = "ack";
      lastAcknowledgeTime = millis();
      clearValues();
      break;
    }

    if ((incoming == "req-high") && (rx_adr == "cc") && (connected == HIGH)) {
      Serial.println("LORA RxD: " + incoming);
      tally(red);
      relai(HIGH);
      mode = "acknowledge";
      mode_s = "ack";
      lastAcknowledgeTime = millis();
      clearValues();
      break;
    }
    if ((incoming == "req-low") && (rx_adr == "cc") && (connected == HIGH)) {
      Serial.println("LORA RxD: " + incoming);
      tally(nocolor);
      relai(LOW);
      mode = "acknowledge";
      mode_s = "ack";
      lastAcknowledgeTime = millis();
      clearValues();
      break;
    }

    if ((incoming == "req-high") && (rx_adr == "dd") && (connected == HIGH)) {
      Serial.println("LORA RxD: " + incoming);
      tally(red);
      relai(HIGH);
      mode = "acknowledge";
      mode_s = "ack";
      lastAcknowledgeTime = millis();
      clearValues();
      break;
    }
    if ((incoming == "req-low") && (rx_adr == "dd") && (connected == HIGH)) {
      Serial.println("LORA RxD: " + incoming);
      tally(nocolor);
      relai(LOW);
      mode = "acknowledge";
      mode_s = "ack";
      lastAcknowledgeTime = millis();
      clearValues();
      break;
    }

    if ((incoming == "req-high") && (rx_adr == "ee") && (connected == HIGH)) {
      Serial.println("LORA RxD: " + incoming);
      tally(red);
      relai(HIGH);
      mode = "acknowledge";
      mode_s = "ack";
      lastAcknowledgeTime = millis();
      clearValues();
      break;
    }
    if ((incoming == "req-low") && (rx_adr == "ee") && (connected == HIGH)) {
      Serial.println("LORA RxD: " + incoming);
      tally(nocolor);
      relai(LOW);
      mode = "acknowledge";
      mode_s = "ack";
      lastAcknowledgeTime = millis();
      clearValues();
      break;
    }
    
    //Routine for Control Online Status
    if ((incoming == "con-rec?") && (tx_adr == "aa")) {
      Serial.println("LORA RxD: " + incoming);
      mode = "control";
      mode_s = "con";
      reg_incoming = incoming;
      reg_tx_adr = tx_adr;
      reg_rssi = rssi;  
      expiredControlTime = expiredControlTimeSync;  // After the first con Message, the time for offline status is reduced
      lastControlTime = millis();
      printDisplay();
      clearValues();
      break;
    }

    //Routine for Base Reset after 10 Minutes On-Time
    if ((incoming == "dis-anyrec?") && (tx_adr == "aa") && (rx_adr == "ff") && (initSuccess == HIGH) && (millis() - lastDiscoverTime > 600000)) {
      Serial.println("LORA RxD: " + incoming);
      mode = "discover";
      mode_s = "dis";
      initSuccess = LOW;
      initSuccess2 = LOW;
      relai(LOW);
      tally(nocolor);
      lastDiscoverTime = millis();
      lastExpiredControlTime = millis();
      expired = LOW;
      printDisplay();
      clearValues();
      break;
    }

    // Only one time Access after Init
    if (initSuccess == LOW) {
      tally(yellow);
      lastDiscoverTime = millis();
      lastInitSuccess = millis();
      lastExpiredControlTime = millis();
      initSuccess = HIGH;
    }

    // Turn off led after Init
    if ((initSuccess2 == LOW) && (millis() - lastInitSuccess > 2000)) {
      tally(nocolor);
      initSuccess2 = HIGH;
    }

    // Status Sync expired
    if ((millis() - lastExpiredControlTime > expiredControlTime) && (expired == LOW)) {           
      tally(yellow);
      relai(LOW);
      connectedState = HIGH;
      connected = LOW;
      expired = HIGH;
      printDisplay();
    }

    //After Reconnet, set tally high
    if ((connected == HIGH) && (connectedState == HIGH)) {           
      tally(nocolor);
      connectedState = LOW;
      connected = HIGH;
      printDisplay();
    }

    // Function Print Display if nothing work
    if ((millis() - lastDisplayPrint > 5000)) {
      clearValues();
      printDisplay();
    }

    // Function to turn on the Display and led internal after energy saving time
    if ((millis() - lastExpiredControlTime > timeToWakeUp)) {
      u8g2.setContrast(defaultBrightnessDisplay);  
      u8g2.sendBuffer();
    }

  }

  // Acknowledge Mode
  if ((mode == "acknowledge") && (millis() - lastAcknowledgeTime > random(150) + 150)) {
    printDisplay();
    //digitalWrite(LED_PIN_INTERNAL, HIGH);
    destination = 0xaa;
    string_destinationAddress = "aa"; 
    outgoing = "ack";              
    sendMessage(outgoing);                                    // Send a message      
    Serial.println("LORA TxD: " + outgoing);
    //digitalWrite(LED_PIN_INTERNAL, LOW);
    mode = "request";
    mode_s = "req";
    clearValues();
  }

  // Control Mode
  if ((mode == "control") && (millis() - lastControlTime > random(150) + 150)) {
    printDisplay();
    //digitalWrite(LED_PIN_INTERNAL, HIGH);
    destination = 0xaa;
    string_destinationAddress = "aa"; 
    outgoing = "con";
    sendMessage(outgoing);                                    // Send a message     
    Serial.println("LORA TxD: " + outgoing);
    //digitalWrite(LED_PIN_INTERNAL, LOW);
    lastExpiredControlTime = millis();
    expired = LOW;

    if ((connectedState == HIGH) && (connected = LOW)) {
      tally(nocolor);
    }

    connectedInit = HIGH;
    connected = HIGH;
    mode = "request";
    mode_s = "req";
    clearValues();
    
    if (txpower == 0x00){loraTxPowerNew = 0;}
    if (txpower == 0x01){loraTxPowerNew = 1;}
    if (txpower == 0x02){loraTxPowerNew = 2;}
    if (txpower == 0x03){loraTxPowerNew = 3;}
    if (txpower == 0x04){loraTxPowerNew = 4;}
    if (txpower == 0x05){loraTxPowerNew = 5;}
    if (txpower == 0x06){loraTxPowerNew = 6;}
    if (txpower == 0x07){loraTxPowerNew = 7;}
    if (txpower == 0x08){loraTxPowerNew = 8;}
    if (txpower == 0x09){loraTxPowerNew = 9;}
    if (txpower == 0x0a){loraTxPowerNew = 10;}
    if (txpower == 0x0b){loraTxPowerNew = 11;}
    if (txpower == 0x0c){loraTxPowerNew = 12;}
    if (txpower == 0x0d){loraTxPowerNew = 13;}
    if (txpower == 0x0e){loraTxPowerNew = 14;}
    if (txpower == 0x0f){loraTxPowerNew = 15;}
    if (txpower == 0x10){loraTxPowerNew = 16;}
    if (txpower == 0x11){loraTxPowerNew = 17;}
    if (txpower == 0x12){loraTxPowerNew = 18;}
    if (txpower == 0x13){loraTxPowerNew = 19;}
    if (txpower == 0x14){loraTxPowerNew = 20;}
    
    // Methode for Reset Lora Values
    if (loraTxPower != loraTxPowerNew) {
      loraTxPower = loraTxPowerNew;
      eeprom.begin("configuration", false);                //false mean use read/write mode
      eeprom.putInt("txpower", loraTxPower);     
      eeprom.end();
      Serial.println("Reset: Change values.");
      u8g2.setDrawColor(1);
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.drawStr(0, 30, "RESET!");
      u8g2.sendBuffer();
      delay(5000);
      ESP.restart();
    }

    // Methode for Restart Lora Values
    if (res == 0x01) {
      Serial.println("Restart: In progress.");
      u8g2.setDrawColor(1);
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.drawStr(0, 30, "RESTART!");
      u8g2.sendBuffer();
      delay(5000);
      ESP.restart();
    }

    //Methode for Sleepp
    if (esm == 0x01) {
      Serial.println("Going to sleep.");
      u8g2.setDrawColor(1);
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.drawStr(0, 30, "SLEEP!");
      u8g2.sendBuffer();
      delay(5000);
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
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////