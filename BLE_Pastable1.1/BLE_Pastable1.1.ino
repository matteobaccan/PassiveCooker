// Pastable Termo  ver 1.1

/*
MIT License

Copyright (c) 2022 [INSERIRE NOME]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

//Feature of the code:
//   notify status (Heating/Boiling) and average temperature
//   trigger Boiling when avgTemp > 65 and stdv < 3
//   Fixed sampling every 5 sec
//   Auto shutdown when connected to a smartphone after 250 readings (about 20 minutes)
//   and under 30 degrees, or 10 minutes inactive.(BLE not connected)

#include <ArduinoBLE.h>
#include <RCircularBuffer.h>
#include <math.h>  //loads the more advanced math functions
RCircularBuffer<float, 6> buffer;

#include "QuickStats.h"
// float average;
float lettura;
int numreadings = 6;
float readings[] = {18.00, 18.00, 18.00, 18.00, 18.00, 18.00};

QuickStats stats;  // initialize an instance of this class

/*
  Analog Input
*/
// Thermistor Example #3 from the Adafruit Learning System guide on Thermistors
// https://learn.adafruit.com/thermistor/overview by Limor Fried, Adafruit Industries
// MIT License - please keep attribution and consider buying parts from Adafruit

// which analog pin A7 to connect
#define THERMISTORPIN 7
                   // resistance at 25 degrees C
#define THERMISTORNOMINAL 100000
                   // temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25
                   // how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 20
                   // The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3950
                   // the value of the 'other' resistor
#define SERIESRESISTOR 100000

int samples[NUMSAMPLES];

int row_excel = 0;       // number of lines
int test = 123;          // test variable to be passed to Excel
int test_2 = 456;        // the second test variable that will be passed to excel
int shutdown_cycle = 0;  // reset number of cycles

BLEService boilService("1101");  // Declare service for Gyroscope
BLEStringCharacteristic boilValuesChar("2101", BLERead | BLENotify, 100);
BLEStringCharacteristic statusValuesChar("3101", BLERead | BLENotify, 100);
void setup() {
  Serial.begin(9600);
  Serial.println("CLEARDATA");                                             // clear excel sheet
  Serial.println("LABEL,Time,Temp, Resistance, Stdv, Average, Num Rows");  // column headers

  pinMode(LED_BUILTIN, OUTPUT);  // Intializes the built in LED to indicate when a central device has connected
  digitalWrite(LED_PWR, HIGH);   // turn on power LED
  delay(1000);
  digitalWrite(LED_PWR, LOW);  // turn off power LED
  if (!BLE.begin()) {
    Serial.println("BLE failed");
    while (1)
      ;
  }

  BLE.setLocalName("Termo");
  BLE.setAdvertisedService(boilService);
  boilService.addCharacteristic(boilValuesChar);    // Adds the gryoscope characteristics
  boilService.addCharacteristic(statusValuesChar);  // Adds the gryoscope characteristics
  BLE.addService(boilService);                      // Adds the Thermo service
  BLE.setAppearance(384);                           // BLE_APPEARANCE_GENERIC_REMOTE_CONTROL
  BLE.advertise();                                  // Starts advertising the peripheral device over bluetooth
  Serial.println("Waiting for connection..");
}
double temp;  // Variable to hold a temperature value
String Level_String;
String Status_String;
String Avg_String;

