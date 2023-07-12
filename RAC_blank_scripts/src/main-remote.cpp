#ifdef REMOTE
#include <iostream>
#include <math.h>
#include <esp_now.h>
#include "esp_wifi.h"
#include <WiFi.h>
#include <batteryMonitor.h>
#include <ledUtility.h>
#include "esp_log.h"
#include "mac.h"

static const char *TAG = "MAIN";
//------------ turn on generic serial printing

#define DEBUG_PRINTS
//data that will be sent to the receiver

typedef struct {
  int16_t speedmotorLeft;
  int16_t speedmotorRight;
  int16_t packetArg1;
  int16_t packetArg2;
  int16_t packetArg3;
  int16_t leverValue;
}
packet_t;


packet_t sentData;
packet_t recData;


//---------------------------------------ESP_NOW Variables


String success;
esp_now_peer_info_t peerInfo;
// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == 0) {
    success = "Delivery Success :)";
  }
  else {
    success = "Delivery Fail :(";
  }
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&recData, incomingData, sizeof(recData));
}




//---------------------------------------HARDWARE DEPENDANT Variables
// one ifdef case per hardware to speed up modularity of the code

//RAC standard remote
const int steerPot = 7;
const int accPot = 10;
const int leverPot = 8;
//const int trimPot = 39;

const int rightBtn = 2;
const int leftBtn = 4;
const int topBtn = 5;
//const int lowSwitch = 32;
//const int topSwitch = 25;
LedUtility Led(21);

//customisable vars
int analogRes = 10;
int analogReadMax = (1 << analogRes)-1;


//variables for the sketch
int leverValue = 0;

unsigned long current_time = 0;


void setup() {
  //store_values(); // uncomment only to initialize mem
  analogReadResolution(analogRes);
  analogSetAttenuation(ADC_11db);
  pinMode(rightBtn, INPUT_PULLUP);
  pinMode(leftBtn, INPUT_PULLUP);
  pinMode(topBtn, INPUT_PULLUP);
  //pinMode(lowSwitch, INPUT_PULLUP);
  //pinMode(topSwitch, INPUT_PULLUP);
  Led.init();
  Led.setBlinks(1,150);
  delay(2000);
#ifdef DEBUG_PRINTS
  Serial.begin(115200);
  Serial.println("RAC GENERIC BOT");
#endif


  //---------------------------------------ESP NOW setup
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  memcpy(peerInfo.peer_addr, robotAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  char macStr[18];
  Serial.print("Packet from: ");
  // Copies the sender mac address to a string
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           peerInfo.peer_addr[0], peerInfo.peer_addr[1], peerInfo.peer_addr[2], peerInfo.peer_addr[3], peerInfo.peer_addr[4], peerInfo.peer_addr[5]);
  Serial.print("sending to: ");
  Serial.println(macStr);
  esp_now_register_recv_cb(OnDataRecv);
  Led.setBlinks(0);
  Led.ledOn();
}

int stable(int n)
{
  if (n > -90 && n < 90)
    return (0);
  return (n);
}

void move(int valY, int valX)
{
	int motorRight = valY, motorLeft = valY;

	if (valY >= 0)
	{
		if (valX <= 0)
			motorLeft += valX;
		else
			motorRight -= valX;
	}
	else
	{
		if (valX <= 0)
			motorLeft -= valX;
		else
			motorRight += valX;
	}
  if ((motorLeft < -400 || motorRight < -400) && (motorLeft == 0 || motorRight == 0))
  {
    if (motorLeft < -400)
      motorRight = 512;
    else
      motorLeft = 512;
  }
  sentData.speedmotorLeft = constrain(motorLeft, -512, 512);
  sentData.speedmotorRight = constrain(motorRight, -512, 512);
}


void loop() {
  //read pots values
  int strValue = stable(map(analogRead(steerPot), 0, 1023, -512, 512));
  delay(3);
  int accValue = stable(map(analogRead(accPot), 0, 1023, 512, -512));
  delay(3);
  int leverValue = stable(map(analogRead(leverPot), 0, 1023, 512, -512));
  delay(3);
  current_time = millis(); 
  bool rightValue = !digitalRead(rightBtn);
  bool leftValue = !digitalRead(leftBtn);
  bool topValue = !digitalRead(topBtn);
  
  // vvvv ----- YOUR AWESOME CODE HERE ----- vvvv //

  // Serial.print(stable(accValue));
  // Serial.print(" ");
  move(accValue, strValue);
  sentData.packetArg1 = (int)rightValue;
  sentData.packetArg2 = (int)leftValue;
  sentData.packetArg3 = (int)topValue;
  sentData.leverValue = leverValue;


  // -------------------------------------------- //
  esp_err_t result = -1;
  result = esp_now_send(robotAddress, (uint8_t *) &sentData, sizeof(sentData));
  if (result == ESP_OK) {
    //Serial.println("Sent with success");
  } else {
    //Serial.println("Error sending the data");
  }
  delay(10);
}
#endif