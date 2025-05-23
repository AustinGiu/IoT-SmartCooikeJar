/*
 * UWA, Computer Science and Software Engineering
 * CITS5506 Internet of Things
 * 
 * Joshua Noble
 * 
 * This code was developed for the Smart Calorie Watcher.
 * It contains the code required to read the sensors, control the 
 * actuators and communicate with the deployed flask server.
 * 
 * Sensors: Lever Switch and Load Cell.
 * Actuators: OLED, LEDs and Servo.
 * 
 * May 2025
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
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>

// this value is obtained by calibrating the scale with known weights;
#define CALIBRATION_FACTOR -706.86251

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 16;
const int LOADCELL_SCK_PIN = 4;
HX711 scale;

//OLED Display size in pixels
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//LED pins
#define LED_RED 12
#define LED_GREEN 13
#define LED_BLUE 14

//switch
#define SWITCH_PIN 17
Pushbutton lever(SWITCH_PIN);

//Servo
#define SERVO_PIN 15
Servo servo;

//status of the container
enum Status{
  OVERLIMIT,
  UNDERLIMIT,
  WAIT
};

//values for servo in degrees
enum Lock {
  LOCK = 0,
  UNLOCK = 90
};

//container status
enum Status g_status;
enum Status g_oldStat = UNDERLIMIT;
enum Lock g_lock = UNLOCK;

//global variables to obtain from the server
int cookiesLeft = -1; 
String timer = "";

//Connect to local WiFi
const char *ssid = "Nokia C01 Plus";
const char *password = "HelloWorld!";

//API Domain name with URL path
const char* statusURL = "https://iot-smartcooikejar.onrender.com/get_command";
const char* uploadWeightURL = "https://iot-smartcooikejar.onrender.com/upload_weight";

/*
 * parse JSON response and update container status, 
 * number of cookies left to take and the lock duration.
 * @param json is the json to parse.
 */
void updateContainerFromJSON(JSONVar json){
  String stat = getValueFromJSON(json, "lid_status");
  String left = getValueFromJSON(json, "cookies_remaining_today");
  String lockDur = getValueFromJSON(json, "lock_until");

  //update global variables
  cookiesLeft = left.toInt();
  timer = lockDur;

  //update status
  enum Status newStatus = getStatusFromString(stat);
  setStatus(newStatus);
}

/*
 * Converts status string to a status enum.
 * Assumes stat will either be "UNLOCK" or "LOCK".
 * @stat is the status string to convert.
 * @return the status enum corresponding to the string.
 */
Status getStatusFromString(String stat) {
  if(stat == "\"UNLOCK\""){
    return UNDERLIMIT;
  } else {
    return OVERLIMIT;
  }
}

/*
 * Converts https response containing JSON to a JSON object.
 * @param response https response containing JSON
 * @return JSONVar object.
 */
JSONVar responseToJSON(String response){
  return JSON.parse(response);
}

/*
 * Retrieves value specified from the JSON object.
 * Assumes 'key' param will be a key that exists in the JSON object.
 * @param key of the value to obtain.
 * @param myObject JSON object to obtain value from.
 * @return the value specified as a string.
 */
String getValueFromJSON(JSONVar myObject, String key) {
  if (JSON.typeof(myObject) == "undefined") {
    Serial.println("Parsing input failed!");
    return "";
  }
  
  JSONVar value = myObject[key];

  return JSON.stringify(value);
}

/*
 * Sends a https post request containing the container weight to the api handle specified.
 * @param serverName URL of the server including the handle.
 * @param weight the weight obtained by the container.
 * @return the server response containing the new container status as a string.
 */
String httpsPOSTRequest(const char* serverName, int weight){
  WiFiClientSecure *client = new WiFiClientSecure;
  String payload = "{}";
  if(client) {
    //insecure because https but no certificate is used.
    client->setInsecure();
    HTTPClient https;
    
    https.begin(*client, serverName);
    https.addHeader("Content-Type", "application/json");

    //max is 3kg a buffer of 100 is more than enough
    int bufSize = 100;
    char buf[bufSize];
    snprintf(buf, bufSize, "{\"weight\":\"%d\"}", weight);
    
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

/*
 * Sends a https get request to the server to obtain the current status of the container.
 * @param the URL of the server including the handle.
 * @return the current status of the container as a string.
 */
String httpGETRequest(const char* serverName) {
  WiFiClientSecure *client = new WiFiClientSecure;
  if(client) {
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

/*
 * Turns all LEDs off except the one specified by the 'led' param.
 * @param led is the pin number of the LED to turn on.
 */
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

/*
 * Sets the lock to the lock status provided.
 * @param lock the status of the lock.
 */
void setLock(enum Lock lock)
{
  servo.write(lock);
}

/*
 * Writes the provided message to the OLED.
 * @param message the message to display on the OLED.
 */
void setWriteOLED(String message)
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  
  // Display static text
  display.println(message);
  display.display();
}

/*
 * Set the container status and update the container hardware accordingly.
 * @param stat the status to assign to the container.
 */
void setStatus(enum Status stat)
{
  switch(stat){
    case OVERLIMIT:
      g_status = OVERLIMIT;
      g_oldStat = OVERLIMIT;
      switchToLED(LED_RED);
      setLock(LOCK);
      setWriteOLED("Unlocks on\n" + timer.substring(1,9) + "\n" + timer.substring(11,18));
      break;
    case UNDERLIMIT:
      g_status = UNDERLIMIT;
      g_oldStat = UNDERLIMIT;
      switchToLED(LED_GREEN);
      setLock(UNLOCK);
      
      //construct string
      int bufSize = 100;
      char message[bufSize];
      snprintf(message, bufSize, "Underlimit\nWell Done!\nCan take %d", cookiesLeft);
      
      //set OLED to message
      setWriteOLED(message);
      break;
    case WAIT:
      g_status = WAIT;
      switchToLED(LED_BLUE);
      setWriteOLED("Waiting...");
      break;
  }
}

/*
 * Function that runs when ESP32 first boots up.
 */
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
  //reset scale to 0.
  scale.tare(); 

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
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
  
  JSONVar json = responseToJSON(response);
  updateContainerFromJSON(json);

  //if lid currently open
  if(digitalRead(SWITCH_PIN)){
    setStatus(WAIT);
  }
  
  delay(10);
}

/*
 * Function that loops forever. Runs after setup function.
 */
void loop() {
  static int oldWeight, weight;

  //lid just opened
  if(lever.getSingleDebouncedRelease())
  {
    //if scale not ready wait.
    if (scale.wait_ready_timeout(200)) {
      oldWeight = round(scale.get_units());
      Serial.print("previous weight: ");
      Serial.println(oldWeight);
    }
    else {
      Serial.println("ERROR: HX711 not found.");
    }

    setStatus(WAIT);

    //lid just closed
  } else if(lever.getSingleDebouncedPress()){
    if (scale.wait_ready_timeout(200)) {
      weight = round(scale.get_units());
      Serial.print("new weight: ");
      Serial.println(weight);
      
      int weightChange = weight - oldWeight;
      
      //if difference greater than 3 grams (to account for drift)
      if (abs(weightChange) > 3){
        setWriteOLED("Please\nWait...");
        
        //send current weight to container
        String response = httpsPOSTRequest(uploadWeightURL, weight);
        Serial.println(response);
        
        JSONVar json = responseToJSON(response);
        updateContainerFromJSON(json);
      } else {
        //if no change to weight set to old status.
        setStatus(g_oldStat);
      }
    } else {
      Serial.println("HX711 not found.");
    }
  }
}
