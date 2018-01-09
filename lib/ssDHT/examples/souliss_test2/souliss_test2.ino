
/**************************************************************************
    Souliss - Test 1

        Run this code on one of the following boards:
      - Arduino Ethernet (W5100)
      - Arduino with Ethernet Shield (W5100)

    As option you can run the same code on the following, just changing the
    relevant configuration file at begin of the sketch
      - Arduino with W5200 Ethernet Shield
      - Arduino with W5500 Ethernet Shield

      Souliss - Temperature and Humidity Measure

  The temperature and humidity are acquired from a DHT11/21/22 sensor
  and available at user interface as half-precision floating point.
  If any in-between calculation would be performed, the values shall
  be converted in single precision floating point and then back in
  half-precision.

  The Device has
    - An DHT11/21/22 sensor on PIN 2

***************************************************************************/

// Configure the framework
#include "bconf/StandardArduino.h"          // Use a standard Arduino
#include "conf/ethW5100.h"                  // Ethernet through Wiznet W5100
#include "conf/Gateway.h"                   // The main node is the Gateway, we have just one node
#include "conf/Webhook.h"                   // Enable DHCP and DNS

// Include framework code and libraries
#include "Souliss.h"
#include "Typicals.h"
#include "SPI.h"
#include "SoulissDHT.h"


/*** All configuration includes should be above this line ***/


// This identify the number of the LED logic
#define MYLEDLOGIC_1          0     //slot0             
#define MYLEDLOGIC_2          1     //slot1

// Sensor management
#define DHT_id1           1     // Identify the sensor, in case of more than one used on the same board
#define DHTPIN            2     //data pin for dht sensor
#define DHTTYPE           DHT22
#define TEMPERATURE       2     // This is the memory slot used for the execution of the logic in network_address1
#define HUMIDITY          4     // This is the memory slot used for the execution of the logic
#define DEADBAND       0.01     // Deadband value 1%


// Setup the DHT sensor
ssDHT22_Init(DHTPIN, DHT_id1);        // pin,sensor  (in case fo more than one sensors)


void setup()
{
  Initialize();
  Serial.begin(9600);      // open the serial port at 9600 bps:
  
   // Define the network configuration according to your router settings
  uint8_t ip_address[4]  = {192, 168, 1, 77};
  uint8_t subnet_mask[4] = {255, 255, 255, 0};
  uint8_t ip_gateway[4]  = {192, 168, 1, 1};

  Souliss_SetIPAddress(ip_address, subnet_mask, ip_gateway);  // Set network parameters
  SetAsGateway(myvNet_dhcp);                                  // Set this node as gateway for SoulissApp

  //Set typicals to use
  Set_SimpleLight(MYLEDLOGIC_1);
  Set_SimpleLight(MYLEDLOGIC_2);
  Set_Temperature(TEMPERATURE);
  Set_Humidity(HUMIDITY);
  ssDHT_Begin(DHT_id1);

  pinMode(4, INPUT);                  // Hardware pulldown required
  pinMode(3, INPUT);                  // Hardware pulldown required
  pinMode(9, OUTPUT);                 // Power the LED
  pinMode(8, OUTPUT);                 // Power the LED


}

void loop()
{
  // Here we start to play
  EXECUTEFAST() {
    UPDATEFAST();

    FAST_50ms() {   // We process the logic and relevant input and output every 50 milliseconds
      DigIn(3, Souliss_T1n_ToggleCmd, MYLEDLOGIC_1);            // Use the pin2 as ON/OFF toggle command
      Logic_SimpleLight(MYLEDLOGIC_1);                          // Drive the LED as per command
      DigOut(8, Souliss_T1n_Coil, MYLEDLOGIC_1);                // Use the pin9 to give power to the LED according to the logic
      DigIn(4, Souliss_T1n_ToggleCmd, MYLEDLOGIC_2);            // Use the pin3 as ON/OFF toggle command
      Logic_SimpleLight(MYLEDLOGIC_2);                          // Drive the LED as per command
      DigOut(9, Souliss_T1n_Coil, MYLEDLOGIC_2);               // Use the pin10 to give power to the LED according to the logic
    }

    FAST_2110ms() {
      Souliss_Logic_T52(memory_map, TEMPERATURE, DEADBAND, &data_changed);
      Souliss_Logic_T53(memory_map, HUMIDITY, DEADBAND, &data_changed);
    }
    
     FAST_GatewayComms();
  }

  EXECUTESLOW() {
    UPDATESLOW();

    SLOW_10s() {  // Read temperature and humidity from DHT every 10 seconds
      
      float temperature = ssDHT_readTemperature(DHT_id1);
      Souliss_ImportAnalog(memory_map, TEMPERATURE, &temperature);

      float humidity = ssDHT_readHumidity(DHT_id1);
      Souliss_ImportAnalog(memory_map, HUMIDITY, &humidity);

      Serial.println();
  Serial.print ("Temp 1 = ");
  Serial.print (temperature);
  Serial.println(" *C");
  Serial.print ("Hum = ");
  Serial.print (humidity);
  Serial.println(" %");

    }
  }
}


