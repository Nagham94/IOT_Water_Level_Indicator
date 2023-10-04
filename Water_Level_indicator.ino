#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo myservo;// servo object 

//WiFi
const char *ssid = "Wokwi-GUEST";
const char *password = "";
//MQTT Broker
const char *mqtt_broker = "broker.emqx.io"; // public broker
const char *topic_publish = "emqx/esp32/publish"; // topic esp publish
const char *topic_subscribe = "emqx/esp32/subscribe"; // topic esp subscribe
const char *mqtt_username = "water";
const char *mqtt_password = "level";
const int mqtt_port = 1883;

WiFiClient espClient; //WiFi Object
PubSubClient Client(espClient);

#define PIN_TRIG 27
#define PIN_ECHO 26
#define buzzer 12
//The tank maximum hight in cm
float tank_height = 400;

//The indication levels
float Very_Low = (tank_height * 80) / 100; // 320 ----> 20%
float Low = (tank_height * 60) / 100;    // 240 ----> 40%
float Halfway = (tank_height * 50) / 100;    // 200 ----> 50%
float Almost_full = (tank_height * 40) / 100;   // 160 ----> 60%
float Full = (tank_height * 20) / 100;    // 80 ----> 80%

void setup() {
  Serial.begin(115200);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(buzzer,OUTPUT);
  wifi_setup();
  Serial.println("After connect MQTT");
  myservo.attach(5);
  Client.subscribe(topic_subscribe);
  Client.setCallback(callback);
  lcd.init(); 
  lcd.backlight();
}

void loop() {
  Client.loop();
  // Start a new measurement:
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  water_level();
  delay(1000);
  lcd.clear();
}

void callback(char* topic, byte* message, unsigned int length){
  printPayload(topic, message, length);
}

void printPayload(char* topic, byte* message, unsigned int length){
  //printing recieved message
  Serial.print("Message recieved on topic: ");
  Serial.print(topic);
  String messageTemp;
  for (int i = 0; i < length; i++){
    messageTemp += (char)message[i];
  }
  Serial.println(messageTemp);
  if(String(topic)== "emqx/esp32/subscribe") { /// esp32 subscribe topic //0,50,100   
   int status = messageTemp.toInt();   
   //int pos = map(status, 1, 100, 0, 180);//100
   Serial.println(status);
    myservo.write(status);
    delay(15);}
}

void wifi_setup() {
  //Connecting to WiFi network
   WiFi.begin(ssid,password);
   while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.println("Connecting to WiFi..");
   }
   Serial.println("Connected to the WiFi network");
   //utilize pubsubClient to establish a connection with the MQTT broker
   //connecting to a MQTT broker
   Serial.println("Before connect to MQTT");
   //broker configuration
   Client.setServer(mqtt_broker,mqtt_port);
   while (!Client.connected()){
    String client_id = "esp32.client";
    client_id += String(WiFi.macAddress());
    // esp32.id -----> esp32.client.macaddress
    //variable.C_str() -----> char [] = string
    Serial.printf("The client %s connects to the public MQTT broker\n", client_id.c_str());
    if (Client.connect(client_id.c_str(), mqtt_username, mqtt_password)){
      Serial.println("Public MQTT broker connected");
    }
    else {
      Serial.print("failed with state ");
      Serial.print(Client.state());
      delay(2000);
    }
   }
}

void water_level() {
   // Read the result:
  int duration = pulseIn(PIN_ECHO, HIGH);
  Serial.print("Distance in CM: ");
  Serial.println(duration / 58);
  int distance = duration / 58;
  double percentage = ((tank_height - distance)*100.0)/tank_height;
  char payload [10];
  sprintf(payload, "%f" , percentage);
  Client.publish(topic_publish, payload);

  if ( distance >= tank_height) {
      delay(1000);
    }
  else if (Very_Low <= distance) {
      Serial.println("Very low level");
      Serial.print("Percentage is: ");
      Serial.print(percentage);
      Serial.println("%");
      lcd.setCursor(0,0);
      lcd.print("Very low level");
      for(int i=0; i<3; i++){
        tone(buzzer, 262, 500);
        delay(250);
      }

   } 
  else if (Low <= distance && Very_Low > distance) {
    
      Serial.println("low level");
      Serial.print("Percentage is: ");
      Serial.print(percentage);
      Serial.println("%");
      lcd.setCursor(0,0);
      lcd.print("low level");
      delay(1000); 
  } 
  else if (Halfway <= distance && Low > distance) {
    
      Serial.println("Halfway");
      Serial.print("Percentage is: ");
      Serial.print(percentage);
      Serial.println("%");
      lcd.setCursor(0,0);
      lcd.print("Halfway");
      delay(1000);
 }               
  else if (Almost_full <= distance && Halfway > distance) {
    
      Serial.println("High level");
      Serial.print("Percentage is: ");
      Serial.print(percentage);
      Serial.println("%");
      lcd.setCursor(0,0);
      lcd.print("High level");
      delay(1000);   
 } 
  else if (Full <= distance && Almost_full > distance) {
    
      Serial.println("Very High level");
      Serial.print("Percentage is: ");
      Serial.print(percentage);
      Serial.println("%");
      lcd.setCursor(0,0);
      lcd.print("Very High level");
      delay(1000);   
 }  
  else if (Full >= distance ) {

      Serial.println("Tank is Full");
      Serial.print("Percentage is: ");
      Serial.print(percentage);
      Serial.println("%");
      lcd.setCursor(0,0);
      lcd.print("Tank is Full");
      for(int i=0; i<3; i++){
        tone(buzzer, 262, 500);
        delay(250);
      }

        }

}