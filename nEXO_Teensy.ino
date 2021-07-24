#include <Adafruit_MAX31865.h>
#include <Adafruit_MAX31856.h>
#include <Wire.h>
#include <SPI.h>
#include <NativeEthernet.h>
#include <Arduino.h>
#include <ArduinoRS485.h> // ArduinoModbus depends on the ArduinoRS485 library
#include <ArduinoModbus.h>

//#define PIN_CS 39
//const byte charTable[10] = {
//    B01111110, B00110000, B01101101, B01111001, B00110011, B01011011, B01011111, B01110000, B01111111, B01111011};
//const byte ledSeg[8] = {
//    B00001000, B00000111, B00000110, B00000101, B00000100, B00000011, B00000010, B00000001 
//};
//#define DECODEMODE_ADDR (0x09)
//#define BRIGHTNESS_ADDR (0x0A)
//#define SCANLIMIT_ADDR (0x0B)
//#define SHUTDOWN_ADDR (0X0C)
//#define DISPLAYTEST_ADDR (0x0F)
//#define OP_OFF (0x0)
//#define OP_ON (0x1)
//#define BTN_1 37 // Display Push button 1
//#define BTN_2 35 // Display Push button 2

Adafruit_MAX31865 rtd01 = Adafruit_MAX31865(10);
 Adafruit_MAX31865 rtd02 = Adafruit_MAX31865(9);
// Adafruit_MAX31865 rtd03 = Adafruit_MAX31865(47);
// Adafruit_MAX31865 rtd04 = Adafruit_MAX31865(45);
// Adafruit_MAX31865 rtd05 = Adafruit_MAX31865(43);

// Thermocouple ADC
Adafruit_MAX31856 tcp01 = Adafruit_MAX31856(8);
//Adafruit_MAX31856 tcp01 = Adafruit_MAX31856(41);
// Adafruit_MAX31856 tcp02 = Adafruit_MAX31856(39);

// The value of the Rref resistor. Use 430.0 for PT100 and 4300.0 for PT1000
#define RREF      430.0
// The 'nominal' 0-degrees-C resistance of the sensor
// 100.0 for PT100, 1000.0 for PT1000
#define RNOMINAL  100.0

// Enter a MAC address for your controller below.
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
// The IP address will be dependent on your local network:
IPAddress ip(192, 168, 0, 242);

EthernetServer ethServer(502);
ModbusTCPServer modbusTCPServer;

unsigned long currentMillis;
unsigned long pollModbusMillis;
unsigned long updateSerialMillis;
unsigned long updateRTDMillis;
unsigned long updateDisplayMillis;
unsigned long countMillis;
int           heartbeat;

//Modbus holding register address mapping
const int rtd01_reg = 0x00;
const int rtd02_reg = 0x01;
const int rtd03_reg = 0x02;
const int rtd04_reg = 0x03;
const int rtd05_reg = 0x04;
const int tcp01_reg = 0x05;
const int tcp02_reg = 0x06;
const int tick_tock = 0x07;

// const int ledPin = LED_BUILTIN;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("nEXO ModbusTCP RTDTest!");
  rtd01.begin(MAX31865_3WIRE);  // set to 2WIRE or 4WIRE as necessary
  rtd02.begin(MAX31865_3WIRE);  // set to 2WIRE or 4WIRE as necessary
  tcp01.begin();
//  Serial.print("TC Type: "); Serial.println(tcp01.getThermocoupleType());
  tcp01.setThermocoupleType(MAX31856_TCTYPE_T);

  Ethernet.begin(mac, ip);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  // Keeps displaying error despite having thernet connection, commented out for now
  // if (Ethernet.linkStatus() == LinkOFF) {
  //   Serial.println("Ethernet cable is not connected.");
  // }

  // start the server
  ethServer.begin();

  // start the Modbus TCP server
  if (!modbusTCPServer.begin()) {
    Serial.println("Failed to start Modbus TCP Server!");
    while (1);
  }

  // configure 7 holding registers at address 0x00
  modbusTCPServer.configureHoldingRegisters(0x00, 8);

