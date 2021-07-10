#include <Servo.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servoLeft;
Servo servoRight;
int bluetoothTx = 3; // 블루투스 Tx
int bluetoothRx = 2; // 블루투스 Rx
SoftwareSerial bluetooth(bluetoothTx, bluetoothRx);
int pcount = 0; // 자동주차를 위한 센서 count
int led_left = A2; // 좌측 LED
int led_right = A1; // 우측 LED
int sensor = A0; // 진동감지 센서
int buzzer = A3; // 부저
// -------초음파센서 선언 ----------
int f_trig = 8;
int f_echo = 9;
int l_trig = 4;
int l_echo = 5;
int r_trig = 6;
int r_echo = 7;
int b_trig = 10;
int b_echo = 11;
//----------------------------
//--------LCD 출력---------------
//수 
byte manual_su[8] = {B00100, B01010, B10001, B00000, B11111, B00100, B00100, B00100};
//동 
byte manual_dong[8] = {B11111, B10000, B11111, B00100, B11111, B01110, B10001, B01110 };
//거꾸로 수 
byte r_manual_su[8] = {B00100, B00100, B00100, B11111, B00000, B10001, B01010, B00100};
// 거꾸로 동 
byte r_manual_dong[8] = {B01110, B10001, B01110, B11111, B00100, B11111, B10000, B11111};
//자동주행
//자 
byte auto_ja[8] = { B00010, B11110, B01011, B10110, B00010, B00010, B00000, B00000};
//동 
byte auto_dong[8] = {B11111, B10000, B11111, B00100, B11111, B01110, B10001, B01110 };
//거꾸로 자 
byte r_auto_ja[8] = { B00000, B00000, B01000, B01000, B01101, B11010, B01111, B01000};
//거꾸로 동 
byte r_auto_dong[8] = {B01110, B10001, B01110, B11111, B00100, B11111, B10000, B11111};

//-------------------------------------

void standby()  // 대기 상태 LCD 출력문
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Standby mode");
}
void stopode()  // 정지 버튼 입력 시 LCD 출력문
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("!STOP!");
}
void manual_lcd() // 수동조작시 LCD 출력문
{
  lcd.clear(); lcd.setCursor(0, 0);
  lcd.write(byte(0));
  lcd.setCursor(0, 1);
  lcd.write(byte(1));
  lcd.setCursor(15, 1);
  lcd.write(byte(2));
  lcd.setCursor(15, 0);
  lcd.write(byte(3));
  lcd.setCursor(4, 0);
  lcd.print("Manual");
  lcd.setCursor(5, 1);
  lcd.print("driving");
}
void parking_lcd(int time) // 주차모드일 때 LCD 출력문
{
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("Parking");
  lcd.setCursor(5, 1);
  lcd.print("Mode");
  delay(time);
}
void autodrive_lcd() // 자율주행일 때 LCD 출력문
{
  lcd.clear();
  delay(500);
  lcd.setCursor(0, 0);
  lcd.write(byte(4));
  lcd.setCursor(0, 1);
  lcd.write(byte(5));
  lcd.setCursor(15, 1);
  lcd.write(byte(6));
  lcd.setCursor(15, 0);
  lcd.write(byte(7));
  lcd.setCursor(3, 0);
  lcd.print("Autonomous");
  lcd.setCursor(5, 1);
  lcd.print("driving");
}
void autoparking_lcd() // 자동주차시 LCD 출력문
{
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("Automatic");
  lcd.setCursor(5, 1);
  lcd.print("parking");
}
void driving(int speedLeft, int speedRight, int time) { // 서보모터 작동 함수
  servoLeft.writeMicroseconds(1500 + speedLeft);
  servoRight.writeMicroseconds(1500 - speedRight);
  delay(time);
}

