#include <SPI.h>
#include "Adafruit_MAX31855.h"


#define MAXCS1   6
Adafruit_MAX31855 thermocouple1(MAXCS1);
#define MAXCS2   7
Adafruit_MAX31855 thermocouple2(MAXCS2);
#define MAXCS3   8
Adafruit_MAX31855 thermocouple3(MAXCS3);
#define MAXCS4   9
Adafruit_MAX31855 thermocouple4(MAXCS4);

typedef struct record_type
{
int one;
int two;
int three;
Adafruit_MAX31855 thermocouple5;
};

Adafruit_MAX31855 thermocouple_channels[4] = {MAXCS1,MAXCS2,MAXCS3,MAXCS4};

void setup() {
  //pinMode(6, OUTPUT); //Chip Select Thermo 2
  //pinMode(7, OUTPUT); //Chip Select Thermo 3
  //pinMode(9, OUTPUT); //Chip Select Thermo 4
  pinMode(10, OUTPUT); //Chip Select CAN module

  //digitalWrite(6, HIGH);   //Disable Thermo 2
  //digitalWrite(7, HIGH);   //Disable Thermo 3
  //digitalWrite(9, HIGH);   //Disable Thermo 4
  digitalWrite(10, HIGH);   //Disable CAN module

  Serial.begin(9600);

  while (!Serial) delay(1); // wait for Serial on Leonardo/Zero, etc

  Serial.println("MAX31855 test");
  Serial.println("Initializing sensors...");
  delay(500);
  for(byte i = 0; i < sizeof(thermocouple_channels)/sizeof(Adafruit_MAX31855) - 1; i++){
    // wait for MAX chip to stabilize
    
    Serial.println(sizeof(thermocouple_channels));
    if (!thermocouple_channels[i].begin()) {
      Serial.println("ERROR. Sensor " + i);
      while (1) delay(10);
    }
  }

  // OPTIONAL: Can configure fault checks as desired (default is ALL)
  // Multiple checks can be logically OR'd together.
  // thermocouple.setFaultChecks(MAX31855_FAULT_OPEN | MAX31855_FAULT_SHORT_VCC);  // short to GND fault is ignored

  Serial.println("DONE.");
}

void loop() {
  for(byte i = 0; i < sizeof(thermocouple_channels)/sizeof(Adafruit_MAX31855) - 1; i++){
    // basic readout test, just print the current temp
    Serial.print("Internal Temp ");
    Serial.print(i+1);
    Serial.print(":");
    Serial.print(thermocouple_channels[i].readInternal());
    Serial.print(",");

    double c = thermocouple_channels[i].readCelsius();
    if (isnan(c)) {
      Serial.print(i + " sensor fault(s) detected!");
      uint8_t e = thermocouple_channels[i].readError();
      if (e & MAX31855_FAULT_OPEN) Serial.print("FAULT: Thermocouple is open - no connections.");
      if (e & MAX31855_FAULT_SHORT_GND) Serial.print("FAULT: Thermocouple is short-circuited to GND.");
      if (e & MAX31855_FAULT_SHORT_VCC) Serial.print("FAULT: Thermocouple is short-circuited to VCC.");
      Serial.print(",");
    } else {
      Serial.print("Temp");
      Serial.print(i+1);
      Serial.print(":");
      Serial.print(c);
      Serial.print(",");
    }
    //Serial.print("F = ");
    //Serial.println(thermocouple.readFahrenheit());
  }
  Serial.println();
  delay(100);
}