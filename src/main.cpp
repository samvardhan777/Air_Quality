#include "DHT.h"
#include <MQ135.h>
#include <MQUnifiedsensor.h>
#include <LiquidCrystal.h>
#include <Arduino.h>

// Definitions for MQ sensors and DHT
#define BOARD "Arduino UNO"
#define VOLTAGE_RESOLUTION 5
#define ADC_BIT_RESOLUTION 10
#define RATIO_MQ7_CLEAN_AIR 27.5 // RS/R0 ratio in clean air for MQ-7
#define RATIO_MQ135_CLEAN_AIR 3.6 // RS/R0 ratio in clean air for MQ-135

const int rs = 9, en = 8, d4 = 6, d5 = 5, d6 = 4, d7 = 3;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Pin definitions
#define DHT_PIN 2 // DHT sensor pin
#define MQ7_PIN A4 // Analog input for MQ7
#define MQ135_PIN A0 // Analog input for MQ135
#define PWM_PIN 5 // PWM control pin for MQ7 heating


// Define thresholds for harmful conditions
const float CO_THRESHOLD = 4.0; // Example threshold for CO in ppm


// Sensor Objects
DHT dht;
MQUnifiedsensor MQ7(BOARD, VOLTAGE_RESOLUTION, ADC_BIT_RESOLUTION, MQ7_PIN, "MQ-7");
MQUnifiedsensor MQ135(BOARD, VOLTAGE_RESOLUTION, ADC_BIT_RESOLUTION, MQ135_PIN, "MQ-135");


void setup() {
  Serial.begin(9600);
  dht.setup(2); // data pin 2

    // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  lcd.print("Air Quality");
  lcd.setCursor(0, 1);
  lcd.print("Meter");
    /*
    MQ-135
    Exponential regression:
  GAS      | a      | b
  CO       | 605.18 | -3.937  
  Alcohol  | 77.255 | -3.18 
  CO2      | 110.47 | -2.862
  Toluen  | 44.947 | -3.445
  NH4      | 102.2  | -2.473
  Aceton  | 34.668 | -3.369
  */
  

  // Initialize and calibrate MQ135
  Serial.print("Calibrating MQ135, please wait...");
  MQ135.setRegressionMethod(1); //_PPM =  a*ratio^b
  MQ135.setA(102.2); MQ135.setB(-2.473);
  MQ135.init();
  float calcR0MQ135 = 0;
  for (int i = 1; i <= 10; i++) {
    MQ135.update();
    calcR0MQ135 += MQ135.calibrate(RATIO_MQ135_CLEAN_AIR);
    Serial.print(".");
  }
  MQ135.setR0(calcR0MQ135 / 10);
  Serial.println(" done!");

  // Error handling for MQ135
  if (isinf(calcR0MQ135) || calcR0MQ135 == 0) {
    Serial.println("Error: Connection issue detected for MQ135. Please check your wiring and supply.");
    while (1);
  }
  MQ135.serialDebug(true);

  MQ7.setRegressionMethod(1); //_PPM =  a*ratio^b
  MQ7.setA(99.042); MQ7.setB(-1.518); // Configure the equation to calculate CO concentration value

    /*
    MQ-7
    Exponential regression:
  GAS     | a      | b
  H2      | 69.014  | -1.374
  LPG     | 700000000 | -7.703
  CH4     | 60000000000000 | -10.54
  CO      | 99.042 | -1.518
  Alcohol | 40000000000000000 | -12.35
  */

  // Initialize and calibrate MQ7
  Serial.print("Calibrating MQ7, please wait...");
  MQ7.init();
  float calcR0MQ7 = 0;
  for (int i = 1; i <= 10; i++) {
    MQ7.update();
    calcR0MQ7 += MQ7.calibrate(RATIO_MQ7_CLEAN_AIR);
    Serial.print(".");
  }
  MQ7.setR0(calcR0MQ7 / 10);
  Serial.println(" done!");

  // Error handling for MQ7
  if (isinf(calcR0MQ7) || calcR0MQ7 == 0) {
    Serial.println("Error: Connection issue detected for MQ7. Please check your wiring and supply.");
    while (1);
  }
  MQ7.serialDebug(true);
}


void loop() {
  unsigned long startTime = millis();
  float sumHumidity = 0, sumTemperature = 0, sumPPMMQ135 = 0, sumPPMMQ7 = 0;
  int count = 0;

  while (millis() - startTime < 15000) { 
    // Reading from DHT sensor
    float humidity = dht.getHumidity();
    float temperature = dht.getTemperature();
    if (!isnan(humidity) && !isnan(temperature)) {
      sumHumidity += humidity;
      sumTemperature += temperature;
    }

    // Update and read from MQ135 sensor (NH4)
    MQ135.update();
    float ppmMQ135 = MQ135.readSensor();
    sumPPMMQ135 += ppmMQ135;

    // Update and read from MQ7 sensor (CO)
    MQ7.update();
    float ppmMQ7 = MQ7.readSensor();
    sumPPMMQ7 += ppmMQ7;

    count++;
    delay(1000); // Delay 1 second between readings
  }

  // Calculate the average readings
  float avgTemperature = sumTemperature / count;
  float avgPPMMQ135 = sumPPMMQ135 / count; // Average NH4
  float avgPPMMQ7 = sumPPMMQ7 / count; // Average CO

  // Display the average readings
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("NH4:");
  lcd.print(avgPPMMQ135, 1);
  lcd.print(" CO:");
  lcd.print(avgPPMMQ7, 1);

  lcd.setCursor(0, 1);
  lcd.print("Temp:");
  lcd.print(avgTemperature, 1);
  lcd.print("C");

  delay(10000); // Display averages for 10 seconds

  // Determine if the environment is harmful
  lcd.clear();
  lcd.setCursor(0, 0);
  if (avgPPMMQ7 > CO_THRESHOLD) {
    lcd.print("Harmful Air!");
  } else {
    lcd.print("Air is Safe"); 
  }

  delay(5000); // Display the safety message for 5 seconds
}