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
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Pushbutton.h>
#include <WiFi.h>
#include <NetworkClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ESP32Servo.h>

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 16;
const int LOADCELL_SCK_PIN = 4;

HX711 scale;
int reading;
int lastReading;
int weightChange;
int snackWeight = 10;
int limit = 3;
int cookiesLeft = limit;
//REPLACE WITH YOUR CALIBRATION FACTOR
#define CALIBRATION_FACTOR -706.86251

//OLED Display
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Button
#define BUTTON_PIN 27
Pushbutton button(BUTTON_PIN);


//Webserver
const char *ssid = "Nokia C01 Plus";
const char *password = "HelloWorld!";
WebServer server(80);

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

//turns all LEDs off except one specified
//by 'led' parameter
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
void consumptionManagementPage() {
  digitalWrite(LED_BUILTIN, HIGH);
  
  char temp[2500];

  snprintf(
    temp, 2500,

    R"(<!DOCTYPE html>
<html lang="en">

<head>
  <meta charset="UTF-8">
  <title>Today's Consumption - Smart Cookie Jar</title>
  <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css">
  <script src="https://code.jquery.com/jquery-3.6.0.min.js"></script>
  <link rel="stylesheet" href="{{ url_for('static', filename='css/styles_today.css') }}">
</head>

<body class="today">
  <div class="wrapper">
    <div class="container">
      <div>
        <h1 class="mb-4 text-center">Smart Cookie Jar</h1>
        <h2 class="mb-4 text-center">Today's Consumption</h2>
        <p class="text-center" id="intro">Fancy some cookies?</p>
      </div>

      <!-- Current snack status section -->
      <section class="mb-4">
        <h4>Current Status</h4>
        <p>Current Cookies' Weight: <span id="current-weight"></span> g</p>
        <p>Consumed Pieces: <span id="pieces-consumed"></span></p>
        <p>Daily Limit: <span id="daily-limit">5</span> pieces</p>
        <p>You could eat <span id="remaining-allowance"></span> pieces more for today</p>
        <p>Lid Lock: <span id="status-message" class="fw-bold"></span></p>
      </section>

      <!-- Snack consumption log table -->
      <section>
        <h4>Wanna know how much you have taken?</h4>
        <table class="table table-striped">
          <thead>
            <tr>
              <th>Energy (KJ)</th>
              <th>Protein(g)</th>
              <th>Fat(g)</th>
              <th>Carbohydrate(g)</th>
              <th>Sugar(g)</th>
              <th>Sodium(mg)</th>
            </tr>
          </thead>
          <tbody id="log-table">
            <!-- Snack log entries will be dynamically inserted here -->
          </tbody>
        </table>
      </section>

      <!--button to past data analysis part-->
      <div class="center-button">
        <a href="{{ url_for('html_routes.past_page') }}">
          <button class="btn btn-primary">View Past Data</button>
        </a>
      </div>

    </div>

    <!-- Footer sticks to bottom -->
    <footer class="footer mt-5">
      <p>CITS5506 &copy; Group 9 IoT Project (2025 S1)</p>
    </footer>
  </div>

  <script src="{{ url_for('static', filename='js/script.js') }}"></script>

</body>

</html>)"
  );
  server.send(200, "text/html", temp);
  
  digitalWrite(LED_BUILTIN, LOW);
}

