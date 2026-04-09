#include <ESP8266WiFi.h>      
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESP8266HTTPClient.h>  
#include <WiFiClientSecure.h> 

// wifi
#define ssid "your ssid"
#define password "your password"

// telegram
#define BOT_TOKEN "your bot token"  
#define CHAT_ID "your chatid"     

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800); 

struct Reminder {
  int hour;
  int minute;
  bool triggered; 
};

Reminder reminders[] = {
  {0, 10, false},
  {20, 20, false},
  {11, 37, false},   
  {9, 23, false},  
  {18, 30, false}  
};

const int NUM_REMINDERS = sizeof(reminders) / sizeof(reminders[0]);

bool medicinePending = false;
unsigned long lastNagTime = 0;
const unsigned long nagInterval = 120000; 
long lastProcessedUpdateId = 0; 

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nConnected!");
  timeClient.begin();
  
  // Initial sync to ignore old messages
  lastProcessedUpdateId = getLatestUpdateId();
}

void loop() {
  timeClient.update();
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();

  String currentTimeStr = (currentHour < 10 ? "0" : "") + String(currentHour) + ":" + (currentMinute < 10 ? "0" : "") + String(currentMinute);

  for(int i = 0; i < NUM_REMINDERS; i++) {
    if(currentHour == reminders[i].hour && currentMinute == reminders[i].minute && !reminders[i].triggered) {
      Serial.println("Reminder text send");
      sendTelegramNotification("Reminder: Medicine time! It is " + currentTimeStr);
      reminders[i].triggered = true;
      medicinePending = true; 
      lastNagTime = millis();
      lastProcessedUpdateId = getLatestUpdateId(); 
    }
    
    if(currentMinute != reminders[i].minute) {
      reminders[i].triggered = false;
    }
  }

  if(medicinePending) {
    checkTelegramForAck(); 

    if(millis() - lastNagTime >= nagInterval && medicinePending) {
      Serial.println("Rechecking message send"); 
      sendTelegramNotification("Have you taken your medicine yet? Reply yes to stop.");
      lastNagTime = millis();
    }
  }

  delay(2000); 
}

// Handles spaces in messages automatically
String cleanUrl(String text) {
  text.replace(" ", "%20");
  return text;
}

void sendTelegramNotification(String message) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;
  String url = "https://api.telegram.org/bot" + String(BOT_TOKEN) + "/sendMessage?chat_id=" + String(CHAT_ID) + "&text=" + cleanUrl(message);
  
  if (https.begin(client, url)) {
    https.GET();
    https.end();
  }
}

long getLatestUpdateId() {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;
  String url = "https://api.telegram.org/bot" + String(BOT_TOKEN) + "/getUpdates?offset=-1";
  if (https.begin(client, url)) {
    int httpCode = https.GET();
    if (httpCode > 0) {
      String payload = https.getString();
      int pos = payload.indexOf("\"update_id\":");
      if (pos > 0) {
        String idStr = payload.substring(pos + 12, payload.indexOf(",", pos));
        https.end();
        return idStr.toInt();
      }
    }
    https.end();
  }
  return 0;
}

void checkTelegramForAck() {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;
  String url = "https://api.telegram.org/bot" + String(BOT_TOKEN) + "/getUpdates?offset=-1";
  
  if (https.begin(client, url)) {
    int httpCode = https.GET();
    if (httpCode > 0) {
      String payload = https.getString();
      payload.toLowerCase();
      
      int pos = payload.indexOf("\"update_id\":");
      if (pos > 0) {
        long currentUpdateId = payload.substring(pos + 12, payload.indexOf(",", pos)).toInt();
        
        if (currentUpdateId > lastProcessedUpdateId) {
          if (payload.indexOf("\"text\":\"yes\"") > 0) {
            Serial.println("yes received");
            medicinePending = false; // This stops the nagging loop immediately
            lastProcessedUpdateId = currentUpdateId;
          } else {
            lastProcessedUpdateId = currentUpdateId;
          }
        }
      }
    }
    https.end();
  }
}