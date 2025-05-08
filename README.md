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


## ⚠️ Data Privacy & Confidentiality Notice
This project may involve sensitive or confidential data (e.g., staff names, contact details, office locations). 
🚫**Do not copy, share, or expose any real personal or organizational data in this repository**.
