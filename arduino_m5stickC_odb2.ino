//twingo const char ISO_15765_11_BIT_500_KBAUD = '6';

//image converter : https://lang-ship.com/tools/image2data/


#include "picture.h"
#include <M5StickCPlus.h>  //lcd 135x240
//#include <M5Stack.h>
//#include "Free_Fonts.h"
#include "BluetoothSerial.h"
#include "ELMduino.h"
//#include "M5Display.h"

//load SD card for M5STACK
#include "FS.h"
#include "SPIFFS.h"

BluetoothSerial    SerialBT;
#define ELM_PORT   SerialBT
#define DEBUG_PORT Serial

//define button
#define BUTTON_A 37
#define BUTTON_B 39

//led
#define RED_LED 10

//timing
#define PERIOD_WAIT_LED_FLASH 20
#define PERIOD_WAIT_LED_OFF   1000

#define PERIOD_SELECT_CAR   5000


ELM327 myELM327;

uint32_t rpm = 0;
char CONVERTED_RPM[32];
char CONVERTED_KPH[32];
char CONVERTED_ENGINETEMP[32];
char CONVERTED_INTAKETEMP[32];

int LOOP_COUNTER = 0;

float temp = 0;

float tempRPM = 0;
int32_t kph = 0;
uint32_t enginetemp = 0;
uint32_t intaketemp = 0;

int int_button_state = 0;
int int_choice_car = 1;  //twingo default
int int_choice_car_old = -10;

int int_choice_screen = 1;
int int_choice_screen_old = -10;

int int_obd_not_connected = 0;

const char* twingo          = "";
const char* twingo_pwd      = "";

const char* duster          = "";
const char* duster_pwd      = "";

unsigned long ulong_time_now       = 0;
unsigned long ulong_wait_time_led_flash  = 0;
unsigned long ulong_wait_time_car_select  = 0;

bool bool_spiffs_error = false;

//functions
void led_blink(int led_active);
void screen_1_view(void);


