#include <SPI.h>
#include "Adafruit_MAX31855.h"
#include <mcp2515.h>
#include <FIR.h>

//General setup
#define BaseCanID 0x600
#define UpdateRate 10 //Frontend thermocouple scan rate and CAN-message update rate in Hz.
#define serialDebug false

struct thermocoupleChannel{
   int channelNumber;
   Adafruit_MAX31855 thermocoupleFrontend;
   FIR<float, 10> fir_avg;
};

struct can_frame canMsg1;
enum CAN_SPEED BaudRate;

//Chip select pins
#define MAXCS1          6
#define MAXCS2          7
#define MAXCS3          8
#define MAXCS4          9
#define CANMODCS        10

//Input settings swithes
#define SW_CANID1       5
#define SW_CANID2       4
#define SW_CANBaudRate  3 
#define SW_Lowpass      2

MCP2515 mcp2515(CANMODCS);

thermocoupleChannel thermocoupleChannels[4]{
  {1, MAXCS1},
  {2, MAXCS2},
  {3, MAXCS3},
  {4, MAXCS4},
};

void setup() {

  Serial.begin(9600);

  while (!Serial) delay(1); // wait for Serial on Leonardo/Zero, etc

  Serial.println("Initializing sensors...");
  
  delay(500); // wait for MAX chip to stabilize
  for(byte i = 0; i < sizeof(thermocoupleChannels[i])/sizeof(thermocoupleChannel); i++){
    Serial.println("Init of channel: " + i);
    if (!thermocoupleChannels[i].thermocoupleFrontend.begin()) {
      Serial.println("ERROR. Sensor " + i);
      while (1) delay(10);
    }
  }
  Serial.println("DONE.");

  //Read setting switches
  //Remember that state switch reading is inverted in relation to physical position
  Serial.println("Reading settings switches...");
  pinMode(SW_CANID1, INPUT_PULLUP);
  pinMode(SW_CANID2, INPUT_PULLUP);
  pinMode(SW_CANBaudRate, INPUT_PULLUP);
  pinMode(SW_Lowpass, INPUT_PULLUP);

  byte canid_LSB = !digitalRead(SW_CANID1);
  Serial.print("canid_LSB: ");
  Serial.println(canid_LSB);
  byte canid_MSB = !digitalRead(SW_CANID2);
  Serial.print("canid_MSB: ");
  Serial.println(canid_MSB);
  int canid = canid_MSB << 1 | canid_LSB | BaseCanID;
  Serial.print("CANid: ");
  Serial.println(canid);

  //Read CAN baudrate switch and set baudrate accordingly
  //Switch state 0 = 250Kbs/s
  //Switch state 1 = 500Kbs/s
  Serial.print("SW_CANBaudRate: ");
  if (digitalRead(SW_CANBaudRate)) {
    BaudRate = CAN_250KBPS;
    Serial.println("250KBPS");
  }
  else {
    BaudRate = CAN_500KBPS;
    Serial.println("500KBPS");
  }
  
  byte Lowpass = !digitalRead(SW_Lowpass);
  Serial.print("SW_Lowpass: ");
  Serial.println(Lowpass);

  Serial.println("DONE.");

  canMsg1.can_id  = canid;
  canMsg1.can_dlc = 8;
  canMsg1.data[0] = 0x00;
  canMsg1.data[1] = 0x00;
  canMsg1.data[2] = 0x00;
  canMsg1.data[3] = 0x00;
  canMsg1.data[4] = 0x00;
  canMsg1.data[5] = 0x00;
  canMsg1.data[6] = 0x00;
  canMsg1.data[7] = 0x00;

  Serial.println("Initializing CAN module...");
  mcp2515.reset();
  mcp2515.setBitrate(BaudRate,MCP_8MHZ);
  mcp2515.setNormalMode();
  Serial.println("DONE.");
}

void loop() {
  int canMsgIndex=0;
  for(byte i = 0; i < sizeof(thermocoupleChannels)/sizeof(thermocoupleChannels[0]); i++){
    // basic readout test, just print the current temp
    if (serialDebug) {
      Serial.print("Internal Temp ");
      Serial.print(i+1);
      Serial.print(":");
      Serial.print(thermocoupleChannels[i].thermocoupleFrontend.readInternal());
      Serial.print(",");
    }

    double c = thermocoupleChannels[i].thermocoupleFrontend.readCelsius();
    if (isnan(c)) {
      if (serialDebug) Serial.print(i + " sensor fault(s) detected!");
      uint8_t e = thermocoupleChannels[i].thermocoupleFrontend.readError();
      if (e & MAX31855_FAULT_OPEN){
        c=0xFFFF;
        if (serialDebug) Serial.print("FAULT: Thermocouple is open - no connections.");
        canMsg1.data[canMsgIndex++] = int(c*10);
        canMsg1.data[canMsgIndex++] = int(c*10) >> 8;
      } 
      if (e & MAX31855_FAULT_SHORT_GND){
        c=0xFFFE;
        if (serialDebug) Serial.print("FAULT: Thermocouple is short-circuited to GND.");
        canMsg1.data[canMsgIndex++] = int(c*10);
        canMsg1.data[canMsgIndex++] = int(c*10) >> 8;
      } 
      if (e & MAX31855_FAULT_SHORT_VCC){
        c=0xFFFD;
        if (serialDebug) Serial.print("FAULT: Thermocouple is short-circuited to VCC.");
        canMsg1.data[canMsgIndex++] = int(c*10);
        canMsg1.data[canMsgIndex++] = int(c*10) >> 8;
      }
      if (serialDebug) Serial.print(",");
    } else {
      if (serialDebug) {
        Serial.print("Temp");
        Serial.print(i+1);
        Serial.print(":");
        Serial.print(int(c*10));
        Serial.print(",");
      }
      int test = 0xFF;
      test = test | int(c*10);
      canMsg1.data[canMsgIndex++] = int(c*10);
      canMsg1.data[canMsgIndex++] = int(c*10) >> 8;
    }
    //Serial.print("F = ");
    //Serial.println(thermocouple.readFahrenheit());
  }
  if (serialDebug) Serial.println();

  mcp2515.sendMessage(&canMsg1);
  //mcp2515.sendMessage(&canMsg2);
  delay(float(1)/UpdateRate*1000); //Crude wait, should use a dynamic time offset depending.
}