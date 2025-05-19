# IoT-SmartCooikeJar
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
- Set daily cooikes consumption amount
- Auto lock when hitting the limitation
- Feedback on the cookie consumption
- UI display

## Public access for user interface
url address: https://iot-smartcooikejar.onrender.com

## Data processing
- Flaks will be used as the backend server, it would get data from the ESP32 and send back the data analysis results.
- The user interface or web pages will be rendered by Flask.
- SQLite would be the database which stores all the cookie intake info.

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

## ⚠️ Data Privacy & Confidentiality Notice
This project may involve sensitive or confidential data (e.g., staff names, contact details, office locations). 
🚫**Do not copy, share, or expose any real personal or organizational data in this repository**.