void setup() {
  // put your setup code here, to run once:
  M5.begin();

  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 15);
  //  M5.Lcd.println(" - OBD2 START -");

  //button led IO
  pinMode(BUTTON_A, INPUT);
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(RED_LED, OUTPUT);

  digitalWrite(RED_LED, LOW);   // turn the LED on (HIGH is the voltage level)
  delay(20);
  digitalWrite(RED_LED, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(200);
  digitalWrite(RED_LED, LOW);   // turn the LED on (HIGH is the voltage level)
  delay(20);
  digitalWrite(RED_LED, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(200);

  DEBUG_PORT.begin(115200);
  DEBUG_PORT.println(" == Start Up ==");
  //  M5.Lcd.fillScreen(BLACK);
  //  M5.Lcd.setTextSize(2);
  //  M5.Lcd.setCursor(10, 15);
  //  M5.Lcd.println(" --- OBD diag ---");
  //  M5.Lcd.setCursor(10, 45);
  //  M5.Lcd.println(" -- choice car --");
  //  delay(50);


  //test memoire interne
  //---------------------
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    bool_spiffs_error = true;
    return;
  }
  else
  {
    Serial.println("SPIFFS Mount Success");
    bool_spiffs_error = false;
  }

  //IMAGES
  //drawJpgFile(SPIFFS, "/obdii.jpg", 0, 0);
  M5.Lcd.pushImage(0, 0, 240, 135, obdii_img);

  delay(2000);


  //CAR SELECTION

  //period with millis()
  ulong_time_now = millis();
  ulong_wait_time_car_select = millis() + PERIOD_SELECT_CAR;

  while ((ulong_time_now <= ulong_wait_time_car_select))
  {
    //period with millis()
    ulong_time_now = millis();

    int_button_state = digitalRead(BUTTON_A);
    delay(10);

    if (int_button_state == 0)
    {
      Serial.println(int_button_state);
      int_choice_car++;
      int_choice_car = int_choice_car % 2;
    }

    if (int_choice_car_old != int_choice_car)
    {
      int_choice_car_old = int_choice_car;

      if (int_choice_car == 0)
      {
        //        M5.Lcd.fillScreen(BLACK);
        //        M5.Lcd.setTextSize(4);
        //        M5.Lcd.setCursor(10, 15);
        //        M5.Lcd.println(" OBDII ");
        //        M5.Lcd.setCursor(10, 75);
        //        M5.Lcd.println(" DUSTER ");
        //        M5.Lcd.setTextSize(2);

        //M5.Lcd.drawJpgFile(SPIFFS, "/duster.jpg", 0, 0);
        M5.Lcd.pushImage(0, 0, 240, 135, duster_img);
      }

      if (int_choice_car == 1)
      {
        //        M5.Lcd.fillScreen(BLACK);
        //        M5.Lcd.setTextSize(4);
        //        M5.Lcd.setCursor(10, 15);
        //        M5.Lcd.println(" OBDII ");
        //        M5.Lcd.setCursor(10, 75);
        //        M5.Lcd.println(" TWINGO ");
        //        M5.Lcd.setTextSize(2);

        //M5.Lcd.drawJpgFile(SPIFFS, "/twingo.jpg", 0, 0);
        M5.Lcd.pushImage(0, 0, 240, 135, twingo_img);

      }
      delay(1000);
    }
  }

  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE, WHITE);
  if (int_choice_car == 0) M5.Lcd.setCursor(124, 15);
  if (int_choice_car == 1) M5.Lcd.setCursor(10, 15);
  M5.Lcd.println(" Connect ");
  M5.Lcd.setTextSize(2);

  ELM_PORT.begin("M5BLE", true);

  //duster
  if (int_choice_car == 0)
  {
    ELM_PORT.setPin(duster_pwd);       // ELM327 BLE 핀 넘버
    if (!ELM_PORT.connect(duster)) // ELM327 BLE 이름
    {
      //M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setTextSize(2);
      M5.Lcd.pushImage(0, 0, 240, 135, duster_img);
      if (int_choice_car == 0) M5.Lcd.setCursor(124, 15);
      if (int_choice_car == 1) M5.Lcd.setCursor(10, 15);
      M5.Lcd.println(" Fail cnx");
      delay(500);
      //while (1);
    }
  }

  //twingo
  if (int_choice_car == 1)
  {
    ELM_PORT.setPin(twingo_pwd);       // ELM327 BLE 핀 넘버
    if (!ELM_PORT.connect(twingo)) // ELM327 BLE 이름
    {
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setTextSize(2);
      M5.Lcd.pushImage(0, 0, 240, 135, twingo_img);

      if (int_choice_car == 0) M5.Lcd.setCursor(124, 15);
      if (int_choice_car == 1) M5.Lcd.setCursor(10, 15);
      M5.Lcd.println(" Fail cnx");
      delay(500);
      //while (1);
    }
  }

  if (!myELM327.begin(ELM_PORT))
  {

    if (int_choice_car == 0)
    {
      M5.Lcd.pushImage(0, 0, 240, 135, duster_img);
      M5.Lcd.setCursor(124, 15);
    }
    if (int_choice_car == 1)
    {
      M5.Lcd.pushImage(0, 0, 240, 135, twingo_img);
      M5.Lcd.setCursor(10, 15);
    }
    M5.Lcd.println(" Ecu Fail");

    delay(1000);
    //while (1);
  }

  Serial.println("Connected to ELM327");
  delay(10);
  //M5.Lcd.fillScreen(BLACK);

}


