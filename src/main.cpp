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

int rssi;
int snr;
int rn;
int first_data_int;
int secound_data_int;

int random_number = random(0, 100);

char delim[] = "/";
char buf1[16];
char buf2[8];
char buf3[8];
char buf_rn[8];
char data[16];

char typ;
char first_adress;
char first_data;
char secound_adress;
char secound_data;

bool clk_state;
bool last_clk_state;
bool high = 1;
bool low = 0;

int defaultBrightness = 50;

unsigned long lastMillisLoop = 0;
unsigned long lastMillisLed = 0;
unsigned long lastMillisTakt_1 = 0;
unsigned long lastMillisTakt_2 = 0;

//U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ 13, /* data=*/ 11, /* reset=*/ 8);
//U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);   // All Boards without Reset of the Display
//U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ 16, /* data=*/ 17, /* reset=*/ U8X8_PIN_NONE);   // ESP32 Thing, pure SW emulated I2C
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 22, /* data=*/ 21);   // ESP32 Thing, HW I2C with pin remapping

#define RELAI_PIN           14
#define LED_PIN             12
#define LED_PIN_INTERNAL    25
#define LED_COUNT            3

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

uint32_t red = strip.Color(255, 0, 0);
uint32_t green = strip.Color(0, 255, 0);
uint32_t blue = strip.Color(0, 0, 255);
uint32_t yellow = strip.Color(255, 170, 0);
uint32_t nocolor = strip.Color(0, 0, 0);


//////////////////////////////////////////////////////////////////////


void printDisplay() {

    u8g2.clearBuffer();					// clear the internal memory
    u8g2.clear();
    u8g2.setFont(u8g2_font_6x13_tf);	// choose a suitable font
    u8g2.drawStr(0,10,"LoRa Receiver");	// write something to the internal memory
    u8g2.drawStr(0,30,buf1);
    u8g2.drawStr(0,50,"RSSI:");
    u8g2.drawStr(35,50,buf2);
    u8g2.drawStr(70,50,"SNR:");
    u8g2.drawStr(105,50,buf3);
    u8g2.drawStr(105,30,buf_rn);
    u8g2.sendBuffer();					// transfer internal memory to the display

}


//////////////////////////////////////////////////////////////////////


void tally(uint32_t color) {
  strip.fill(color, 0, LED_COUNT);
  strip.show();
}


void relai(bool state) {
  digitalWrite(RELAI_PIN, state);
}


//////////////////////////////////////////////////////////////////////


void onReceive(int packetSize) {
  if (packetSize == 0) return;          // if there's no packet, return

  String incoming = "";

  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }

  Serial.print("Received Message: '" + incoming);
  Serial.print("' with RSSI: " + String(LoRa.packetRssi()));
  Serial.print(" and SNR: " + String(LoRa.packetSnr()));
  Serial.println();

  rssi = (LoRa.packetRssi());
  snr = (LoRa.packetSnr());
  rn = random_number;

  sprintf(buf1, "%s", incoming);
  sprintf(buf2, "%d", rssi);
  sprintf(buf3, "%d", snr);
  sprintf(buf_rn, "%d", rn);
  sprintf(data, "%s", incoming);

  printDisplay();
  digitalWrite(LED_PIN_INTERNAL, HIGH);

  char *typ = strtok(data, delim);
  char *first_adress = strtok(NULL, delim);
  char *first_data = strtok(NULL, delim);
  char *secound_adress = strtok(NULL, delim);
  char *secound_data = strtok(NULL, delim);

  sscanf(first_data, "%d", &first_data_int);
  sscanf(secound_data, "%d", &secound_data_int);

  //if (test.equals(first_data))
  if (secound_data_int == random_number) {
    tally(red);
    relai(high);
  }
  else {
    tally(nocolor);
    relai(low);
  }

}


//////////////////////////////////////////////////////////////////////


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial);

  lastMillisLoop = millis();
  lastMillisLed = millis();
  lastMillisTakt_1 = millis();
  lastMillisTakt_2 = millis();

  Serial.println("LoRa Receiver");

  LoRa.setPins(csPin, resetPin, irqPin); // set CS, reset, IRQ pin

  LoRa.setTxPower(17);  //2-20 default 17
  LoRa.setSpreadingFactor(7);    //6-12 default 7
  LoRa.setSignalBandwidth(125E3);   //7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, 250E3, and 500E3 default 125E3
  LoRa.setCodingRate4(5);   //5-8 default 5
  LoRa.setPreambleLength(8);    //6-65535 default 8

  LoRa.begin(868E6);  //set Frequenz 915E6 or 868E6

  if (!LoRa.begin(868E6)) {
    Serial.println("Starting LoRa failed");
    while (1);
  }

  u8g2.begin();     //display begin

  strip.begin();    //led begin
  strip.setBrightness(defaultBrightness);    
  strip.show(); // Initialize all pixels to 'off'

  pinMode(RELAI_PIN, OUTPUT);
  pinMode(LED_PIN_INTERNAL, OUTPUT);

}


//////////////////////////////////////////////////////////////////////


void loop() {
  
  onReceive(LoRa.parsePacket());

  //MILLIS CLOCK
  if ((millis() - lastMillisLoop) >= 10000) {
    random_number = random(0, 100);
    Serial.print("CHANGE:"); Serial.println(random_number);
    lastMillisLoop = millis();
  }


  if ((millis() - lastMillisLed) >= 500) {
    digitalWrite(LED_PIN_INTERNAL, LOW);
    lastMillisLed = millis();
  }

  //CLOCK
  if (clk_state == 0 && millis() - lastMillisTakt_1 < 1000) {
    clk_state = 1;
  }
    
  if (clk_state == 1 && millis() - lastMillisTakt_1 >= 1000) {
    if(millis() - lastMillisTakt_1 <= 2000) {
    clk_state = 0;
    }
  }
    
  if (last_clk_state != clk_state) {   
    last_clk_state = clk_state;
    if (clk_state == 1) {
      //tally(red);
      //relai(clk_state);

    }
    if (clk_state == 0) {
      //tally(nocolor);
      //relai(clk_state);

    }
  }

  if (millis() - lastMillisTakt_1 >= 2000){
    lastMillisTakt_1 = millis();
  }


}


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////