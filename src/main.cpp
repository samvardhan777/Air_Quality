#include "DHT.h"
#include <MQ135.h>
#include <MQUnifiedsensor.h>

// Definitions for MQ sensors and DHT
#define BOARD "Arduino UNO"
#define VOLTAGE_RESOLUTION 5
#define ADC_BIT_RESOLUTION 10
#define RATIO_MQ7_CLEAN_AIR 27.5 // RS/R0 ratio in clean air for MQ-7
#define RATIO_MQ135_CLEAN_AIR 3.6 // RS/R0 ratio in clean air for MQ-135

// Pin definitions
#define DHT_PIN 2 // DHT sensor pin
#define MQ7_PIN A4 // Analog input for MQ7
#define MQ135_PIN A0 // Analog input for MQ135
#define PWM_PIN 5 // PWM control pin for MQ7 heating

// Sensor Objects
DHT dht;
MQUnifiedsensor MQ7(BOARD, VOLTAGE_RESOLUTION, ADC_BIT_RESOLUTION, MQ7_PIN, "MQ-7");
MQUnifiedsensor MQ135(BOARD, VOLTAGE_RESOLUTION, ADC_BIT_RESOLUTION, MQ135_PIN, "MQ-135");

void setup() {
  Serial.begin(9600);
  dht.setup(2); // data pin 2

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
  // Reading from DHT sensor
  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();
  
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Update and read from MQ135 sensor
  MQ135.update(); 
  float ppmMQ135 = MQ135.readSensor(); // Read concentration from MQ135

  // Update and read from MQ7 sensor
  MQ7.update(); 
  float ppmMQ7 = MQ7.readSensor(); // Read CO concentration from MQ7

  // Print DHT sensor readings
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print("%  Temperature: ");
  Serial.print(temperature);
  Serial.println("°C");

  // Print MQ135 sensor readings
  Serial.print("MQ135 PPM: ");
  Serial.println(ppmMQ135);

  // Print MQ7 sensor readings
  Serial.print("MQ7 CO PPM: ");
  Serial.println(ppmMQ7);

  // Basic inference logic based on sensor readings
  if (ppmMQ135 > 1000 || ppmMQ7 > 50) {
    Serial.println("Warning: High levels of pollutants detected!");
  }

  // Wait some time before next loop iteration
  delay(1000);
}