//  pinMode(PIN_CS, OUTPUT);
//  pinMode(BTN_1, INPUT);
//  pinMode(BTN_2, INPUT);
//  digitalWrite(PIN_CS, HIGH);
//  SPI.begin(); 
//  //SPI.beginTransaction(SPISettings(SPIMAXSPEED, MSBFIRST, SPI_MODE0));  
//  spiWrite(DISPLAYTEST_ADDR, OP_OFF);
//  //we go into shutdown-mode on startup
//  spiWrite(SHUTDOWN_ADDR, OP_OFF);
//  //scanlimit is set to max on startup
//  spiWrite(SCANLIMIT_ADDR, 7); // show 8 digits
//  //decode is done in raw mode
//  spiWrite(DECODEMODE_ADDR, 0);
//  //clearSegments
//  spiWrite(SHUTDOWN_ADDR,OP_ON);
//  // set brightness
//  spiWrite(BRIGHTNESS_ADDR, 5);
}

void loop() {
  EthernetClient client = ethServer.available();

//  modbusTCPServer.accept(client);
  // listen for incoming clients
  if (client) {
     Serial.println("new client");
     modbusTCPServer.accept(client);
  
     while (client.connected()) {
        currentMillis = millis();
        
        if (currentMillis - pollModbusMillis > 1000) {
          // poll for Modbus TCP requests, while client connected
  //        Serial.println("polling Modbus");
            modbusTCPServer.poll();
            pollModbusMillis = currentMillis;
  
  //      if (heartbeat == 1) {
  //        modbusTCPServer.holdingRegisterWrite(tick_tock, 1);
  //        heartbeat = 0;
  //      } else {
  //        modbusTCPServer.holdingRegisterWrite(tick_tock, 0);
  //        heartbeat = 1;
  //      }
        }     
        if (currentMillis - updateSerialMillis > 1000) {
//          Serial.println("updating Serial");
          updateSerial();
          updateSerialMillis = currentMillis;
        }
//        if (currentMillis - updateRTDMillis > 1000) {
//          clearALL();
//          updateRTD(rtd01);
//          updateRTDMillis = currentMillis;
//        }
//        if (currentMillis - updateDisplayMillis > 1000) {
//          updateDisplay();
//          updateDisplayMillis = currentMillis;
        } 
        //  if (currentMillis - countMillis > 100) {
        //    Serial.println("millis");
        //    countMillis = currentMillis;
        //  }
        //  if (digitalRead(BTN_1)) {
        //    writeV1(555.5);
        //  } else {
        //    writeV1(99.9);
        //  }
        //  if (digitalRead(BTN_2)) {
        //    writeSensor("rtd");
        //  } else {
        //    writeSensor("tcp");
        //  }
     }
  // backup for if client disconnects, display still runs
//  updateDisplay();
  delay(1000);
}


void  updateSerial() {
  uint16_t rtd = rtd01.readRTD();
  Serial.print("RTD value: "); Serial.println(rtd);
  float ratio = rtd;
  ratio /= 32768;
  Serial.print("Ratio = "); Serial.println(ratio,8);
  Serial.print("Resistance = "); Serial.println(RREF*ratio,8);
  Serial.print("Temperature = "); Serial.println(rtd01.temperature(RNOMINAL, RREF));
  Serial.print("Temperature = "); Serial.println(rtd02.temperature(RNOMINAL, RREF));
  Serial.println();
  Serial.print("TC Cold Junction: "); Serial.println(tcp01.readCJTemperature());
  Serial.print("TC Temperature: "); Serial.println(tcp01.readThermocoupleTemperature());
//  Serial.print("TC Fault: "); Serial.println(tcp01.readFault());
  Serial.println();
}

//void updateRTD(Adafruit_MAX31865 rtd) {
//  uint16_t rawRTD = rtd.readRTD();
//  float temp = rtd.temperature(RNOMINAL, RREF);
//  temp = 988.7;
//  // need half precision float 16bit
////  float16 ratio = rtd;
////  ratio /= 32768;
//  modbusTCPServer.holdingRegisterWrite(rtd01_reg, rawRTD);
//  Serial.print("Temperature = "); Serial.println(rtd01.temperature(RNOMINAL, RREF));
////  writeV1(temp);
//}

