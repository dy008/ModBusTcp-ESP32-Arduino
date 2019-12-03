/*
  WiFi Modbus TCP Server LED

  This sketch creates a Modbus TCP Server with a simulated coil.
  The value of the simulated coil is set on the LED

  Circuit:
   - ESP32

  created 1 octember 2019
  by Bill Deng
*/
#include <Arduino.h>
#include <WiFi.h> // for ESP32

#include <ArduinoRS485.h> // ArduinoModbus depends on the ArduinoRS485 library
#include <ArduinoModbus.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

#define delayInMillis   50000   // for client break check time millis
#define TcpServerPort   502
#define TcpServerID   0xFF    // Modbus Device ID 0xFF for all

const int ledPin = LED_BUILTIN;
const int numCoils = 10;
const int numDiscreteInputs = 10;
const int numHoldingRegisters = 10;
const int numInputRegisters = 10;

#define MAX_SRV_CLIENTS 3  // Define the maximum number of clients
WiFiServer wifiServer(TcpServerPort);
WiFiClient serverClients[MAX_SRV_CLIENTS];
unsigned long lastTcpRequestTime[MAX_SRV_CLIENTS] ;

ModbusTCPServer modbusTCPServer;

void updateLED();

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(115200);

    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    //reset saved settings
    //wifiManager.resetSettings();
    wifiManager.setTimeout(60);
    //set custom ip for portal
    //wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
    //fetches ssid and pass from eeprom and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration
    //wifiManager.autoConnect("AutoConnectAP");
    //or use this for auto generated name ESP + ChipID
    wifiManager.autoConnect();  
    //if you get here you have connected to the WiFi
    //Serial.println("WiFi connected...yeey :)");
    //wifiManager.~WiFiManager();
    
  // start the server
  wifiServer.begin();
  wifiServer.setNoDelay(1); //关闭小包合并包功能，不会延时发送数据

  // start the Modbus TCP server id=0xFF
  if (!modbusTCPServer.begin(TcpServerID)) {
    Serial.println("Failed to start Modbus TCP Server!");
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
  }
  Serial.println("Modbus TCP Server start");

  // configure the LED
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  // configure coils at address 0x00
  modbusTCPServer.configureCoils(0x00, numCoils);

  // configure discrete inputs at address 0x00
  modbusTCPServer.configureDiscreteInputs(0x00, numDiscreteInputs);

  // configure holding registers at address 0x00
  modbusTCPServer.configureHoldingRegisters(0x00, numHoldingRegisters);

  // configure input registers at address 0x00
  modbusTCPServer.configureInputRegisters(0x00, numInputRegisters);
  
  for (uint8_t i = 0; i < MAX_SRV_CLIENTS; i++)
  {
    lastTcpRequestTime[i] = millis();
  }
}

void loop() {
  uint8_t coil = 0;
  uint16_t holdreg = 0;
  uint8_t i = 0;

  // listen for incoming clients
  if (wifiServer.hasClient()) {  // check for client coming
    for (i = 0; i < MAX_SRV_CLIENTS; i++) {      
      // release old or invald client
      if (!serverClients[i] || !serverClients[i].connected()) {
        if (serverClients[i]) {
          // check for vald client
          serverClients[i].stop(); 
          Serial.print("Break Client: "); 
          Serial.println(i);
        }        
        serverClients[i] = wifiServer.available();  // allocation new client
        Serial.print("New Client: "); 
        Serial.println(i);
        lastTcpRequestTime[i] = millis(); 
        break;
      }
    }    
    // if clients more MAX_SRV_CLIENTS do refuse the connection
    if (i >= MAX_SRV_CLIENTS) {
      WiFiClient client = wifiServer.available();
      client.stop();
      Serial.println("No Free Client ");
    }
  }
  
  // do clients data
  for (i = 0; i < MAX_SRV_CLIENTS; i++) {
    if (serverClients[i] && serverClients[i].connected()) {
      // if client vald and connected
      if (millis() - lastTcpRequestTime[i] < delayInMillis){
        // if client is ontime
        if (serverClients[i].available()) {
          lastTcpRequestTime[i] = millis();
          // let the Modbus TCP accept the connection 
          modbusTCPServer.accept(serverClients[i]);

          // check more data
          while (serverClients[i].available()) {
            modbusTCPServer.poll();
            // for test
            coil = modbusTCPServer.coilRead(0);
            modbusTCPServer.coilWrite(0,!coil);
            holdreg = modbusTCPServer.holdingRegisterRead(0) ;
            modbusTCPServer.holdingRegisterWrite(0,++holdreg);
            Serial.printf("%d",coil);
            Serial.printf("   %d\n",holdreg);
            updateLED();          
            }
        }
      }
      else {
        serverClients[i].stop();  // the client is break
        Serial.print("Stop Client: "); 
        Serial.println(i);
      }
    }
  }
}

void updateLED() {
  // read the current value of the coil
  int coilValue = modbusTCPServer.coilRead(0x00);

  if (coilValue) {
    // coil value set, turn LED on
    digitalWrite(ledPin, HIGH);
  } else {
    // coild value clear, turn LED off
    digitalWrite(ledPin, LOW);
  }
}
