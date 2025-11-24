#define DEBUG
//#define DEBUG_WIFI
#define AP_SSID "iotB"
#define AP_PASS "iotB1234"
#define SERVER_NAME "10.10.14.64"
#define SERVER_PORT 5000
#define LOGID "KGH_ARD"
#define PASSWD "PASSWD"

#define WIFITX 7  //7:TX -->ESP8266 RX
#define WIFIRX 6  //6:RX-->ESP8266 TX

#define CMD_SIZE 60
#define ARR_CNT 5

#include "WiFiEsp.h"
#include "SoftwareSerial.h"
#include <TimerOne.h>
#include <LiquidCrystal_I2C.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// 센서 핀
const int GasPin = A0;
const int SoundPin = A1;
const int FirePin = A2;
const int RelayPin = 5;

LiquidCrystal_I2C lcd(0x27, 16, 2);
char sendBuf[CMD_SIZE];
bool timerIsrFlag = false;

unsigned long secCount;
int sensorTime;
bool fanActive = false;
unsigned long fanOnTime = 0; 
unsigned long lastSend = 0;
const unsigned long FAN_ON_MS = 10000;

const int SOUND_ALERT = 900;
unsigned long lastAlert = 0;
const unsigned long ALERT_COOLDOWN_MS = 5000; // 5초 간격으로만 알림

SoftwareSerial wifiSerial(WIFIRX, WIFITX);
WiFiEspClient client;

void wifi_Setup();
void wifi_Init();
int  server_Connect();
void printWifiStatus();
void updateLCD(int gas, int sound, int fire);
void socketEvent();
void timerIsr();

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);              //DEBUG

  pinMode(RelayPin, OUTPUT); 
  digitalWrite(RelayPin, LOW);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0); 
  lcd.print("System Ready"); 

  wifi_Setup();
  Timer1.initialize(1000000);         //1000000uS ==> 1Sec
  Timer1.attachInterrupt(timerIsr);  // timerIsr to run every 1 seconds
}

