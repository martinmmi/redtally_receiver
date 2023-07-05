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
#include <bitmaps.h>
#include <rom/rtc.h>

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
String version = "R0.04";               // Frimeware Version
String numb = "aa";
String rx_adr, tx_adr, incoming, outgoing, rssi;
String reg_rssi;

String oledInit;
String loraInit;

char buf_version[5];
char buf_localAddress[5];
char buf_mode[4];
char buf_bV[5];
char buf_bL[4];
char buf_oledInit[12];
char buf_stripInit[12];
char buf_loraInit[12];
char buf_rssi[4];

///////////////////////////////////////////////
/////////// Setup Receiver Values /////////////
///////////////////////////////////////////////

byte localAddress = 0xdd;                 // Address of this device                   
byte destination = 0xaa;                  // Destination to send to                      

///////////////////////////////////////////////
///////////////////////////////////////////////
///////////////////////////////////////////////

byte msgKey1 = 0x2a;                        // Key of outgoing messages
byte msgKey2 = 0x56;
byte resFlag;                               // 0x00 -> OFF  0x01 -> ON
byte esmFlag;                               // 0x00 -> OFF  0x01 -> ON
byte transmissionPower;
byte receiverMode = 0x00;
byte receiverState = 0x00;
byte receiverColor = 0x00;
byte msgCount = 0; 

byte receiverRSSI;
byte receiverBL;

unsigned long lastOfferTime = 0;                   // Last send time
unsigned long lastAcknowledgeTime = 0;
unsigned long lastControlTime = 0;
unsigned long lastClockTime = 0;
unsigned long lastInitSuccess = 0;
unsigned long lastGetBattery = 0;
unsigned long lastExpiredControlTime = 0;
unsigned long lastDisplayPrint = 0;
unsigned long lastDiscoverTime = 0;    // Last send time

int expiredControlTime = 720000;      // 12 minutes waiting for control signal, then turn offline
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
int loraSpreadingFactor = 8;            //6-12 default  7     if higher = need more transmitttime, more current, more distance, only one message per secound
double loraSignalBandwidth = 250E3;     //7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, 250E3, and 500E3 default 125E3   if higher = faster in the transmission
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

uint32_t nocolor = strip.Color(0, 0, 0);
uint32_t red = strip.Color(255, 0, 0);
uint32_t green = strip.Color(0, 90, 0);
uint32_t amber = strip.Color(255, 50, 0);
uint32_t white = strip.Color(255, 255, 255);

uint32_t default_color = amber;

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

