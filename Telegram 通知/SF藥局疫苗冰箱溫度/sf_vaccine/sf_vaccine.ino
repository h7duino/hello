/*程式名稱:ESP8266警報發送的Telegram Bot通知。   
            
專案名稱:藥局冰箱溫度異常監視
版本:sf_vaccine 2024/10/16
環境:arduino ide=1.8.13版
開發板(版本號):2.7.4
           
//Sensor B接點請用3.3V來觸發..
/*線路圖
         NodeMCU        Relay1       Relay2
           VIN                        
           GND           COM          COM
            D1            NC               
            D2                         NC   
            
  

 */



#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>

// Telegram Bot 設定
const char* TELEGRAM_BOT_TOKEN = "7466900614:AAHBZnNOTeZZ6XO4b7nNQlHgIdhDeSvEX_c";  // 替換為您自己的 Token
const char* CHAT_ID = "911750774";  // 替換為您的 Chat ID

int Sensor = D1;   
int Sensor1 = D2;  
int LED = D4;     
boolean flg = false;
boolean flg1 = false;

WiFiClientSecure client;
UniversalTelegramBot bot(TELEGRAM_BOT_TOKEN, client);

void setup() {
  Serial.begin(9600);
  pinMode(Sensor, INPUT_PULLUP);
  pinMode(Sensor1, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);  // LED 熄滅

  // WiFi 連線設定
  WiFiManager wifiManager;
  wifiManager.autoConnect("SING FDNG HOSPITAL");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  client.setInsecure();  // 忽略 SSL 驗證

  // 開機 Telegram 通知
  sendTelegramMessage("冰箱溫度監視重新開機...OK!");
  Serial.println("系統已啟動...");
}

void loop() {
  int val = digitalRead(Sensor);  
  if (val == 1 && !flg) {
    digitalWrite(LED, LOW);  // LED 亮起
    Serial.println("D1..Motion Detected ALARM");
    sendAlert("疫苗冰箱溫度異常!", "https://www.lewei50.com/u/g/13286");
    flg = true;
  }
  if (val == 0) {
    digitalWrite(LED, HIGH);  // LED 熄滅
    flg = false;
  }

  int valb = digitalRead(Sensor1);  
  if (valb == 1 && !flg1) {
    digitalWrite(LED, LOW);  // LED 亮起
    Serial.println("D2..Motion Detected ALARM");
    sendAlert("血庫冰箱溫度異常!", "http://klw.lewei50.com/u/g/12923");
    flg1 = true;
  }
  if (valb == 0) {
    digitalWrite(LED, HIGH);  // LED 熄滅
    flg1 = false;
  }

  delay(1000);  
}

// 發送 Telegram 通知
void sendTelegramMessage(String message) {
  if (bot.sendMessage(CHAT_ID, message, "")) {
    Serial.println("Telegram 訊息已發送");
  } else {
    Serial.println("Telegram 訊息發送失敗");
  }
}

// 發送警告至 Telegram
void sendAlert(String alertMessage, String url) {
  sendTelegramMessage(alertMessage + "\n請至: " + url);
}
