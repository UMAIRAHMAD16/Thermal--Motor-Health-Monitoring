# Thermal--Motor-Health-Monitoring
#  ESP32 Thermal Imaging System

A real-time thermal imaging system using ESP32 and AMG8833 sensor for monitoring temperature distribution and detecting motor faults.


##  Features

- Real-time thermal visualization on TFT display
- Web-based heatmap using ESP32 WiFi
- Temperature interpolation (8x8 → 32x32 grid)
- Color mapping (cold → hot)
- Useful for predictive maintenance


##  Hardware Used

- ESP32
- AMG8833 Thermal Sensor
- ST7735 TFT Display


##  Technologies

- Embedded C / Arduino
- ESP32 WiFi (WebServer)
- I2C Communication
- HTML Canvas (for web heatmap)


##  Output
<img width="1600" height="1251" alt="images jpg" src="https://github.com/user-attachments/assets/340c6b2b-9475-4752-a37b-e293f4b1e492" />


##  How it Works

1. AMG8833 captures temperature (8x8 grid)
2. Data is interpolated to 32x32
3. Color mapping applied
4. Displayed on TFT + Web interface


##  Applications

- Motor health monitoring
- Fault detection
- Industrial automation
- Smart maintenance systems


##  Code

Main logic is implemented in 'main.ino'

 Author

Umair Ahmad
