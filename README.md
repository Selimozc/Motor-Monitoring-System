# Motor-Monitoring-System
This project is an IoT-based motor monitoring system developed to track the vibration and temperature values of three-phase electric motors in real time. The system aims to contribute to preventive maintenance and predictive maintenance practices in industrial environments.
📌 Features

ESP32-based microcontroller system

ADXL345 accelerometer for motor vibration measurement

Reduction of 3-axis acceleration data into a single composite axis

Acceleration → velocity conversion and RMS velocity (mm/s) calculation

FFT analysis to transform vibration data from time domain to frequency domain

DS18B20 temperature sensor for motor surface temperature measurement

ThingSpeak IoT integration

Real-time graphical data visualization

Cloud data transmission and storage

Telegram Bot integration

Query real-time temperature and vibration values

Automatic alert messages when critical thresholds are exceeded

🛠️ Hardware Components

ESP32 Development Board

ADXL345 3-Axis Accelerometer

DS18B20 Temperature Sensor

LM2596 Buck Converter (24V → 5V power regulation)

Industrial motor (test environment)

📊 Working Principle

Motor vibrations are collected from the ADXL345 sensor.

Acceleration data is processed and converted into RMS velocity values.

FFT analysis is applied to extract the frequency spectrum.

Temperature is measured using the DS18B20 sensor.

Data is transmitted to the ThingSpeak IoT platform and sent to the user via Telegram Bot.

When predefined threshold values are exceeded, the system automatically sends an alert.

📷 Example Outputs

ThingSpeak dashboard (vibration & temperature trends)

Telegram instant alert messages

🚀 Installation & Usage

Connect the ESP32 board to your computer.

Open Arduino IDE or PlatformIO.

Install the required libraries:

WiFi.h

WiFiClientSecure.h

ThingSpeak.h

OneWire.h

DallasTemperature.h

UniversalTelegramBot.h

ArduinoJson.h

Adafruit_Sensor.h

Adafruit_ADXL345_U.h

arduinoFFT.h

Update the code with your WiFi SSID, password, ThingSpeak API Key, and Telegram Bot Token.

Upload the code to the ESP32.

📌 Future Improvements

MQTT integration

Long-term data storage in a database

Machine learning-based fault classification

Web-based dashboard
