/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-load-cell-hx711/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

// Library HX711 by Bogdan Necula: https://github.com/bogde/HX711
// Library: pushbutton by polulu: https://github.com/pololu/pushbutton-arduino

#include <Arduino.h>
#include "HX711.h"
#include <Wire.h>
#include <Pushbutton.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <ESP32Servo.h>

//for connecting to cloud service (flask server in this case)
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 16;
const int LOADCELL_SCK_PIN = 4;

HX711 scale;
int cookiesLeft = 3; //to get from server

// this value is obtained by calibrating the scale with known weights;
#define CALIBRATION_FACTOR -706.86251

//OLED Display
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Connect to local WiFi
const char *ssid = "Nokia C01 Plus";
const char *password = "HelloWorld!";

//API Domain name with URL path
const char* statusURL = "https://iot-smartcooikejar.onrender.com/get_command";
const char* uploadWeightURL = "https://iot-smartcooikejar.onrender.com/upload_weight";

//LEDs LED
#define LED_RED 12
#define LED_GREEN 13
#define LED_BLUE 14

#define SWITCH_PIN 17
Pushbutton lever(SWITCH_PIN);

//status of the container
enum Status{
  OVERLIMIT,
  UNDERLIMIT,
  WAIT
};

//values for servo in degrees
enum Lock {
  LOCK = 90,
  UNLOCK = 0
};

//Servo
#define SERVO_PIN 15
Servo servo;

enum Status g_status;
enum Status g_oldStat = UNDERLIMIT;
enum Lock g_lock = UNLOCK;

//retrieves the container status from a JSON string.
//@param response is the JSON string.
Status getValueFromJSON(String response, String key) {
  //convert response to JSON
  JSONVar myObject = JSON.parse(response);

  // JSON.typeof(jsonVar) can be used to get the type of the var
  if (JSON.typeof(myObject) == "undefined") {
    Serial.println("Parsing input failed!");
    return WAIT;
  }

  // myObject.keys() can be used to get an array of all the keys in the object
  //JSONVar keys = myObject.keys();
  JSONVar value = myObject[key];
  Serial.println(value);

  Serial.print(JSON.stringify(value));
  Serial.print("=");
  Serial.println("UNLOCK?");
  Serial.println(JSON.stringify(value) == "\"UNLOCK\"");

  if(JSON.stringify(value) == "\"UNLOCK\""){
    return UNDERLIMIT;
  } else {
    return OVERLIMIT;
  }
}

//https POST Request
//This request posts the weight of the containers contents.
String httpsPOSTRequest(const char* serverName, int weight){
  WiFiClientSecure *client = new WiFiClientSecure;
  String payload = "{}";
  if(client) {
    client->setInsecure();
    HTTPClient https;
    
    https.begin(*client, serverName);
    https.addHeader("Content-Type", "application/json");

    //max is 3kg a buffer of 100 is more than enough
    int bufSize = 100;
    char buf[bufSize];
    snprintf(buf, bufSize, "{\"weight\":\"%d\"}", weight);
    
    //TODO INSERT ACTUAL WEIGHT
    //String httpsRequestData = "{\"weight\":\"10\"}";
    String httpsRequestData = buf;
    int httpsResponseCode = https.POST(httpsRequestData);

    Serial.print("HTTP Response code: ");
    Serial.println(httpsResponseCode);

    if(httpsResponseCode > 0){
      payload = https.getString();
    }
      
    // Free resources
    https.end();
  }
  
  return payload;
}

//https Get Request
//this request retrieves the current status of the container
String httpGETRequest(const char* serverName) {
  WiFiClientSecure *client = new WiFiClientSecure;
  if(client) {
    // set secure client without certificate
    client->setInsecure();
    HTTPClient https;
    https.begin(*client, serverName);

    // Send HTTP get request
    int httpsResponseCode = https.GET();
    
    String payload = "{}"; 
    
    if (httpsResponseCode>0) {
      Serial.print("HTTPS Response code: ");
      Serial.println(httpsResponseCode);
      payload = https.getString();
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpsResponseCode);
    }
    // Free resources
    https.end();
  
    return payload;
  }
}

