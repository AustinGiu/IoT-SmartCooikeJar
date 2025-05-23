# IoT-SmartCookieJar
This is for the use of CITS5506 IoT project

## Group Members
This project is conducted in a group of 3.
| No.       | Student Name         | Student Number     |
|:-------------|:--------------:|---------------:|
| 1      | Joshua Noble    | 22760978   |
| 2        | Zhengxu Jin     | 24117922 |
| 3    | Yumin Zeng  | 24073955    |

## Project Overview
Smart Cookie Jar is an IoT-based snack monitoring system designed to help users manage their daily snack intake. The system uses an ESP32 microcontroller to detect snack removal via a load cell and sends this data to a Flask-based server. The server logs snack consumption, tracks calories and sugar, and enforces a user-defined daily snack limit by activating a locking mechanism when the limit is reached. Additional features include a refill reminder, OLED display with lock status and remaining snacks, red and green LEDs for visual status indication, and a web dashboard that provides snacking analytics such as peak snacking times and nutritional trends. The system is designed for a single user and emphasizes low power consumption, simplicity, and clear user feedback.

## Features
- Set daily cookies consumption amount
- Auto lock when hitting the limit
- Feedback on the cookie consumption
- UI display

## Public access for user interface
The user interface can be found [here](https://iot-smartcooikejar.onrender.com/today)

## Data processing
- Flask is used as the backend server, it obtains data from the ESP32 and sends back the data analysis results.
- The user interface is rendered by Flask.
- SQLite is the database which stores all the cookie intake info.

## The Cookie Ingredient Table(the one used in this project)
| Nutrient      | Per Cookie     |
|---------------|----------------|
| Energy        | 188 kcal       |
| Protein       | 0.43 g         |
| Fat           | 1.87 g         |
| Carbohydrate  | 6.47 g         |
| Sugar         | 3.5 g          |
| Sodium        | 47 mg          |

## Why not letting users to decide the snack and daily limit
The users should have access to choose the snack type and daily limit. However we did not implement this part in our design. Because we choose one type of cookie from the market as the defalut snack type for demo use. As for the users' access for daily limit, if the users could set the limit as they wish, this project would loss the meaning of snack monitor. How the users could eat less if they are allowed to change their mind everytime they want?

## Hardware setup
It's recommended to create a container to store the electrical components and cookies such as the one that was developed for the project for the best experience.

### Wiring
The hardware used for this project was:
- FireBeetle 2 ESP32-E IoT Microcontroller
- Lever Switch.
- 9g Micro Servo - FS90
- HX711 Amplifier
- 3kg beam loadcell.
- Blue OLED I2C Display
- 3x LEDs (blue, green, and red).
- 3x 300 ohm resistors.
- 1x 3kohm resistor.

Follow the schematic diagram below to wire the connections.
![schematic](https://github.com/user-attachments/assets/b3e8ed7c-5d86-47b3-ab6b-14a944964ee3)

### Arduino Libraries
Make sure the Arduino IDE is installed on your device. Once installed install the following libraries via the Arduino IDE Library Manager, which can be found under the *tools* tab:
- **Pushbutton** by Pololu v2.0
- **Arduino_JSON** by Arduino v0.2
- **ESP32Servo** by Kevin Harrington v3.0.6
- **WiFi** by Arduino v1.2.7
- **Adafruit SSD1306** by Adafruit v2.5.13
- **Adafruit GFX Library** by Adafruit v1.12
- **HX711 Arduino Library** by Bogdan Necula v0.7.5

Once the above libraries have been installed open the SmartCalorieWatcher.ino from your Arduino IDE, plug your ESP32 into one of your laptop ports then in the Arduino IDE open tools->ports and check that the correct port is selected if not select the correct port and press the *Upload* button.

Once the code has been installed onto the ESP32 the SmartCalorieWatcher will now be functional!

## ⚠️ Data Privacy & Confidentiality Notice
This project may involve sensitive or confidential data (e.g., staff names, contact details, office locations). 
🚫**Do not copy, share, or expose any real personal or organizational data in this repository**.