void printError()
{
  DEBUG_PORT.print("Received: ");
  for (byte i = 0; i < myELM327.recBytes; i++)
    DEBUG_PORT.write(myELM327.payload[i]);
  DEBUG_PORT.println();

  if (myELM327.status == ELM_SUCCESS)
    DEBUG_PORT.println(F("\tELM_SUCCESS"));
  else if (myELM327.status == ELM_NO_RESPONSE)
    DEBUG_PORT.println(F("\tERROR: ELM_NO_RESPONSE"));
  else if (myELM327.status == ELM_BUFFER_OVERFLOW)
    DEBUG_PORT.println(F("\tERROR: ELM_BUFFER_OVERFLOW"));
  else if (myELM327.status == ELM_UNABLE_TO_CONNECT)
    DEBUG_PORT.println(F("\tERROR: ELM_UNABLE_TO_CONNECT"));
  else if (myELM327.status == ELM_NO_DATA)
    DEBUG_PORT.println(F("\tERROR: ELM_NO_DATA"));
  else if (myELM327.status == ELM_STOPPED)
    DEBUG_PORT.println(F("\tERROR: ELM_STOPPED"));
  else if (myELM327.status == ELM_TIMEOUT)
    DEBUG_PORT.println(F("\tERROR: ELM_TIMEOUT"));
  else if (myELM327.status == ELM_TIMEOUT)
    DEBUG_PORT.println(F("\tERROR: ELM_GENERAL_ERROR"));

  delay(100);
}

int temp_min = 30;
int temp_max = 120;

void loop() {

  //period with millis()
  ulong_time_now = millis();

  tempRPM = myELM327.rpm();
  rpm = (uint32_t)tempRPM;
  kph = myELM327.kph();
  enginetemp = myELM327.engineCoolantTemp(); // ELMduino 라이브러리에 사용자 기능 포함
  intaketemp = myELM327.intakeAirTemp();     // ELMduino 라이브러리에 사용자 기능 포함

  //  rpm = random(600, 3000);
  //  enginetemp = random(-30, 120);
  //  intaketemp = random(-30, 50);
  //
  //  rpm = rpm + 100;
  //  rpm = rpm % 3000;
  //
  //  enginetemp = enginetemp + 1;
  //  enginetemp = enginetemp % (temp_max + temp_min);
  //
  //  intaketemp = random(-30, 50);

  if (myELM327.status == ELM_SUCCESS)
    //if (true)
  {

    //alert
    if (enginetemp >= 89)
    {
      //led_blink(1);
      digitalWrite(RED_LED,  LOW);
    }
    else
    {
      digitalWrite(RED_LED, HIGH);
    }

    if (int_obd_not_connected == 1)
    {
      M5.Lcd.fillScreen(BLACK);
    }

    int_obd_not_connected = 0;

    int_button_state = digitalRead(BUTTON_A);
    delay(10);

    if (int_button_state == 0)
    {
      Serial.println(int_button_state);
      int_choice_screen++;
      int_choice_screen = int_choice_screen % 2;
    }

    if (int_choice_screen_old != int_choice_screen)
    {
      int_choice_screen_old = int_choice_screen;
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setTextColor(WHITE);
      if (int_choice_screen == 0)    screen_1_view();
      if (int_choice_screen == 1)    screen_2_view();
      M5.Lcd.setTextColor(WHITE, WHITE);
      delay(500);
    }

    M5.Lcd.setTextColor(WHITE, BLACK);
    if (int_choice_screen == 0)    screen_1_view();
    if (int_choice_screen == 1)    screen_2_view();
    M5.Lcd.setTextColor(WHITE, WHITE);

  }
  else
  {

    if (int_obd_not_connected == 0)
    {
      int_obd_not_connected = 1;
      //      M5.Lcd.fillScreen(BLACK);
      //      M5.Lcd.setTextColor(RED);
      //      M5.Lcd.setTextSize(4);
      //      M5.Lcd.setCursor(10, 20);
      //      M5.Lcd.println("   OBD   ");
      //
      //      M5.Lcd.setCursor(10, 75);
      //      M5.Lcd.println("no detect");
      //      M5.Lcd.setTextSize(2);
      //      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.pushImage(0, 0, 240, 135, obdii_nc);
    }
    printError();
  }

  delay(50);

}



