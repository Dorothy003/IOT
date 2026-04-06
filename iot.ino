#include <ESP8266WiFi.h>      
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESP8266HTTPClient.h>  

//wifi
#define ssid "Pixel_7a"
#define password "papaya99"

//telegram
#define BOT_TOKEN "8567672316:AAHGdv0wi1w9m39DALHJ_vgvehQVJHg1Jeg"  
#define CHAT_ID "1285319071"     

// ntp server
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800); 

struct Reminder {
  int hour;
  int minute;
  bool triggered; 
};

// test- indian railway timing
Reminder reminders[] = {
  {11, 37, false},   
  {9, 23, false},  
  {18, 30, false}  
};

const int NUM_REMINDERS = sizeof(reminders) / sizeof(reminders[0]);

void setup() {
  Serial.begin(115200);

  // Connect Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Start NTP server
  timeClient.begin();
  timeClient.update();
}

void loop() {
  timeClient.update(); // fetch current time

  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();
  
  Serial.printf("Current Time: %02d:%02d\n", currentHour, currentMinute);


  for(int i=0;i<NUM_REMINDERS;i++){
    if(currentHour == reminders[i].hour && currentMinute == reminders[i].minute && !reminders[i].triggered){
      sendTelegramNotification(reminders[i]);
      reminders[i].triggered = true;
    }

    // Reset trigger for next day
    if(currentHour != reminders[i].hour || currentMinute != reminders[i].minute){
      reminders[i].triggered = false;
    }
  }

  delay(1000);
}

void sendTelegramNotification(Reminder r){
  Serial.printf("Reminder: %02d:%02d - Sending Telegram...\n", r.hour, r.minute);

  if(WiFi.status() == WL_CONNECTED){
    WiFiClientSecure client;
    client.setInsecure(); 
    HTTPClient https;

    String message = "Time to take your medicine! (" + String(r.hour) + ":" + String(r.minute) + ")";
    String url = "https://api.telegram.org/bot" + String(BOT_TOKEN) + 
                 "/sendMessage?chat_id=" + String(CHAT_ID) +
                 "&text=" + message;
    
    https.begin(client, url);
    int code = https.GET();
    if(code>0){
      Serial.println("Telegram message sent!");
    } else {
      Serial.println("Error sending Telegram message");
    }
    https.end();
  }
}