void loop() {
  static unsigned long previousMillis = 0;
  int SENSOR_UPDATE_INTERVALL = 5000;  // Sampling reading sensor rate every 5 sec
  int SHUTDOWN_UPDATE_INTERVALL = 60000;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis > SHUTDOWN_UPDATE_INTERVALL) {
    shutdown_cycle++;                // cycle number + 1
    previousMillis = currentMillis;  // reset counter
  }
  // if cycle are more than 10, then start shutdown after 10 minutes
  if (shutdown_cycle > 10) {
    NRF_POWER->SYSTEMOFF = 1;  // System OFF
  }

  // #define SENSOR_UPDATE_INTERVALL 15000  // measure every 15 secs

  BLEDevice central = BLE.central();  // Waits for BLE Central device to connect

  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());
    digitalWrite(LED_BUILTIN, HIGH);  // Turn on peripheral LED to indicate valid connection with Central Device
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
    row_excel = 0;               // Reset counter
    shutdown_cycle = 0;          // reset number of cycles
    while (central.connected())  // While the Peripheral Device is connected to the Central Device
    {
      if (stats.average(readings, numreadings) > 40) {
        SENSOR_UPDATE_INTERVALL = 5000;  // measure every 5 secs
      } else
        SENSOR_UPDATE_INTERVALL = 5000;  // measure every 5 secs (or decide other)
      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis > SENSOR_UPDATE_INTERVALL) {
        previousMillis = currentMillis;  // reset counter
        uint8_t i;
        float average;

        // take N samples in a row, with a slight delay
        for (i = 0; i < NUMSAMPLES; i++) {
          samples[i] = analogRead(THERMISTORPIN);
          delay(10);
        }

        // average all the samples out
        average = 0;
        for (i = 0; i < NUMSAMPLES; i++) {
          average += samples[i];
        }
        average /= NUMSAMPLES;

        Serial.print("Average analog reading ");
        Serial.println(average);

        // convert the value to resistance
        average = 1023 / average - 1;
        average = SERIESRESISTOR / average;
        Serial.print("Thermistor resistance ");
        Serial.println(average);

        float steinhart;
        steinhart = average / THERMISTORNOMINAL;           // (R/Ro)
        steinhart = log(steinhart);                        // ln(R/Ro)
        steinhart /= BCOEFFICIENT;                         // 1/B * ln(R/Ro)
        steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15);  // + (1/To)
        steinhart = 1.0 / steinhart;                       // Invert
        steinhart -= 273.15;                               // convert to C

        Serial.print("Temperature ");
        Serial.print(steinhart);
        Serial.println(" *C");
        temp = steinhart;

        float aleatory = random(1, 100);  // generate an aleatory number (only for testing).

        Level_String = String(temp, 3);

        buffer.push(temp);  // float value
                            // the following ensures using the right type for the index variable
        using index_t = decltype(buffer)::index_t;
        for (index_t i = 0; i < buffer.size(); i++) {
          readings[i] = buffer[i];  // copy element at location i to 0
        }
        Serial.print("Sample Interval: ");
        Serial.println(SENSOR_UPDATE_INTERVALL);
        Serial.print("shutdown cycle: ");
        Serial.println(shutdown_cycle);
        Serial.print("row counter: ");
        Serial.println(row_excel);
        Serial.println("Descriptive Statistics");
        Serial.print("Average: ");
        Serial.println(stats.average(readings, numreadings));
        Serial.print("Geometric mean: ");
        Serial.println(stats.g_average(readings, numreadings));
        Serial.print("Minimum: ");
        Serial.println(stats.minimum(readings, numreadings));
        Serial.print("Maximum: ");
        Serial.println(stats.maximum(readings, numreadings));
        Serial.print("Standard Deviation: ");
        Serial.println(stats.stdev(readings, numreadings));
        Serial.print("Standard Error: ");
        Serial.println(stats.stderror(readings, numreadings));
        Serial.print("Coefficient of Variation (%): ");
        Serial.println(stats.CV(readings, numreadings));

        // evaluate boiling curve
        if (stats.average(readings, numreadings) > 65 && stats.stdev(readings, numreadings) < 3) {
          Status_String = "Boiling";
          row_excel = 130;  // need to partially reset counter once reach Boliling level and start to shutdown
                            // when temperature low again and stop after 10 minutes 250-130 = 120x5sec (10 minutes)
        } else
          Status_String = "Heating";

        Serial.println(Level_String);
        statusValuesChar.writeValue(Status_String);
        Avg_String = String(stats.average(readings, numreadings), 2);
        boilValuesChar.writeValue(Avg_String);

        row_excel++;                 // row number + 1
        Serial.print("DATA,TIME,");  // excel record current date and time
        Serial.print(Level_String);
        Serial.print(",");
        Serial.print(average);
        Serial.print(",");
        Serial.print(stats.stdev(readings, numreadings));
        Serial.print(",");
        Serial.print(stats.average(readings, numreadings));
        Serial.print(",");
        Serial.print(stats.g_average(readings, numreadings));
        Serial.print(",");
        Serial.println(row_excel);
        // if row counter are more than 250 (readings every 5 seconds) and average temp is less 30 degree stable, then shutdown
        if (row_excel > 250 && stats.average(readings, numreadings) < 30 && stats.stdev(readings, numreadings) < 1) {
          NRF_POWER->SYSTEMOFF = 1;  // System OFF
          row_excel = 0;
          Serial.println("ROW,SET,2");
        }
      }
    }
  }
  digitalWrite(LED_BUILTIN, LOW);
  Serial.print("Disconnected from central: ");
  Serial.println(central.address());
}