void loop() {
  int gas = analogRead(GasPin);
  int sound = analogRead(SoundPin);
  int fire = analogRead(FirePin);
  
  if (sound >= SOUND_ALERT) {
    if (!(secCount % 5)){
      char alert[96];
      int n = snprintf(alert, sizeof(alert),
                  "[KGH_SMP] Noise detected in room 1, db=%d\r\n", sound);   // <-- 콜론(:) 대신 = , 그리고 \r\n              
      client.write((const uint8_t*)alert, n);
      Serial.print("ALERT -> "); 
      Serial.print(alert);  // 디버그
      lastAlert = millis();
    }
  }

  // put your main code here, to run repeatedly:
  if (client.available()) {
      socketEvent();
  }

  if (!fanActive && (gas > 500 || fire < 100)) 
  { 
    digitalWrite(RelayPin, HIGH);
    fanActive = true; 
    Serial.println("FAN ON (조건충족)"); 

    char alert[96];
    int n = snprintf(alert, sizeof(alert), 
                "[KGH_SMP] GAS/FIRE detected in room 1, GAS=%d, FIRE=%d\r\n", gas, fire);                 
    client.write((const uint8_t*)alert, n);

    Serial.print("ALERT -> "); 
    Serial.print(alert);  // 디버그
  } 

  if (fanActive) {   
    if (!(secCount % 5)) {
      digitalWrite(RelayPin, LOW);

      fanActive = false; 
      Serial.println("FAN OFF (자동, 5s)");
    }
  }
  
  if (timerIsrFlag) {   //1초에 한번씩 실행
    timerIsrFlag = false;
    if (!(secCount % 5)) { 
      updateLCD(gas, sound, fire);
    }

    if (!(secCount % 5)) {  //5초 한번씩 실행, 와이파이 재접속
      if (!client.connected()) {
        server_Connect();
      }
    }

    if (!(secCount % 5)) { // SQL에 보내기
      char msg[80];
      int len = snprintf(msg, sizeof(msg),
                        "[KGH_SQL]SENSOR@ROOM@%d@GAS@%d@FIRE@%d@SOUND@%d\n",
                        1, gas, fire, sound);  // 순서: ROOM NUMBER, GAS, FIRE, SOUND
      client.write((const uint8_t*)msg, len);
      Serial.print("TX: ");
      Serial.print(len);
      Serial.print(" bytes -> ");
      Serial.println(msg);  // 확인용(원하면 지워도 됨)
    }
  }
}
void socketEvent() {
  int i = 0;
  char* pToken;
  char* pArray[ARR_CNT] = { 0 };
  char recvBuf[CMD_SIZE] = { 0 };
  int len;

  len = client.readBytesUntil('\n', recvBuf, CMD_SIZE);
  //  recvBuf[len] = '\0';
  client.flush();
#ifdef DEBUG
  Serial.print("recv : ");
  Serial.println(recvBuf);
#endif
  pToken = strtok(recvBuf, "[@]");
  while (pToken != NULL) {
    pArray[i] = pToken;
    if (++i >= ARR_CNT)
      break;
    pToken = strtok(NULL, "[@]");
  }
  if (!strncmp(pArray[1], " New connected", 4))  // New Connected
  {
    Serial.write('\n');
    return;
  } else if (!strncmp(pArray[1], " Alr", 4))  //Already logged
  {
    Serial.write('\n');
    client.stop();
    server_Connect();
    return;
  } 
  else if (!strcmp(pArray[1], "ROOM")) {
    if (!strcmp(pArray[3], "NOISE")) {
      int room = atoi(pArray[2]);          // ★ pArray[2]가 방 번호
      lcd.clear();
      lcd.setCursor(0,0); lcd.print("Room "); lcd.print(room); lcd.print(":");
      lcd.setCursor(0,1); lcd.print("Noise, Bequiet !");
    }
    return;
  }


  client.write(sendBuf, strlen(sendBuf));
  client.flush();

#ifdef DEBUG
  Serial.print(", send : ");
  Serial.print(sendBuf);
#endif
}
void timerIsr() {
  //  digitalWrite(LED_BUILTIN_PIN,!digitalRead(LED_BUILTIN_PIN));
  timerIsrFlag = true;
  secCount++;
}
void wifi_Setup() {
  wifiSerial.begin(38400);
  wifi_Init();
  server_Connect();
}
void wifi_Init() {
  do {
    WiFi.init(&wifiSerial);
    if (WiFi.status() == WL_NO_SHIELD) {
#ifdef DEBUG_WIFI
      Serial.println("WiFi shield not present");
#endif
    } else
      break;
  } while (1);

#ifdef DEBUG_WIFI
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(AP_SSID);
#endif
  while (WiFi.begin(AP_SSID, AP_PASS) != WL_CONNECTED) {
#ifdef DEBUG_WIFI
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(AP_SSID);
#endif
  }
#ifdef DEBUG_WIFI
  Serial.println("You're connected to the network");
  printWifiStatus();
#endif
}
int server_Connect() {
#ifdef DEBUG_WIFI
  Serial.println("Starting connection to server...");
#endif

  if (client.connect(SERVER_NAME, SERVER_PORT)) {
#ifdef DEBUG_WIFI
    Serial.println("Connected to server");
#endif
    client.print("[" LOGID ":" PASSWD "]");
  } else {
#ifdef DEBUG_WIFI
    Serial.println("server connection failure");
#endif
  }
}
void printWifiStatus() {
  // print the SSID of the network you're attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void updateLCD(int gas, int sound, int fire) {
  static unsigned long last = 0;
  if (millis() - last < 300) return;  // 300ms마다 갱신
  last = millis();

  char line1[17], line2[17];
  snprintf(line1, sizeof(line1), "G:%4d S:%4d", gas, sound);
  snprintf(line2, sizeof(line2), "F:%4d",        fire);

  lcd.setCursor(0,0); lcd.print(line1);
  for (int i=strlen(line1); i<16; ++i) lcd.print(' ');  // 잔여칸 지우기
  lcd.setCursor(0,1); lcd.print(line2);
  for (int i=strlen(line2); i<16; ++i) lcd.print(' ');
}