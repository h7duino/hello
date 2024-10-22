/*程式名稱:ESP8266火警警報發送的Telegram Bot通知緊急召回。   
            
專案名稱:FIRE_ALARM緊急召回
版本:sf_fire_alarm 2024/10/16
環境:arduino ide=1.8.13版
開發板(版本號):2.7.4    
 
           
//Sensor B接點請用3.3V來觸發..
/*線路圖
         NodeMCU        Relay1        Relay2         
           VIN                        
           GND           COM            COM
            D1            NC               
            D2                           NC   
            
           

 */
#include <ESP8266WiFi.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>

// Telegram Bot 設定
const char* TELEGRAM_BOT_TOKEN = "7466900614:AAHBZnNOTeZZ6XO4b7nNQlHgIdhDeSvEX_c";  // 替換為您的 Token
const char* CHAT_ID = "-4514156084";  // 替換為您要發送到哪個聊天室的 Chat ID

int Sensor = D1;   // 火警受信總機移報B接點
int Sensor1 = D2;  // 6F人員實體按鍵移報B接點
int LED = D4;      // 內建 LED (D4)
boolean flg = false;   // Sensor 變動偵測旗標
boolean flg1 = false;  // Sensor1 變動偵測旗標

WiFiClientSecure client;
UniversalTelegramBot bot(TELEGRAM_BOT_TOKEN, client);

void setup() {
  Serial.begin(9600);
  pinMode(Sensor, INPUT_PULLUP);   // B接點: GND 長閉，斷開時動作
  pinMode(Sensor1, INPUT_PULLUP);  // B接點: GND 長閉，斷開時動作
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);  // LED 熄滅

  // WiFi 自動連線
  WiFiManager wifiManager;
  wifiManager.autoConnect("SING FDNG HOSPITAL");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  client.setInsecure();  // 忽略 SSL 驗證

  // 開機 Telegram 通知
  sendTelegramMessage("緊急召回通訊設備因斷電或重新開機初始化完成《上線監視中!》");
  Serial.println("Ready....Online.....");
}

void loop() {
  // 讀取 Sensor 狀態 (火警受信總機)
  int val = digitalRead(Sensor);
  if (val == 1 && !flg) {
    digitalWrite(LED, LOW);  // LED 亮起
    Serial.println("D1..Motion Detected ALARM");
    sendAlert("1.火災警報觸發! 請盡速採取必要措施或趕往醫院救災!");
    sendAlert("2.火災警報觸發! 請盡速採取必要措施或趕往醫院救災!");
    sendAlert("3.火災警報觸發! 請盡速採取必要措施或趕往醫院救災!");

    flg = true;
  }
  if (val == 0) {
    digitalWrite(LED, HIGH);  // LED 熄滅
    flg = false;
  }

  // 讀取 Sensor1 狀態 (6F 緊急按鍵)
  int valb = digitalRead(Sensor1);
  if (valb == 1 && !flg1) {
    digitalWrite(LED, LOW);  // LED 亮起
    Serial.println("D2..Motion Detected ALARM");
    sendAlert("1.值班人員發生緊急事件! 請盡速撥打 04-25234112 分機 600 聯絡!");
    sendAlert("2.值班人員發生緊急事件! 請盡速撥打 04-25234112 分機 600 聯絡!");
    sendAlert("3.值班人員發生緊急事件! 請盡速撥打 04-25234112 分機 600 聯絡!");

    flg1 = true;
  }
  if (valb == 0) {
    digitalWrite(LED, HIGH);  // LED 熄滅
    flg1 = false;
  }

  delay(1000);  // 稍作延遲
}

// 發送 Telegram 通知
void sendTelegramMessage(String message) {
  if (bot.sendMessage(CHAT_ID, message, "")) {
    Serial.println("Telegram 訊息已發送");
  } else {
    Serial.println("Telegram 訊息發送失敗");
  }
}

// 發送警告通知
void sendAlert(String alertMessage) {
  sendTelegramMessage(alertMessage);
}
