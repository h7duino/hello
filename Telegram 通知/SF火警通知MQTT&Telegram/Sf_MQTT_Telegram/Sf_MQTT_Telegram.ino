/*
MQTT 與 Telegram 整合通知(WIFI設定可以自訂義)
此版使用 WiFiManager 庫來替換現有的 Wi-Fi 連線程式碼，使得 ESP8266 在啟動時允許用戶透過 Web Portal 設定 Wi-Fi，而無需在程式中硬編碼 SSID 和密碼。
如果無法自動連接 Wi-Fi，設備會建立一個 ESP8266_AP 存取點，允許用戶透過瀏覽器配置 Wi-Fi。
程式碼說明
一.Wi-Fi、MQTT 與 Telegram 初始化：
連接 Wi-Fi 並同步 NTP 時間。
連接 MQTT Broker，並設定發送主題為 home/door/state。
初始化 Telegram Bot，使用 Bot Token 與 Chat ID。
二.GPIO 狀態變化偵測：
磁簧開關連接到 GPIO 4，偵測門的開啟或關閉。
三.訊息內容與格式：
每當門狀態變化時，將發送一條包含時間戳記的訊息到 MQTT 和 Telegram。
NTP 時間同步：
使用 configTime() 函數從 NTP 伺服器同步當前時間。
四.測試步驟
硬體接線：
將磁簧開關接到 GPIO 4，另一端接 GND。
當門開啟或關閉時，磁簧開關狀態會變化。
上傳程式碼到 ESP8266。
在 Telegram 中接收通知：
確保 Telegram Bot 已啟用，並能接收到訊息。
五.注意事項:
當你使用 HTTPS 連接 Telegram API 時，它需要耗費較多的 RAM 和處理資源，這可能導致 MQTT 和 Telegram 的連接產生衝突。
為了解決這個問題要分開使用不同的客戶端：
使用 WiFiClient 進行 MQTT 通信（TCP）。
使用 WiFiClientSecure 專門連接 Telegram API（HTTPS）。
降低資源占用：
每次需要發送 Telegram 訊息時建立臨時的 WiFiClientSecure 連接，避免和 MQTT 客戶端同時占用資源。
避免資源衝突：
每次發送 Telegram 訊息時，建立新的 WiFiClientSecure 連接。
六.MQTT 連線狀態碼解釋
-1：MQTT 伺服器無法連接。
-2：MQTT 客戶端 ID 被拒絕。(包括使用TLS加密)
-3：MQTT 伺服器無法認證用戶。
-4：MQTT 主題無法發佈。
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
const int reedSwitchPin = 4;  // 磁簧開關接 GPIO 4
bool doorState = HIGH;         // 儲存當前門狀態
bool lastDoorState = HIGH;     // 儲存上次的門狀態

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
void sendStateChangeMessage(bool state) {
  String stateStr = state ? "火災警報動作" : "待命";
  String timestamp = getCurrentTime();
  String message = "消防受信總機" + stateStr + "中!更新時間: " + timestamp;

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
  pinMode(reedSwitchPin, INPUT_PULLUP);  // 初始化磁簧開關輸入

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

  lastDoorState = digitalRead(reedSwitchPin);  // 儲存初始門狀態
}

void loop() {
  if (!mqttClient.connected()) connectMQTT();  // 若 MQTT 斷線，重連
  mqttClient.loop();  // 保持 MQTT 連接

  // 偵測磁簧開關狀態變化
  doorState = digitalRead(reedSwitchPin);
  if (doorState != lastDoorState) {  // 若門狀態變化
    sendStateChangeMessage(doorState);  // 發送狀態更新訊息
    lastDoorState = doorState;  // 更新狀態!
  }

  delay(100);  // 每 100 毫秒檢查一次狀態!
}
