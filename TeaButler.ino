#include <U8g2lib.h>
#include <SoftwareSerial.h>
#include <CheapStepper.h>

//Status
#define MENU      0
#define MOVEDOWN  1
#define INPROCESS 2
#define CANCELLED 3
#define DONE      4

//OLED
#define TIMER     0
#define CUP       1
#define BELL      2
#define NONE      9

//MP3
#define WELCOME   0
#define STARTED   1
#define SORRY     2
#define READY     3
#define JEOPARDY  4
#define MACKIE    9

//PINs
const int switchPin   = 2;  //stop sensor
const int buttonPin   = 3;  //button
const int mp3PinRX    = 5;  //MP3 RX
const int mp3PinTX    = 6;  //MP3 TX
const int stepperPin1 = 8;  //Stepper
const int stepperPin2 = 9;  //Stepper
const int stepperPin3 = 10; //Stepper
const int stepperPin4 = 11; //Stepper
const int potPin      = A0; //Poti

//OLED
U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);

//MP3
SoftwareSerial mp3(mp3PinRX, mp3PinTX);
const String welcomeMessage = ("TeaButler");
const String doneMessage = ("Done!");
const int defaultVolume = 20;

//Stepper
CheapStepper stepper (stepperPin1, stepperPin2, stepperPin3, stepperPin4);
bool moveDown = true;
bool moveUp = false;
const int maxSteps = 11000;
//const int stepperHighPosition = 150;
//const int stepperLowPosition = 70;
//const int stepperSpeedDelay = 20; // decrease this value to increase the servo speed

unsigned long steepingTime;
unsigned long startTime;
long timeLeft;
unsigned long actSteps;

volatile int state;

void setup() {
  Serial.begin(9600);

  pinMode(switchPin, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(switchPin), switchISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonISR, CHANGE);

  //OLED
  u8g2.begin();
  printOled(NONE, welcomeMessage, "" );

  //MP3
  mp3.begin(9600);
  initSound(defaultVolume);
  //playSound(WELCOME);

  //Stepper
  stepper.setRpm(12);

  //Status
  state = MENU;

}

void loop() {

  switch (state) {

    case MENU:
      playSound(WELCOME, defaultVolume);
      while (state == MENU) {
        //steepingTime = 30000 * map(analogRead(potPin), 0, 1023, 1, 20);
        steepingTime = 15000 * map(analogRead(potPin), 0, 1023, 1, 40);
        printOled(TIMER, millisToTime(steepingTime), "");
        delay(200);
      }
      break;

    case MOVEDOWN:
      playSound(STARTED, defaultVolume);
      printOled(NONE, "Press button", "to stop");
      stepper.newMove(moveDown, maxSteps);
      actSteps = maxSteps;
      while ( stepper.getStepsLeft() > 0) {
        stepper.run();
      }
      state = INPROCESS;
      break;

    case INPROCESS:
      playSound(JEOPARDY, 15);
      startTime = millis();

      while (state == INPROCESS) {

        timeLeft = steepingTime - (millis() - startTime);

        if (timeLeft > 0) {
          printOled(NONE, millisToTime(timeLeft), "");
        }
        else state = DONE;

        delay(500);
      }
      break;

    case CANCELLED:

      playSound(SORRY, defaultVolume);
      printOled(NONE, "Infusion", "cancelled");
  
      stepper.newMove(moveUp, actSteps);
      while ( stepper.getStepsLeft() != 0) {
        stepper.run();
      }
      state = MENU;
      break;

    case DONE:
      playSound(READY, defaultVolume);
      printOled(BELL, doneMessage, "");

      stepper.newMove(moveUp, actSteps);
      while ( stepper.getStepsLeft() != 0) {
        stepper.run();
      }

      while (state == DONE);
      break;
  }

}   // loop()

void switchISR() {

  static unsigned long lastInterruptTime = 0; //used to debounce button input
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > 500) {
    Serial.println("stop");
  }
  lastInterruptTime = interruptTime;

}