void sendMessage() {
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgKey1);                  // add message KEY
  LoRa.write(msgKey2);                  // add message KEY
  LoRa.write(receiverRSSI);
  LoRa.write(receiverBL);
  LoRa.write(receiverMode);             
  LoRa.write(receiverState);            
  LoRa.write(receiverColor);            
  LoRa.write(msgCount);                 // add message ID
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID

  Serial.print("LORA TxD:");
  Serial.print(" DST: "); Serial.print(destination);
  Serial.print(" SOURCE: "); Serial.print(localAddress);
  Serial.print(" MSGKEY1: "); Serial.print(msgKey1);
  Serial.print(" MSGKEY2: "); Serial.print(msgKey2);
  Serial.print(" RRSSI: "); Serial.print(receiverRSSI);
  Serial.print(" RBL: "); Serial.print(receiverBL);
  Serial.print(" RMODE: "); Serial.print(receiverMode);
  Serial.print(" RSTATE: "); Serial.print(receiverState);
  Serial.print(" RCOLORE: "); Serial.print(receiverColor);
  Serial.print(" MSGCOUNT: "); Serial.println(msgCount);

}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void onReceive(int packetSize, String *ptr_rx_adr, String *ptr_tx_adr, byte *ptr_res, byte *ptr_esm, byte *ptr_txpower, byte *ptr_mode, byte *ptr_state, byte *ptr_color, String *ptr_rssi) {
  if (packetSize == 0) return;          // if there's no packet, return

  //Clear the variables
  *ptr_rx_adr = "";
  *ptr_tx_adr = "";
  *ptr_rssi = "";
  *ptr_mode = 0x00;
  *ptr_state = 0x00;
  *ptr_color = 0x00;

  rx_adr = "";
  tx_adr = "";
  rssi = "";
  *ptr_mode = 0x00;
  *ptr_state = 0x00;
  *ptr_color = 0x00;

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgKey1 = LoRa.read();   // incoming msg KEY1
  byte incomingMsgKey2 = LoRa.read();   // incoming msg KEY2
  byte incomingRes = LoRa.read();       // incoming reset
  byte incomingEsm = LoRa.read();       // incoming energie save mode
  byte incomingTxPower = LoRa.read();   // incoming txpower
  byte incomingMode = LoRa.read();      
  byte incomingState = LoRa.read();     
  byte incomingColor = LoRa.read();     
  byte incomingMsgId = LoRa.read();     // incoming msg ID

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

  *ptr_rx_adr = String(recipient, HEX);
  *ptr_tx_adr = String(sender, HEX);
  *ptr_res = incomingRes;
  *ptr_esm = incomingEsm;
  *ptr_txpower = (incomingTxPower);
  *ptr_mode = (incomingMode);
  *ptr_state = (incomingState);
  *ptr_color = (incomingColor);
  *ptr_rssi = String(LoRa.packetRssi());

  Serial.print("LORA RxD:");
  Serial.print(" SOURCE: "); Serial.print(sender);
  Serial.print(" RECIPIENT: "); Serial.print(recipient);
  Serial.print(" MSGKEY1: "); Serial.print(incomingMsgKey1);
  Serial.print(" MSGKEY2: "); Serial.print(incomingMsgKey2);
  Serial.print(" RESFLAG: "); Serial.print(incomingRes);
  Serial.print(" ESMFLAG: "); Serial.print(incomingEsm);
  Serial.print(" TXPOWER: "); Serial.print(incomingTxPower);
  Serial.print(" RMODE: "); Serial.print(incomingMode);
  Serial.print(" RSTATE: "); Serial.print(incomingState);
  Serial.print(" RCOLOR: "); Serial.print(incomingColor);
  Serial.print(" MSGCOUNT: "); Serial.println(incomingMsgId);

  return;

}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void clearValues() {
  rx_adr = "";
  tx_adr = "";
  rssi = "";
  receiverMode = 0x00;
  receiverState = 0x00;
  receiverColor = 0x00;
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
  receiverRSSI = buf_rssi_int;                   //Transmit Rssi back to transmitter

if ((millis() - lastGetBattery > 10000) || (initBattery == LOW)) {
    bV = BL.getBatteryVolts();
    bL = BL.getBatteryChargeLevel();
    snprintf(buf_bV, 5, "%f", bV);
    snprintf(buf_bL, 4, "%d", bL);
    receiverBL = bL;
    initBattery = HIGH;
    lastGetBattery = millis();
  }

  u8g2.clearBuffer();					      // clear the internal memory

  //Battery Level Indicator
  u8g2.setFont(u8g2_font_6x13_tf);
  u8g2.setDisplayRotation(U8G2_R0);
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
  if (((connected == HIGH) || (connectedInit == HIGH)) && (buf_rssi_int <= -80) && ((receiverMode == 0x01) || (receiverMode = 0x05))) {
    u8g2.drawXBM(posXssi, posYssi, signalWidth, signalHeight, signal1);
  }
  if (((connected == HIGH) || (connectedInit == HIGH)) && (buf_rssi_int <= -60 ) && (buf_rssi_int >= -79) && ((receiverMode == 0x01) || (receiverMode = 0x05))) {
    u8g2.drawXBM(posXssi, posYssi, signalWidth, signalHeight, signal2);
  }
  if (((connected == HIGH) || (connectedInit == HIGH)) && (buf_rssi_int <= -40 ) && (buf_rssi_int >= -59) && ((receiverMode == 0x01) || (receiverMode = 0x05))) {
    u8g2.drawXBM(posXssi, posYssi, signalWidth, signalHeight, signal3);
  }
  if (((connected == HIGH) || (connectedInit == HIGH)) && (buf_rssi_int <= -20 ) && (buf_rssi_int >= -39) && ((receiverMode == 0x01) || (receiverMode = 0x05))) {
    u8g2.drawXBM(posXssi, posYssi, signalWidth, signalHeight, signal4);
  }
  if (((connected == HIGH) || (connectedInit == HIGH)) && (buf_rssi_int >= -19) && ((receiverMode == 0x01) || (receiverMode = 0x05))) {
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

uint32_t chooseColor(byte color) {
  if (color == 0x00) {return nocolor;}
  if (color == 0x01) {return red;}
  if (color == 0x02) {return green;}
  if (color == 0x03) {return amber;}
  return nocolor;
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
  if (localAddress == 0xbb) {
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
  if (localAddress == 0xcc) {
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
  if (localAddress == 0xdd) {
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
  if (localAddress == 0xee) {
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

  //1=POWERON_RESET   3=SW_RESET  4=OWDT_RESET    5=DEEPSLEEP_RESET   6=SDIO_RESET    7=TG0WDT_SYS_RESET  8=TG1WDT_SYS_RESET  9=RTCWDT_SYS_RESET  
  //10=INTRUSION_RESET  11=TGWDT_CPU_RESET  12=SW_CPU_RESET 13=RTCWDT_CPU_RESET   14=EXT_CPU_RESET    15=RTCWDT_BROWN_OUT_RESET   16=RTCWDT_RTC_RESET     default=NO_MEAN

  Serial.print("CPU0 reset reason: ");
  Serial.println(rtc_get_reset_reason(0));

  Serial.print("CPU1 reset reason: ");
  Serial.println(rtc_get_reset_reason(1));

  if (rtc_get_reset_reason(0) == 1 || rtc_get_reset_reason(0) == 14 || rtc_get_reset_reason(1) == 1 || rtc_get_reset_reason(1) == 14) {
          eeprom.begin("configuration", false); 
          eeprom.clear();           //Clear the eeprom when the reset button is pushed
          eeprom.end();
  }

//////////////////////////////////////////////////////////////////////

  eeprom.begin("configuration", false); 
  loraTxPower = eeprom.getInt("txpower", loraTxPower); 
  Serial.print("loraTxPower: ");
  Serial.println(loraTxPower);
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
    onReceive(LoRa.parsePacket(), &rx_adr, &tx_adr, &resFlag, &esmFlag, &transmissionPower, &receiverMode, &receiverState, &receiverColor, &rssi);    // Parse Packets and Read it

    tallyBlinkSlow(default_color);

    //Routine for discover new receivers at the start
    if ((receiverMode == 0x01) && (tx_adr == "aa") && (rx_adr == "ff")) {
      lastOfferTime = millis();
      mode = "offer";
      mode_s = "off";
      reg_rssi = rssi;
      printDisplay();
      clearValues();
      break;
    }
    
    //Routine for rediscover receivers, which lost signal, empty battery or switched off
    if ((receiverMode == 0x05) && (tx_adr == "aa")) {
      mode = "control";
      mode_s = "con";
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
    tallyBlinkFast(default_color);
    if (millis() - lastOfferTime > waitOffer) {  
      printDisplay();
      //digitalWrite(LED_PIN_INTERNAL, HIGH);
      destination = 0xaa;
      receiverMode = 0x02;                         
      sendMessage();                                    // Send a message      
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
    onReceive(LoRa.parsePacket(), &rx_adr, &tx_adr, &resFlag, &esmFlag, &transmissionPower, &receiverMode, &receiverState, &receiverColor, &rssi);    // Parse Packets and Read it

    if ((receiverMode == 0x03) && (rx_adr == "bb") && (receiverState == 0x01) && (connected == HIGH)) {
      tally(chooseColor(receiverColor));
      if (receiverColor == 0x01){relai(HIGH);}
      if (receiverColor == 0x02){relai(LOW);}
      if (receiverColor == 0x03){relai(LOW);}
      mode = "acknowledge";
      mode_s = "ack";
      lastAcknowledgeTime = millis();
      clearValues();
      break;
    }
    if ((receiverMode == 0x03) && (rx_adr == "bb") && (receiverState == 0x00) && (connected == HIGH)) {
      tally(chooseColor(receiverColor));
      relai(LOW);
      mode = "acknowledge";
      mode_s = "ack";
      lastAcknowledgeTime = millis();
      clearValues();
      break;
    }

    if ((receiverMode == 0x03) && (rx_adr == "cc") && (receiverState == 0x01) && (connected == HIGH)) {
      tally(chooseColor(receiverColor));
      if (receiverColor == 0x01){relai(HIGH);}
      if (receiverColor == 0x02){relai(LOW);}
      if (receiverColor == 0x03){relai(LOW);}
      mode = "acknowledge";
      mode_s = "ack";
      lastAcknowledgeTime = millis();
      clearValues();
      break;
    }
    if ((receiverMode == 0x03) && (rx_adr == "cc") && (receiverState == 0x00) && (connected == HIGH)) {
      tally(chooseColor(receiverColor));
      relai(LOW);
      mode = "acknowledge";
      mode_s = "ack";
      lastAcknowledgeTime = millis();
      clearValues();
      break;
    }

    if ((receiverMode == 0x03) && (rx_adr == "dd") && (receiverState == 0x01) && (connected == HIGH)) {
      tally(chooseColor(receiverColor));
      if (receiverColor == 0x01){relai(HIGH);}
      if (receiverColor == 0x02){relai(LOW);}
      if (receiverColor == 0x03){relai(LOW);}
      mode = "acknowledge";
      mode_s = "ack";
      lastAcknowledgeTime = millis();
      clearValues();
      break;
    }
    if ((receiverMode == 0x03) && (rx_adr == "dd") && (receiverState == 0x00) && (connected == HIGH)) {
      tally(chooseColor(receiverColor));
      relai(LOW);
      mode = "acknowledge";
      mode_s = "ack";
      lastAcknowledgeTime = millis();
      clearValues();
      break;
    }

    if ((receiverMode == 0x03) && (rx_adr == "ee") && (receiverState == 0x01) && (connected == HIGH)) {
      tally(chooseColor(receiverColor));
      if (receiverColor == 0x01){relai(HIGH);}
      if (receiverColor == 0x02){relai(LOW);}
      if (receiverColor == 0x03){relai(LOW);}
      mode = "acknowledge";
      mode_s = "ack";
      lastAcknowledgeTime = millis();
      clearValues();
      break;
    }
    if ((receiverMode == 0x03) && (rx_adr == "ee") && (receiverState == 0x00) && (connected == HIGH)) {
      tally(chooseColor(receiverColor));
      relai(LOW);
      mode = "acknowledge";
      mode_s = "ack";
      lastAcknowledgeTime = millis();
      clearValues();
      break;
    }
    
    //Routine for Control Online Status
    if ((receiverMode == 0x05) && (tx_adr == "aa")) {
      mode = "control";
      mode_s = "con";
      reg_rssi = rssi;  
      expiredControlTime = expiredControlTimeSync;  // After the first con Message, the time for offline status is reduced
      lastControlTime = millis();
      printDisplay();
      clearValues();
      break;
    }

    //Routine for Base Reset after 10 Minutes On-Time
    if (((receiverMode == 0x01)) && (tx_adr == "aa") && (rx_adr == "ff") && (initSuccess == HIGH) && (millis() - lastDiscoverTime > 600000)) {
      mode = "discover";
      mode_s = "dis";
      initSuccess = LOW;
      initSuccess2 = LOW;
      relai(LOW);
      tally(chooseColor(receiverColor));
      lastDiscoverTime = millis();
      lastExpiredControlTime = millis();
      expired = LOW;
      printDisplay();
      clearValues();
      break;
    }

    // Only one time Access after Init
    if (initSuccess == LOW) {
      tally(default_color);
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
      tally(default_color);
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
    receiverMode = 0x04;             
    sendMessage();                                    // Send a message      
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
    receiverMode = 0x05;   
    sendMessage();                                    // Send a message     
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
    
    if (transmissionPower == 0x00){loraTxPowerNew = 0;}
    if (transmissionPower == 0x01){loraTxPowerNew = 1;}
    if (transmissionPower == 0x02){loraTxPowerNew = 2;}
    if (transmissionPower == 0x03){loraTxPowerNew = 3;}
    if (transmissionPower == 0x04){loraTxPowerNew = 4;}
    if (transmissionPower == 0x05){loraTxPowerNew = 5;}
    if (transmissionPower == 0x06){loraTxPowerNew = 6;}
    if (transmissionPower == 0x07){loraTxPowerNew = 7;}
    if (transmissionPower == 0x08){loraTxPowerNew = 8;}
    if (transmissionPower == 0x09){loraTxPowerNew = 9;}
    if (transmissionPower == 0x0a){loraTxPowerNew = 10;}
    if (transmissionPower == 0x0b){loraTxPowerNew = 11;}
    if (transmissionPower == 0x0c){loraTxPowerNew = 12;}
    if (transmissionPower == 0x0d){loraTxPowerNew = 13;}
    if (transmissionPower == 0x0e){loraTxPowerNew = 14;}
    if (transmissionPower == 0x0f){loraTxPowerNew = 15;}
    if (transmissionPower == 0x10){loraTxPowerNew = 16;}
    if (transmissionPower == 0x11){loraTxPowerNew = 17;}
    if (transmissionPower == 0x12){loraTxPowerNew = 18;}
    if (transmissionPower == 0x13){loraTxPowerNew = 19;}
    if (transmissionPower == 0x14){loraTxPowerNew = 20;}
    
    // Methode for Reset Lora Values
    if (loraTxPower != loraTxPowerNew) {
      loraTxPower = loraTxPowerNew;
      eeprom.begin("configuration", false);                //false mean use read/write mode
      eeprom.putInt("txpower", loraTxPower);     
      eeprom.end();
      Serial.print("NewloraTxPower: ");
      Serial.println(loraTxPower);
      Serial.println("Reset: Change values.");
      u8g2.setDrawColor(1);
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.drawStr(0, 30, "RESET!");
      u8g2.sendBuffer();
      delay(5000);
      ESP.restart();
    }

    // Methode for Restart 
    if (resFlag == 0x01) {
      Serial.println("Restart: In progress.");
      u8g2.setDrawColor(1);
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.drawStr(0, 30, "RESTART!");
      u8g2.sendBuffer();
      delay(5000);
      ESP.restart();
    }

    //Methode for Sleepp
    if (esmFlag == 0x01) {
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