//turns all LEDs off except one specified
//@param led is the pin number of the LED to turn on.
void switchToLED(int led)
{
  //so fast won't even notice all were off.
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE, LOW);
  
  switch(led)
  {
    case LED_RED:
      digitalWrite(LED_RED, HIGH);
      break;
    case LED_GREEN:
      digitalWrite(LED_GREEN, HIGH);
      break;
    case LED_BLUE:
      digitalWrite(LED_BLUE, HIGH);
      break;
  }
}

//sets the lock to the lock status provided
//@param lock lo
void setLock(enum Lock lock)
{
  servo.write(lock);
}

void setWriteOLED(String message)
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  // Display static text
  display.println(message);
  
  //print cookies left to take.
  if(g_status == UNDERLIMIT)
  {
    display.setTextSize(1);
    display.print("cookies left to take: ");
    display.println(cookiesLeft);
  }

  display.display();
}

//set container status and update
//container hardware accordingly
void setStatus(enum Status stat)
{
  switch(stat){
    case OVERLIMIT:
      if(true){
        g_status = OVERLIMIT;
        g_oldStat = OVERLIMIT;
        switchToLED(LED_RED);
        setLock(LOCK);
        setWriteOLED("Overlimit\nContainer Locked!");
      }
      break;
    case UNDERLIMIT:
      if(true){
        g_status = UNDERLIMIT;
        g_oldStat = UNDERLIMIT;
        switchToLED(LED_GREEN);
        setLock(UNLOCK);
        setWriteOLED("Underlimit\nWell Done!");
      }
      break;
    case WAIT:
      if(true) {
        g_status = WAIT;
        switchToLED(LED_BLUE);
        setWriteOLED("Waiting...");
      }
      break;
  }
}

void setup() {
  Serial.begin(115200);

  //set up servo
  servo.attach(SERVO_PIN);

  //set up display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(2000);
  display.clearDisplay();
  display.setTextColor(WHITE);

  //set up scale
  Serial.println("Initializing the scale");
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  //calibrate 
  scale.set_scale(CALIBRATION_FACTOR);
  scale.tare();               // reset the scale to 0

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  //pinMode(SWITCH_PIN, INPUT);
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_BLUE, LOW);
  digitalWrite(LED_GREEN, LOW);
  
  
  //set up server
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("Connecting");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //retrieve data from backend (cloud) set accordingly.
  String response = httpGETRequest(statusURL);
  Serial.println(response);
  enum Status initialStatus = getValueFromJSON(response, "lid_status");
  setStatus(initialStatus);

  //if lid currently open
  if(digitalRead(SWITCH_PIN)){
    setStatus(WAIT);
  }
  
  delay(10);
}

void loop() {
  static int oldWeight, weight;
  if(lever.getSingleDebouncedRelease())
  {
    //scale
    if (scale.wait_ready_timeout(200)) {
      oldWeight = round(scale.get_units());
      Serial.print("previous weight: ");
      Serial.println(oldWeight);
    }
    else {
      Serial.println("ERROR: HX711 not found.");
    }
    
    setStatus(WAIT);
    
  } else if(lever.getSingleDebouncedPress()){
    if (scale.wait_ready_timeout(200)) {
      weight = round(scale.get_units());
      Serial.print("new weight: ");
      Serial.println(weight);
      int weightChange = weight - oldWeight;
      
      //if difference greater than 3 grams
      if (abs(weightChange) > 3){
          String response = httpsPOSTRequest(uploadWeightURL, weight);
          Serial.println(response);
          enum Status newStatus = getValueFromJSON(response, "lid_status");
          
          setStatus(newStatus);
      } else {
        //if no change to weight
        setStatus(g_oldStat);
      }
    } else {
      Serial.println("HX711 not found.");
    }
  }
}