float drive(int trig, int echo) // 초음파센서 작동 및 거리측정 후 측정값을 cm단위로 변경하는 함수
{
  digitalWrite(trig, LOW);
  digitalWrite(echo, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  unsigned long duration = pulseIn(echo, HIGH);
  float distance = duration / 29.0 / 2.0;
  return distance;
}

void setup() {
  lcd.begin();
  pinMode(sensor, INPUT); // 진동감지모듈
  pinMode(buzzer, OUTPUT); // 부저

  pinMode(led_left, OUTPUT); // 왼쪽 LED
  pinMode(led_right, OUTPUT); // 오른쪽 LED
  digitalWrite(led_left, LOW);
  digitalWrite(led_right, LOW);

  pinMode(f_trig, OUTPUT); // 전면 초음파 센서
  pinMode(f_echo, INPUT);
  pinMode(l_trig, OUTPUT); // 좌측 초음파 센서
  pinMode(l_echo, INPUT);
  pinMode(r_trig, OUTPUT); // 우측 초음파 센서
  pinMode(r_echo, INPUT);
  pinMode(b_trig, OUTPUT); // 후면 초음파 센서
  pinMode(b_echo, INPUT);

  bluetooth.begin(9600);  // 블루투스 통신 시작
  servoLeft.attach(13);
  servoRight.attach(12);
  driving(0, 0, 0);

  lcd.createChar(0, manual_su); //   ---
  lcd.createChar(1, manual_dong); //   ---
  lcd.createChar(2, r_manual_su);  //   ---     LCD 출력을 위한 createChar지정
  lcd.createChar(3, r_manual_dong); //  ---
  lcd.createChar(4, auto_ja);  //       ---
  lcd.createChar(5, auto_dong);  //     ---
  lcd.createChar(6, r_auto_ja);  //     ---
  lcd.createChar(7, r_auto_dong); //    ---
  standby();

  delay(1000);
}

void loop() {
  int breakout = 0;

  char cmd;
  if (bluetooth.available()) {
    cmd = (char)bluetooth.read();
    manual_lcd();

    // ---------수동 조작----------
    if (cmd == '1') {
      manual_lcd();  // 전진
      driving(200, 200, 20);
    }
    if (cmd == '2') {
      manual_lcd();  //좌회전
      driving(-200, 200, 20);
    }
    if (cmd == '3') {
      manual_lcd();  // 우회전
      driving(200, -200, 20);
    }
    if (cmd == '4') {
      manual_lcd();  // 후진
      driving(-200, -200, 20);
    }
    //--------------------------------
    //-----------자율주행---------------
    if (cmd == '5')
    {
      autodrive_lcd();
      while (true) {
        int count = 0;
        if (bluetooth.available() || breakout == 1) break;
        while (true) {
          if (bluetooth.available()) {
            breakout = 1;
            break;
          }
          delay(2);
          float f, b, l, r;
          f = drive(f_trig, f_echo); 
          b = drive(b_trig, b_echo); 
          l = drive(l_trig, l_echo);
          r = drive(r_trig, r_echo);
          if (f < 10)
          {
            driving(-200, -200, 20);
            count++;
          }
          else if (b < 10) {
            driving(200, 200, 20);
            count++;
          }
          else if (l < 10)
          {
            driving(200, -200, 20);
            break;
          }
          else if (r < 10) {
            driving(-200, 200, 20);
            break;
          }
          else driving(200, 200, 20);
          if (count > 6) {
            driving(-200, 200, 300);
            count = 0;
            break;
          }
        }
      }
    }
    //---------------------------------

    //----------자동주차----------
    if (cmd == '6')
    { delay(3000);
      while (true) { // 자동주차 오른쪽공간
        if (bluetooth.available() || breakout == 1) {
          break;
        }
        autoparking_lcd();
        delay(2);
        float f, b, l, r;
        f = drive(f_trig, f_echo);
        b = drive(b_trig, b_echo);
        l = drive(l_trig, l_echo);
        r = drive(r_trig, r_echo);
        if (pcount == 0 && r < 15) {
          pcount = 1;
          driving(50, 50, 200);
        }
        if (pcount == 1 && r > 15) {
          driving(50, 50, 200); // 전진
          pcount = 2;
        }
        if (pcount == 2 && r < 15) {
          driving(0, 0, 1000);
          while (true) {
            f = drive(f_trig, f_echo);
            b = drive(b_trig, b_echo);
            l = drive(l_trig, l_echo);
            r = drive(r_trig, r_echo);
            driving(-60, -25, 100);
            if ( b < 10 || r < 10 ) {
              break;
            }
          }
          pcount = 3;
        }
        if (pcount == 3 && b < 10)
        {
          driving(0, 0, 1000);
          while (true) {
            f = drive(f_trig, f_echo);
            b = drive(b_trig, b_echo);
            l = drive(l_trig, l_echo);
            r = drive(r_trig, r_echo);
            driving(50, -30 , 50);
            if (f < 10 ) break;
          }
          pcount = 4;
        }
        if (pcount == 4)
        {
          while (true) {
            if (bluetooth.available() || breakout == 1) break;
            delay(2);
            f = drive(f_trig, f_echo);
            b = drive(b_trig, b_echo);
            l = drive(l_trig, l_echo);
            r = drive(r_trig, r_echo);
            if ((int)f < (int)b) {
              delay(500);
              driving(-60, -50, 100);
            }
            else if ((int)f > (int)b) {
              delay(500);
              driving(50, 50, 100);
            }
            else
            {
              servoLeft.detach();
              servoRight.detach();
              parking_lcd(3000);
              while (true)
              {
                if (bluetooth.available() || breakout == 1) break;
                int data = digitalRead(sensor);
                if (data == HIGH)  noTone(buzzer);
                else if (data == LOW)
                {
                  while (true)
                  {
                    digitalWrite(led_left, HIGH);
                    digitalWrite(led_right, HIGH);
                    tone(buzzer, 3000);
                    delay(200);
                    noTone(buzzer);
                    digitalWrite(led_left, LOW);
                    digitalWrite(led_right, LOW);
                    delay(100);
                    if (bluetooth.available())
                    {
                      pcount = 0;
                      breakout = 1;
                      break;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    if (cmd == '7')
    { delay(3000);
      while (true) { // 자동주차 왼쪽공간
        if (bluetooth.available() || breakout == 1) {
          break;
        }
        autoparking_lcd();
        delay(2);
        float f, b, l, r;
        f = drive(f_trig, f_echo);
        b = drive(b_trig, b_echo);
        l = drive(l_trig, l_echo);
        r = drive(r_trig, r_echo);



        if (pcount == 0 && l < 15) {
          pcount = 1;
          driving(50, 50, 200);
        }
        if (pcount == 1 && l > 15) {
          driving(50, 50, 200); // 전진
          pcount = 2;

        }
        if (pcount == 2 && l < 15) {
          driving(0, 0, 1000);
          while (true) {
            f = drive(f_trig, f_echo);
            b = drive(b_trig, b_echo);
            l = drive(l_trig, l_echo);
            r = drive(r_trig, r_echo);
            driving(-40, -70, 100);
            if ( b < 10 || l < 10 ) {
              break;
            }
          }
          pcount = 3;
        }

        if (pcount == 3 && b < 10)
        {
          driving(0, 0, 1000);
          while (true) {
            f = drive(f_trig, f_echo);
            b = drive(b_trig, b_echo);
            l = drive(l_trig, l_echo);
            r = drive(r_trig, r_echo);
            driving(50, -40 , 50);
            if (f < 10 ) break;
          }

          pcount = 4;
        }
        if (pcount == 4)
        {
          while (true) {
            if (bluetooth.available() || breakout == 1) break;
            delay(2);
            f = drive(f_trig, f_echo);
            b = drive(b_trig, b_echo);
            l = drive(l_trig, l_echo);
            r = drive(r_trig, r_echo);
            if ((int)f < (int)b) {

              driving(-50, -50, 100);
            }
            else if ((int)f > (int)b) {

              driving(50, 60, 100);
            }
            else
            {
              servoLeft.detach();
              servoRight.detach();
              parking_lcd(3000);
              while (true)
              {
                if (bluetooth.available() || breakout == 1) break;
                int data = digitalRead(sensor);
                if (data == HIGH)  noTone(buzzer);
                else if (data == LOW)
                {
                  while (true)
                  {
                    digitalWrite(led_left, HIGH);
                    digitalWrite(led_right, HIGH);
                    tone(buzzer, 3000);
                    delay(200);
                    noTone(buzzer);
                    digitalWrite(led_left, LOW);
                    digitalWrite(led_right, LOW);
                    delay(100);
                    if (bluetooth.available())
                    {
                      pcount = 0;
                      breakout = 1;
                      break;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    if (cmd  == '8') // 정지
    {
      while (true) {
        stopode();
        driving(0, 0, 1000);
        if (bluetooth.available() ) break;
      }
    }
  }
  servoLeft.attach(13);
  servoRight.attach(12);
}