void snackTrackingPage() {
  digitalWrite(LED_BUILTIN, HIGH);
  
  char temp[2500];

  snprintf(
    temp, 2500,
    R"(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>Snack Tracking - Smart Calorie Watcher</title>
  <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css">
</head>
<body class="container py-4">

  <h1 class="mb-4">Snack Tracking</h1>

  <!-- Current snack status section -->
  <section class="mb-4">
    <h4>Current Status</h4>
    <p>Current Weight: <span id="current-weight">--</span> g</p>
    <p>Consumed Pieces: <span id="pieces-consumed">--</span></p>
    <p>Daily Limit: <span id="daily-limit">--</span> pieces</p>
    <p>Remaining Allowance: <span id="remaining-allowance">--</span> pieces</p>
    <p>Status: <span id="status-message" class="fw-bold">--</span></p>
  </section>

  <!-- Snack configuration form -->
  <section class="mb-4">
    <h4>Snack Configuration</h4>
    <form id="snack-settings-form">
      <div class="mb-2">
        <label for="snack-type" class="form-label">Snack Type</label>
        <input type="text" id="snack-type" class="form-control" placeholder="e.g., Cookie" required>
      </div>
      <div class="mb-2">
        <label for="unit-weight" class="form-label">Weight per Piece (g)</label>
        <input type="number" id="unit-weight" class="form-control" step="0.1" required>
      </div>
      <div class="mb-2">
        <label for="max-daily-limit" class="form-label">Daily Limit (pieces)</label>
        <input type="number" id="max-daily-limit" class="form-control" required>
      </div>
      <button type="submit" class="btn btn-primary mt-2">Save Settings</button>
    </form>
  </section>

  <!-- Change snack type button -->
  <section class="mb-4">
    <button class="btn btn-secondary" id="change-snack-btn">Change Snack Type</button>
  </section>

  <!-- Snack consumption log table -->
  <section>
    <h4>Snack Log</h4>
    <table class="table table-striped">
      <thead>
        <tr>
          <th>Time</th>
          <th>Type</th>
          <th>Pieces</th>
          <th>Weight (g)</th>
          <th>Calories</th>
        </tr>
      </thead>
      <tbody id="log-table">
        <!-- Snack log entries will be dynamically inserted here -->
      </tbody>
    </table>
  </section>

  <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js"></script>
  <!-- JavaScript for dynamic updates will be added here -->
</body>
</html>)"
  );
  server.send(200, "text/html", temp);
  
  digitalWrite(LED_BUILTIN, LOW);
}

//graph that's drawn for the above page.
void drawGraph() {
  String out = "";
  char temp[100];
  out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"400\" height=\"150\">\n";
  out += "<rect width=\"400\" height=\"150\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n";
  out += "<g stroke=\"black\">\n";
  int y = rand() % 130;
  for (int x = 10; x < 390; x += 10) {
    int y2 = rand() % 130;
    sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"1\" />\n", x, 140 - y, x + 10, 140 - y2);
    out += temp;
    y = y2;
  }
  out += "</g>\n</svg>\n";

  server.send(200, "image/svg+xml", out);
}

void handleNotFound() {
  digitalWrite(LED_BUILTIN, HIGH);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
  digitalWrite(LED_BUILTIN, LOW);
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

  scale.set_scale(CALIBRATION_FACTOR);   // this value is obtained by calibrating the scale with known weights; see the README for details
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
  Serial.println("");

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

  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }

  //set handles
  server.on("/", snackTrackingPage);
  server.on("/consumption", consumptionManagementPage); 
  server.on("/test.svg", drawGraph);
  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");


  //TODO
  //retrieve data from backend (cloud) set accordingly.
  //for now just set container to wait status
  delay(10);
  setStatus(WAIT);
}

void loop() {
  static int switchState; 
  //handle client
  server.handleClient();
  delay(2);  //allow the cpu to switch to other tasks
  static int stateToggle = 0;

  if(lever.getSingleDebouncedRelease())
  {
    stateToggle = (stateToggle + 1)%2; // for testing
    
    //scale
    if (scale.wait_ready_timeout(200)) {
      reading = round(scale.get_units());
      Serial.print("previous weight: ");
      Serial.println(reading);
      lastReading = reading;
    }
    else {
      Serial.println("HX711 not found.");
    }
    
    Serial.print("Wait\n");
    setStatus(WAIT);
    
  } else if(lever.getSingleDebouncedPress()){
    Serial.print(stateToggle);
    Serial.print(" Toggle\n");

    if (scale.wait_ready_timeout(200)) {
      reading = round(scale.get_units());
      Serial.print("new weight: ");
      Serial.println(reading);
      
      //if difference greater than 3 grams
      if (abs(reading - lastReading) > 3){
        weightChange = lastReading - reading;
        if(weightChange > 0) {
          
          //cookies taken
          Serial.print("weight taken ");
          Serial.println(weightChange);

          cookiesLeft -= weightChange/snackWeight;

          //update status depending if over limit
          if(cookiesLeft < 1)
          {
            setStatus(OVERLIMIT);
          } else {
            setStatus(UNDERLIMIT);
          }
        } else {
          //container refilled
          Serial.print("weight added ");
          //abs since it's negative
          Serial.println(abs(weightChange));
          
          //if refill set to old status
          setStatus(g_oldStat);
        }
      } else {
        //if no change to weight
        setStatus(g_oldStat);
      }
    }
    else {
      Serial.println("HX711 not found.");
    }
  }
    
  //button
  if (button.getSingleDebouncedPress()){
    Serial.print("tare...");
    scale.tare();
  }
}