//void updateDisplay() {
//    writeV1(23.4);
//
//}

//void writeV1(float val) {
//  // if > 999.9 show something else
//  // Get 100's place
//  int v100 = (int) val / 100;
//  int v10  = ((int) val % 100) /10;
//  int v1   = ((int) val ) % 10;
//  int v01  = ((int) (val*10) ) %10;
//  Serial.print(v100);
//  Serial.print(v10);
//  Serial.print(v1);
//  Serial.print(".");
//  Serial.println(v01);
////  if (v100 == 0){
////    spiWrite(ledSeg[0], B00000000);
////  }
////  else {
////    spiWrite(ledSeg[0], charTable[v100]);
////  }
////  // Write 10s place
////  spiWrite(ledSeg[1], charTable[v10]);
////  // Write 1s place
////  spiWrite(ledSeg[2], B10000000+charTable[v1]);
////  // Write 0.1s place
////  spiWrite(ledSeg[3], charTable[v01]);
////
////  spiWrite(B00000001, B11111111);
//
//  if (v100 == 0){
//    spiWrite(ledSeg[4], B00000000);
//  }
//  else {
//    spiWrite(ledSeg[4], charTable[v100]);
//  }
//  // Write 10s place
//  spiWrite(ledSeg[5], charTable[v10]);
//  // Write 1s place
//  spiWrite(ledSeg[6], B10000000+charTable[v1]);
//  // Write 0.1s place
//  spiWrite(ledSeg[7], charTable[v01]);
//}
//void writeSensor(String sensor) {
//  if (sensor == "rtd") {
//    spiWrite(ledSeg[0], B00000101);
//    spiWrite(ledSeg[1], B00001111);
//    spiWrite(ledSeg[2], B00111101);
//  } else if (sensor == "tcp") {
//    spiWrite(ledSeg[0], B00001111);
//    spiWrite(ledSeg[1], B01001110);
//    spiWrite(ledSeg[2], B01100111);
//  }
//}
//void clearALL() {
//  spiWrite(ledSeg[0], B00000000);
//  spiWrite(ledSeg[1], B00000000);
//  spiWrite(ledSeg[2], B00000000);
//  spiWrite(ledSeg[3], B00000000);
//  spiWrite(ledSeg[4], B00000000);
//  spiWrite(ledSeg[5], B00000000);
//  spiWrite(ledSeg[6], B00000000);
//  spiWrite(ledSeg[7], B00000000);
//}
//// Taken from Examples->SPI->BarometricPressureSensor
//void spiWrite(byte thisRegister, byte thisValue) {
//  // SCP1000 expects the register address in the upper 6 bits
//  // of the byte. So shift the bits left by two bits:
//  //thisRegister = thisRegister << 2;
//  // now combine the register address and the command into one byte:
//  //byte dataToSend = thisRegister | WRITE;
//  // take the chip select low to select the device:
//  digitalWrite(PIN_CS, LOW);
//  SPI.transfer(thisRegister); //Send register location
//  SPI.transfer(thisValue);  //Send value to record into register
//  // take the chip select high to de-select:
//  digitalWrite(PIN_CS, HIGH);
//}

void checkPTFault() {
  // Check and print any faults
  uint8_t fault = rtd01.readFault();
  if (fault) {
    Serial.print("Fault 0x"); Serial.println(fault, HEX);
    if (fault & MAX31865_FAULT_HIGHTHRESH) {
      Serial.println("RTD High Threshold");
    }
    if (fault & MAX31865_FAULT_LOWTHRESH) {
      Serial.println("RTD Low Threshold");
    }
    if (fault & MAX31865_FAULT_REFINLOW) {
      Serial.println("REFIN- > 0.85 x Bias");
    }
    if (fault & MAX31865_FAULT_REFINHIGH) {
      Serial.println("REFIN- < 0.85 x Bias - FORCE- open");
    }
    if (fault & MAX31865_FAULT_RTDINLOW) {
      Serial.println("RTDIN- < 0.85 x Bias - FORCE- open");
    }
    if (fault & MAX31865_FAULT_OVUV) {
      Serial.println("Under/Over voltage");
    }
    rtd01.clearFault();
  }
}
