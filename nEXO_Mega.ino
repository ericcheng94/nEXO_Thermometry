#include <Adafruit_MAX31865.h>
#include <Wire.h>
#include <SPI.h>
#include <Ethernet.h>
#include <Arduino.h>
#include <ArduinoRS485.h> // ArduinoModbus depends on the ArduinoRS485 library
#include <ArduinoModbus.h>

// Use software SPI: CS, DI, DO, CLK
//Adafruit_MAX31865 thermo = Adafruit_MAX31865(10, 11, 12, 13);
// use hardware SPI, just pass in the CS pin

Adafruit_MAX31865 rtd01 = Adafruit_MAX31865(53);
// Adafruit_MAX31865 rtd02 = Adafruit_MAX31865(49);
// Adafruit_MAX31865 rtd03 = Adafruit_MAX31865(47);
// Adafruit_MAX31865 rtd04 = Adafruit_MAX31865(45);
// Adafruit_MAX31865 rtd05 = Adafruit_MAX31865(43);

// Thermocouple ADC
Adafruit_MAX31856 tcp01 = Adafruit_MAX31856(41);
// Adafruit_MAX31856 tcp02 = Adafruit_MAX31856(39);

// The value of the Rref resistor. Use 430.0 for PT100 and 4300.0 for PT1000
#define RREF      430.0
// The 'nominal' 0-degrees-C resistance of the sensor
// 100.0 for PT100, 1000.0 for PT1000
#define RNOMINAL  100.0

// Enter a MAC address for your controller below.
byte mac[] = {0xA8, 0x61, 0x0A, 0xAE, 0x71, 0x17};
// The IP address will be dependent on your local network:
IPAddress ip(192, 168, 0, 242);

EthernetServer ethServer(502);
ModbusTCPServer modbusTCPServer;

unsigned long currentMillis;
unsigned long pollModbusMillis;
unsigned long updateSerialMillis;
unsigned long updateRTDMillis;
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
  Ethernet.init(10);  // Most Arduino shields

  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("nEXO ModbusTCP RTDTest!");
  rtd01.begin(MAX31865_3WIRE);  // set to 2WIRE or 4WIRE as necessary
  tcp01.begin();
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

  EthernetClient client = ethServer.available();
  // listen for incoming clients
  while (!client) {
    client = ethServer.available();
    Serial.println("Waiting for client...");
    if (client) {
      Serial.println("new client");
      modbusTCPServer.accept(client);
    }
    delay(1000);
  }
}

void loop() {
  currentMillis = millis();

  if (currentMillis - pollModbusMillis > 1000) {
    // poll for Modbus TCP requests, while client connected
    modbusTCPServer.poll();
    pollModbusMillis = currentMillis;

    if (heartbeat == 1) {
      modbusTCPServer.holdingRegisterWrite(tick_tock, 1);
      heartbeat = 0;
    } else {
      modbusTCPServer.holdingRegisterWrite(tick_tock, 0);
      heartbeat = 1;
    }
  }

  if (currentMillis - updateSerialMillis > 500) {
    updateSerial();
    updateSerialMillis = currentMillis;
  }

  if (currentMillis - updateRTDMillis > 200) {
    updateRTD();
    updateRTDMillis = currentMillis;
  }
}

void  updateSerial() {
  uint16_t rtd = rtd01.readRTD();
//  uint16_t rtd = thermo2.readRTD();
  Serial.print("RTD value: "); Serial.println(rtd);
  float ratio = rtd;
  ratio /= 32768;
  Serial.print("Ratio = "); Serial.println(ratio,8);
  Serial.print("Resistance = "); Serial.println(RREF*ratio,8);
  Serial.print("Temperature = "); Serial.println(rtd01.temperature(RNOMINAL, RREF));
//  Serial.print("Temperature = "); Serial.println(thermo2.temperature(RNOMINAL, RREF));
  Serial.println();
  Serial.print("TC Cold Junction: "); Serial.println(tcp01.readCJTemperature());
  Serial.print("TC Temperature: "); Serial.println(tcp01.readThermocoupleTemperature());

}

void updateRTD() {
  uint16_t rawRTD = rtd01.readRTD();
  // need half precision float 16bit
//  float16 ratio = rtd;
//  ratio /= 32768;
  modbusTCPServer.holdingRegisterWrite(rtd01_reg, rawRTD);
}
//
//void updateTemp2() {
//  uint16_t rtd = thermo2.readRTD();
//  // need half precision float 16bit
////  float16 ratio = rtd;
////  ratio /= 32768;
//  modbusTCPServer.holdingRegisterWrite(rtd02, rtd);
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
