#ifdef ROBOT
#include <esp_now.h>
#include "esp_wifi.h"
#include <WiFi.h>
#include <motorControl.h>
#include <batteryMonitor.h>
#include <ledUtility.h>
#include "esp_log.h"
#include "mac.h"
static const char *TAG = "MAIN";

#define weapPot 7

//------------ turn on generic serial printing
#define DEBUG_PRINTS


#define MOTOR_A_IN1 8
#define MOTOR_A_IN2 18

#define MOTOR_B_IN1 16
#define MOTOR_B_IN2 17

#define MOTOR_C_IN1 4
#define MOTOR_C_IN2 5



//RIGHT
MotorControl motor1 = MotorControl(MOTOR_B_IN1, MOTOR_B_IN2); 
//LEFT
MotorControl motor2 = MotorControl(MOTOR_A_IN1, MOTOR_A_IN2);
//WPN
MotorControl motor3 = MotorControl(MOTOR_C_IN1, MOTOR_C_IN2);

BatteryMonitor Battery = BatteryMonitor();

LedUtility Led = LedUtility();

typedef struct {
  int16_t speedmotorLeft;
  int16_t speedmotorRight;
  int16_t packetArg1;
  int16_t packetArg2;
  int16_t packetArg3;
  int16_t packetArg4;
}
packet_t;
packet_t recData;


bool failsafe = false;
unsigned long failsafeMaxMillis = 400;
unsigned long lastPacketMillis = 0;

int recLpwm = 0;
int recRpwm = 0;
int recArg1 = 0;
int recArg2 = 0;
int recArg3 = 0;
int recArg4 = 0;


// Callback when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  memcpy(&recData, incomingData, sizeof(recData));
  recLpwm = recData.speedmotorLeft;
  recRpwm = recData.speedmotorRight;
  recArg1 = recData.packetArg1;
  recArg2 = recData.packetArg2;
  recArg3 = recData.packetArg3;
  recArg4 = recData.packetArg4;
  lastPacketMillis = millis();
  failsafe = false;
}

int handle_blink(){
  if(Battery.isLow()){
    Led.setBlinks(1,1000);
    return 1;
  }
  if (failsafe){
    Led.setBlinks(2,500);
    return -1;
  }
  Led.setBlinks(0);
  Led.ledOn();
  return 0;
}

void setup()
{
#ifdef DEBUG_PRINTS
  Serial.begin(115200);
  Serial.println("Ready.");
#endif
  analogReadResolution(10);
  Led.init();
  delay(20);
  Led.setBlinks(1,150);
  delay(2000);
  Battery.init();
  delay(20);


  analogSetAttenuation(ADC_11db);
  motor1.setSpeed(0);
  motor2.setSpeed(0);
  motor3.setSpeed(0);
  delay(500);

  WiFi.mode(WIFI_STA);
  if (esp_wifi_set_mac(WIFI_IF_STA, &robotAddress[0]) != ESP_OK)
  {
    Serial.println("Error changing mac");
    return;
  }
  Serial.println(WiFi.macAddress());
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
  Led.setBlinks(0);
  Led.ledOn();
}

void loop()
{
  unsigned long current_time = millis();
  int weap;

  weap = map(analogRead(weapPot), 0, 1023, 1023, 0);
  if (current_time - lastPacketMillis > failsafeMaxMillis)
  {
    failsafe = true;
  }
  handle_blink();
  if (failsafe)
  {
    motor1.setSpeed(0);
    motor2.setSpeed(0);
    motor3.setSpeed(0);
  }
  else
  {
  // vvvv ----- YOUR AWESOME CODE HERE ----- vvvv //

  // Serial.print(recData.speedmotorLeft);
  // Serial.print(" ");
  // Serial.println(recData.speedmotorRight);
  // Serial.println(map(analogRead(weapPot), 0, 1023, -512, 512));
  // Serial.println(map(recData.packetArg4, 0, 1023, -512, 512));

  // while (recData.packetArg1)
  // Serial.print(recData.packetArg1);
  // Serial.print(recData.packetArg2);
  // Serial.print(recData.packetArg3);
  // Serial.println(recData.packetArg4);
  // Serial.println(weap);
  motor2.setSpeed(recData.speedmotorLeft * -1);
  motor1.setSpeed(recData.speedmotorRight);
  if (recData.packetArg4 > 0 && weap < 1022)
    motor3.setSpeed(512);
  else if (recData.packetArg4 < 0 && weap > 260)
      motor3.setSpeed(-512);
  else
    motor3.setSpeed(0);
  // -------------------------------------------- //
  }
  delay(2);
}
#endif