/* 
   具有 Telegram 通知功能的 ESP8266 NodeMCU 閘狀態監視器
  完整的專案詳細資訊請造訪 https://RandomNerdTutorials.com/esp8266-nodemcu-door-status-telegram/
*/

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// 設定 LED 和磁簧開關的 GPIO
const int reedSwitch = 4;
const int led = 2; // 可選

// 偵測門的狀態是否改變
bool changeState = false;

// 儲存磁簧開關的狀態 (1=開啟, 0=關閉)
bool state;
String doorState;

// 輔助變數 (只會偵測相隔 1500 毫秒的狀態變化)
unsigned long previousMillis = 0; 
const long interval = 1500;

const char* ssid = "HomeAssistant1";
const char* password = "25693927";

// 初始化 Telegram BOT
#define BOTtoken "你的 Bot Token (從 Botfather 取得)"
// 使用 @myidbot 找出個人或群組的聊天 ID
#define CHAT_ID "911750774"

X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// 當磁簧開關狀態改變時執行
ICACHE_RAM_ATTR void changeDoorStatus() {
  Serial.println("狀態已改變");
  changeState = true;
}

void setup() {
  // 用於除錯的序列埠
  Serial.begin(115200);
  configTime(0, 0, "pool.ntp.org"); // 透過 NTP 取得 UTC 時間
  client.setTrustAnchors(&cert); // 加入 api.telegram.org 的根憑證
  
  // 讀取目前的門狀態
  pinMode(reedSwitch, INPUT_PULLUP);
  state = digitalRead(reedSwitch);

  // 設定 LED 狀態以符合門狀態
  pinMode(led, OUTPUT);
  digitalWrite(led, state);
  
  // 設定磁簧開關為中斷，指定中斷函式並設定 CHANGE 模式
  attachInterrupt(digitalPinToInterrupt(reedSwitch), changeDoorStatus, CHANGE);

  // 連接 Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Wi-Fi 連線成功");  

  bot.sendMessage(CHAT_ID, "通知已啟動", "");
}

void loop() {
  if (changeState) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      // 如果狀態改變，反轉目前的門狀態   
      state = !state;
      if (state) {
        doorState = "關閉";
      } else {
        doorState = "開啟";
      }
      digitalWrite(led, state);
      changeState = false;
      Serial.println(state);
      Serial.println(doorState);
      
      // 發送通知
      bot.sendMessage(CHAT_ID, "門的狀態是：" + doorState, "");
    }  
  }
}
