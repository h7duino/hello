/*
SF火警及值班人員通報
新增第二個磁簧開關的代碼範例，讓每個磁簧開關都有自己的 GPIO 腳位並可以分別偵測狀態變化。
//Sensor B接點請用3.3V來觸發..
/*線路圖
         NodeMCU        Relay1        Relay2         
           VIN                        
           GND           COM            COM
            D1            NC               
            D2                           NC   
*/

#include <ESP8266WiFi.h>          // ESP8266 Wi-Fi 函式庫
#include <WiFiManager.h>           // WiFiManager 庫
#include <WiFiClientSecure.h>      // HTTPS 客戶端函式庫
#include <PubSubClient.h>          // MQTT 函式庫
#include <time.h>                  // NTP 時間函式庫

// MQTT Broker 設定
const char* mqttServer = "60.249.3.189";
const int mqttPort = 1883;
const char* mqttUser = "admin";
const char* mqttPassword = "54h78800";

// Telegram Bot 設定
#define BOT_TOKEN "7466900614:AAHBZnNOTeZZ6XO4b7nNQlHgIdhDeSvEX_c"
#define CHAT_ID "-4514156084"  // 目標 Telegram 聊天 ID

// GPIO 腳位設定
const int reedSwitchPin1 = 4;  // 第一個磁簧開關 GPIO4 D2 火警受信總機移報B接點
const int reedSwitchPin2 = 5;  // 第二個磁簧開關 GPIO5 D1 6F人員實體按鍵移報B接點
bool doorState1 = HIGH;         // 第一個磁簧開關的狀態
bool lastDoorState1 = HIGH;     // 儲存上次的狀態
bool doorState2 = HIGH;         // 第二個磁簧開關的狀態
bool lastDoorState2 = HIGH;     // 儲存上次的狀態

// 初始化 MQTT 客戶端
WiFiClient mqttWifiClient;
PubSubClient mqttClient(mqttWifiClient);

// 函式：連接 NTP 伺服器
void connectNTP() {
  configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("正在同步 NTP 時間...");
  while (time(nullptr) < 8 * 3600) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nNTP 時間同步完成");
}

// 函式：連接 MQTT Broker
void connectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("正在連接 MQTT...");
    if (mqttClient.connect("ESP8266Client", mqttUser, mqttPassword)) {
      Serial.println("MQTT 已連接");
    } else {
      Serial.print("MQTT 連接失敗，狀態碼 = ");
      Serial.println(mqttClient.state());
      delay(5000);  // 5 秒後重試
    }
  }
}

// 函式：取得當前時間字串
String getCurrentTime() {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  char buffer[25];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
  return String(buffer);
}

// 函式：發送 Telegram 訊息
void sendTelegramMessage(String message) {
  WiFiClientSecure telegramClient;
  telegramClient.setInsecure();  // 忽略憑證驗證

  if (telegramClient.connect("api.telegram.org", 443)) {
    Serial.println("連接到 Telegram API 成功");

    String url = "https://api.telegram.org/bot" + String(BOT_TOKEN) +
                 "/sendMessage?chat_id=" + String(CHAT_ID) +
                 "&text=" + message;

    telegramClient.print(String("GET ") + url + " HTTP/1.1\r\n" +
                         "Host: api.telegram.org\r\n" +
                         "Connection: close\r\n\r\n");

    while (telegramClient.connected()) {
      String line = telegramClient.readStringUntil('\n');
      if (line == "\r") break;
    }

    String response = telegramClient.readString();
    Serial.println("Telegram API 回應: " + response);
  } else {
    Serial.println("無法連接到 Telegram API");
  }
}

// 函式：當門狀態變化時發送 MQTT 和 Telegram 訊息
void sendStateChangeMessage(int pin, bool state) {
  String pinStr = (pin == reedSwitchPin1) ? "火警受信機" : "六樓緊急呼叫按鈕";
  String stateStr = state ? "動作!" : "待機中!";
  String timestamp = getCurrentTime();
  String message = pinStr + " 已 " + stateStr + "! 更新時間: " + timestamp;

  // 發送 MQTT 訊息
  if (mqttClient.publish("home/door/state", message.c_str())) {
    Serial.println("MQTT 訊息發佈成功: " + message);
  } else {
    Serial.println("MQTT 訊息發佈失敗");
  }

  // 發送 Telegram 訊息
  sendTelegramMessage(message);
}

void setup() {
  Serial.begin(115200);  // 初始化序列埠
  pinMode(reedSwitchPin1, INPUT_PULLUP);  // 初始化第一個磁簧開關
  pinMode(reedSwitchPin2, INPUT_PULLUP);  // 初始化第二個磁簧開關

  // 使用 WiFiManager 進行 Wi-Fi 連線
  WiFiManager wifiManager;
  if (!wifiManager.autoConnect("ESP8266_AP")) {
    Serial.println("Wi-Fi 連接失敗，將重啟...");
    ESP.restart();  // 若連接失敗則重啟裝置
  }
  Serial.println("Wi-Fi 已連接");
  Serial.print("IP 位址: ");
  Serial.println(WiFi.localIP());

  connectNTP();  // 連接 NTP 伺服器同步時間
  mqttClient.setServer(mqttServer, mqttPort);  // 設定 MQTT 伺服器
  connectMQTT();  // 連接 MQTT

  // 儲存初始狀態
  lastDoorState1 = digitalRead(reedSwitchPin1);
  lastDoorState2 = digitalRead(reedSwitchPin2);
}

void loop() {
  if (!mqttClient.connected()) connectMQTT();  // 若 MQTT 斷線，重連
  mqttClient.loop();  // 保持 MQTT 連接

  // 偵測第一個磁簧開關狀態變化
  doorState1 = digitalRead(reedSwitchPin1);
  if (doorState1 != lastDoorState1) {
    sendStateChangeMessage(reedSwitchPin1, doorState1);
    lastDoorState1 = doorState1;
  }

  // 偵測第二個磁簧開關狀態變化
  doorState2 = digitalRead(reedSwitchPin2);
  if (doorState2 != lastDoorState2) {
    sendStateChangeMessage(reedSwitchPin2, doorState2);
    lastDoorState2 = doorState2;
  }

  delay(100);  // 每 100 毫秒檢查一次狀態
}