void led_blink(int led_active)
{
  //wait time
  if (led_active == 1)
  {
    if ((ulong_time_now >= ulong_wait_time_led_flash))
    {
      ulong_wait_time_led_flash = millis() + PERIOD_WAIT_LED_OFF;
    }

    if (ulong_time_now < (ulong_wait_time_led_flash - PERIOD_WAIT_LED_OFF + PERIOD_WAIT_LED_FLASH))
    {
      digitalWrite(RED_LED, HIGH);   // turn the LED on (HIGH is the voltage level)
    }
    else
    {
      digitalWrite(RED_LED, LOW);   // turn the LED off (LOW is the voltage level)
    }
  }
}



void screen_1_view(void)
{
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setCursor(10, 15);
  M5.Lcd.fillRect(0, 40, 240, 2 , 0x00);
  M5.Lcd.printf("RPM");
  M5.Lcd.setCursor(90, 15);
  M5.Lcd.printf("%d       ", rpm );
  //M5.Lcd.progressBar(int x, int y, int w, int h, uint8_t val);
  M5.Lcd.fillRect(0, 40, (uint8_t)(240 * rpm / 3000), 2 , 0x51d);

  M5.Lcd.setCursor(10, 50);
  M5.Lcd.fillRect(0, 80, 240, 2 , 0x00);
  M5.Lcd.printf("IN ");
  M5.Lcd.setCursor(90, 50);
  M5.Lcd.printf("%d       ", intaketemp );
  if ((intaketemp + temp_min) < 0)
    M5.Lcd.fillRect(0, 80, (uint8_t)(240 * (temp_min + intaketemp) / (temp_min + temp_max)), 2 , 0x51d);
  else
    M5.Lcd.fillRect(0, 80, (uint8_t)(240 * (temp_min + intaketemp) / (temp_min + temp_max)), 2 , 0xe8e4);

  if (enginetemp >= 89)
  {
    //led_blink(1);
    M5.Lcd.setTextColor(RED, BLACK);
  }
  else
  {
    M5.Lcd.setTextColor(WHITE, BLACK);
  }

  M5.Lcd.setCursor(10, 90);
  M5.Lcd.fillRect(0, 120, 240, 2 , 0x00);
  M5.Lcd.printf("ENG");
  M5.Lcd.setCursor(90, 90);

  M5.Lcd.printf("%d       ", enginetemp );
  if ((enginetemp + temp_min) < 0)
    M5.Lcd.fillRect(0, 120, (uint8_t)(240 * (temp_min + enginetemp) / (temp_min + temp_max)), 2 , 0x51d);
  else
    M5.Lcd.fillRect(0, 120, (uint8_t)(240 * (temp_min + enginetemp) / (temp_min + temp_max)), 2 , 0xe8e4);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setTextSize(2);
}



void screen_2_view(void)
{
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setCursor(10, 90);
  M5.Lcd.fillRect(0, 120, 240, 2 , 0x00);
  M5.Lcd.printf("ENG");
  M5.Lcd.setTextSize(6);
  M5.Lcd.setCursor(90, 40);
  if (enginetemp >= 89)
  {
    //led_blink(1);
    M5.Lcd.setTextColor(RED, BLACK);
  }
  else
  {
    M5.Lcd.setTextColor(WHITE, BLACK);
  }
  M5.Lcd.printf("%d ", enginetemp );

  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(10, 90);
  M5.Lcd.printf("ENG");

  if ((enginetemp + temp_min) < 0)
    M5.Lcd.fillRect(0, 120, (uint8_t)(240 * (temp_min + enginetemp) / (temp_min + temp_max)), 2 , 0x51d);
  else
    M5.Lcd.fillRect(0, 120, (uint8_t)(240 * (temp_min + enginetemp) / (temp_min + temp_max)), 2 , 0xe8e4);

  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setTextSize(2);
}



/* TO DO
    led rouge flash si temp> 90°
    choix duster twingo au demarrage
*/