void buttonISR() {

  static unsigned long lastInterruptTime = 0; //used to debounce button input
  unsigned long interruptTime = millis();
  int stepsLeft;

  if (interruptTime - lastInterruptTime > 500) { //long debounce time to allow long presses

    switch (state) {

      case MENU:
        state = MOVEDOWN;
        break;

      case MOVEDOWN:
        actSteps = maxSteps - stepper.getStepsLeft();
        stepper.stop();
        state = INPROCESS;
        break;

      case INPROCESS:
        state = CANCELLED;
        break;

      case DONE:
        state = MENU;
        break;
    }
  }
  lastInterruptTime = interruptTime;

}

String millisToTime(long milliseconds) {

  unsigned long minutes = (milliseconds / 1000) / 60;
  unsigned long seconds = (milliseconds / 1000) % 60;

  String minutesString = String(minutes);
  String secondsString = String(seconds);

  if (minutes < 10) minutesString = "0" + minutesString;

  if (seconds < 10) secondsString = "0" + secondsString;

  return minutesString + ":" + secondsString;
}

void drawSymbol(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol)
{

  switch (symbol)
  {
    case TIMER:
      u8g2.setFont(u8g2_font_open_iconic_app_6x_t);
      u8g2.drawGlyph(x, y, 72);
      break;
    case CUP:
      u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
      u8g2.drawGlyph(x, y, 65);
      break;
    case BELL:
      u8g2.setFont(u8g2_font_open_iconic_embedded_6x_t);
      u8g2.drawGlyph(x, y, 65);
      break;
  }
}

void printOled(uint8_t symbol, const String& text1, const String& text2) {
  int posX = 0;
  int posY1;
  int posY2;

  if (text2 == "") {
    posY1 = 44;
  } else {
    posY1 = 30;
    posY2 = 60;
  }

  u8g2.firstPage();
  do {
    if (symbol != NONE) {
      drawSymbol(0, 48, symbol);
      posX = 42;
    }
    if (text2 == ""){
      u8g2.setFont(u8g2_font_logisoso24_tf);
    } else {
      u8g2.setFont(u8g2_font_logisoso16_tf);
    }
    u8g2.setCursor(posX, posY1);
    u8g2.print(text1);
    u8g2.setCursor(posX, posY2);
    u8g2.print(text2);
  } while ( u8g2.nextPage() );
}

void initSound(int16_t volume) {

  int8_t Send_buf[5] = {0};

  Send_buf[0] = 0x7e; //starting byte
  Send_buf[1] = 0x03; //the number of bytes of the command without starting byte and ending byte
  Send_buf[2] = 0x35; //Select source
  Send_buf[3] = 0x01; //SD Card
  Send_buf[4] = 0xef;

  for (uint8_t i = 0; i < 5; i++) //
  {
    mp3.write(Send_buf[i]) ;
  }

  delay(100);

  setVolume(volume);

}

void setVolume(int8_t volume) {

  int8_t Send_buf[5] = {0};

  Send_buf[0] = 0x7e; //starting byte
  Send_buf[1] = 0x03; //the number of bytes of the command without starting byte and ending byte
  Send_buf[2] = 0x31; //Set volume
  Send_buf[3] = (int8_t)(volume); //between 0 and 30
  Send_buf[4] = 0xef;

  for (uint8_t i = 0; i < 5; i++) //
  {
    mp3.write(Send_buf[i]) ;
  }
  delay(100);
  
}

void playSound(int16_t sound, int8_t volume) {

  static int8_t Send_buf[6] = {0};

  Send_buf[0] = 0x7e; //starting byte
  Send_buf[1] = 0x04; //the number of bytes of the command without starting byte and ending byte
  Send_buf[2] = 0x42; //Play with folder and filename
  Send_buf[3] = 0x01; //Folder 01
  Send_buf[4] = (int8_t)(sound);
  Send_buf[5] = 0xef;

  for (uint8_t i = 0; i < 6; i++) //
  {
    mp3.write(Send_buf[i]) ;
  }
  delay(100);

  setVolume(volume);
  
}
