
//****************************************************************************
/* Code for automatic opening and closing of door using ESP32 arduino board, PIR Sensor, Obstacle avoidance sensor.
It logs the count of entries or occupants each time the door opens and closes.
Logs can be seen on thingsboard, thingspeak and serial monitor

Parts of this code was referenced from several resources online.

The code can be used freely and referenced with no liabilty to me or anyone*/



#include <Arduino.h> 
#include <ESP32Time.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <ESP32Servo.h>
#include "Secret.h"
#include <ThingsBoard.h>    // ThingsBoard SDK
#include "ThingSpeak.h"


// ThingsBoard server instance.
#define TOKEN               "token" //Input your token key from Thingsboard
#define THINGSBOARD_SERVER  "demo.thingsboard.io"

Servo myservo;  // create servo object to control a servo
// 16 servo objects can be created on the ESP32

const char* ssid = SSID;   // Write your SSID in this format "SSID" unless using "Secret.h"
const char* password = PASSWORD;   // Write your WIFI password in this format "PASSWORD" unless using "Secret.h"

WiFiClient  client;

//Initialize ThingsBoard instance
ThingsBoard tb(client);

ESP32Time rtc; //creating a time object

unsigned long Channel_ID = channelID;  //Input your channel ID from Thingspeak
const char * API_key = "API key"; //Input your API key from Thingspeak


unsigned long last_time = 0;
unsigned long Delay = 20000; //was 30000 (Delay used for thingspeak send). If it is set too low 
//which means updating fast, error in update would happen at intervals

unsigned long lastMillis = 0; 

const char* ntpServer = "pool.ntp.org"; //NTP server address
const long  gmtOffset_sec = -18000;  // Time zone EST UTC-5
const int   daylightOffset_sec = 3600;  // 3600 for daylight setting and 0 for no daylight setting

void printLocalTime()
 {
   struct tm timeinfo;
   if(!getLocalTime(&timeinfo)){
     Serial.println("Failed to obtain time");
     return;
   }
   Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

#define PIR 4
#define objectDetect 5
int count = 1;
int occupants;


//  int pos = 0;    // variable to store the servo position
// Recommended PWM GPIO pins on the ESP32 include 2,4,12-19,21-23,25-27,32-33 
// Possible PWM GPIO pins on the ESP32-S2: 0(used by on-board button),1-17,18(used by on-board LED),19-21,26,33-42

//Specify arduino pin for servo
int servoPin = 17;



void setup() {
  Serial.begin(9600);  
  

  // Allow allocation of all timers
	ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
  pinMode(PIR,INPUT);
  pinMode(objectDetect,INPUT);
	myservo.setPeriodHertz(50);    // standard 50 hz servo
	myservo.attach(servoPin, 500, 2500); // attach the servo on pin 17 to the servo object
	// used min/max of 500us and 2500us
	
  
  WiFi.mode(WIFI_STA);   
  
  ThingSpeak.begin(client);  // Initialize ThingSpeak

  /*---------set with NTP---------------*/
 
   //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)){
    rtc.setTimeStruct(timeinfo); 
  }

}

void loop() {

  int motion = digitalRead(PIR);
  int pastdoor = digitalRead(objectDetect);


  if(motion == 1){
            
		      myservo.write(95);    // tell servo to go to position in variable '95' which is door open position
		      //delay(500);             // waits 5ms for the servo to reach the position
	}
  else if(pastdoor ==0){ 
	        
          myservo.write(150);    // tell servo to go to position in variable '150' which is door closed position
		      occupants = count ++;
		      delay(250);   // Delay 2.5ms before returning to 90 degrees
		      // delay(15);             // waits 15ms for the servo to reach the position

          Serial.print ("Occupants = ");
          Serial.println (occupants);
          delay(1000);
          Serial.println(rtc.getDateTime());      //  (String) Sun, Jan 17 2021 15:24:38
          delay(1000);
	}


// Reconnect to WiFi, if needed
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting...");
      while(WiFi.status() != WL_CONNECTED){
        WiFi.begin(ssid, password); 
        delay(5000);     
      } 
      Serial.println("\nConnected to WiFi");
  }

// Set to true if application is subscribed for RPC messages.
  //bool subscribed = false;

  //Reconnect to thingsboard if needed
  
  if (!tb.connected()) {
    

    // Connect to the ThingsBoard
    // Serial.print(" Connecting to ");
    // Serial.println(THINGSBOARD_SERVER);
    // delay (5000);
    // Serial.print(" with token ");
    // Serial.println(TOKEN);
    if (!tb.connect(THINGSBOARD_SERVER, TOKEN)) {
      Serial.println("Failed to connect");
      return;
  
    }
  
  }

  // Write or send thingsboard data
    //("Date & Time: ", (rtc.getDateTime())); //Cannot send in this format as it needs to converted to long string or epoch format
    tb.sendTelemetryFloat("Occupants", occupants);
  
      
  if ((millis() - last_time) > Delay) {
    

    if(WiFi.status() != WL_CONNECTED){
      Serial.print("Connecting...");
      while(WiFi.status() != WL_CONNECTED){
        WiFi.begin(ssid, password); 
        delay(5000);     
      } 
      Serial.println("\nConnected.");
      
      Serial.print("Date & Time: ");
      Serial.println(rtc.getDateTime());
      delay(30000); //Delay for current time to be aquired
    }


    
    Serial.print("Date & Time: ");
    Serial.println(rtc.getDateTime());

// Define thingspeak channel field data
    ThingSpeak.setField(1, (rtc.getDateTime()));
    ThingSpeak.setField(2, occupants);

// Write or send thingspeak channel fields data
//  int Data = ThingSpeak.writeField(Channel_ID, 1, (rtc.getDateTime()),API_key);
    int Data = ThingSpeak.writeFields(Channel_ID,API_key);
    
 
    if(Data == 200){
      Serial.println("Channel updated successfully!");
    }
    else{
      Serial.println("Problem updating channel. HTTP error code " + String(Data));
    }
    last_time = millis();
  }
}