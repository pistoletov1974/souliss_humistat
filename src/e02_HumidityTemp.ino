/**************************************************************************
    Souliss - DHTxx
    
    Sensor
      - DHTxx
      // Connect pin 1 (on the left) of the sensor to +5V
      // NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
      // to 3.3V instead of 5V!
      // Connect pin 2 of the sensor to whatever your DHTPIN is
      // Connect pin 4 (on the right) of the sensor to GROUND
      // Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor      
     
    Run this code on one of the following boards:
      - Arduino Ethernet (W5100) 
      - Arduino with Ethernet Shield (W5100)
      
    As option you can run the same code on the following, just changing the
    relevant configuration file at begin of the sketch
      - Arduino with ENC28J60 Ethernet Shield
      - Arduino with W5200 Ethernet Shield
      - Arduino with W5500 Ethernet Shield 
      
***************************************************************************/

// Let the IDE point to the Souliss framework
#include "SoulissFramework.h"

// Configure the framework
#include "bconf/StandardArduino.h" // Use a standard Arduino
#include "conf/ethW5100.h"		   // Ethernet through Wiznet W5100
#include "conf/Gateway.h"		   // The main node is the Gateway, we have just one node
#include "conf/SmallNetwork.h"
#include "user/float16.h"
// Enable DHCP and DNS
#include "Souliss.h"
// Include framework code and libraries
#include <SPI.h>
//#include <DHT.h>
#include <SoulissDHT.h>


//debug mode
//#define SERIALPORT_INSKETCH
//#define LOG Serial

// Include sensor libraries (from Adafruit) Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22 // DHT 22 (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
#define DHTPIN            2     //data pin for dht sensor
#define DHTTYPE           DHT22
#define LIGHTPIN          5
#define LIGHT_TIME        20000UL // time before fan goes on in millis
////#define DHTPIN 2 // what digital pin we're connected to

#define DHT_id1   1 
// Setup a DHT22 instan
//dht DHT;

/*** All configuration includes should be above this line ***/


#define HUMIDITY 0 // Leave 2 slots for T53
#define TEMP0 2	// Leave 2 slots for T52
#define FAN_LOW 4
#define FAN_HIGH 5
#define LIGHT 6


// DHT sensor
// DHT dht(DHTPIN, DHTTYPE); // for ESP8266 use dht(DHTPIN, DHTTYPE, 11)
//ssDHT22_Init(DHTPIN, DHT_id1);
ssDHT ssDHT1(2, DHT22);
uint8_t ip_address[4] = {192, 168, 0, 78};
uint8_t subnet_mask[4] = {255, 255, 255, 0};
uint8_t ip_gateway[4] = {192, 168, 0, 1};
uint8_t hour = 23;
uint8_t light_state=0, light_prev=0;
unsigned long light_on_millis;
enum states
{
	FAN_OFF,
	FAN_ON_HUMI,
	FAN_ON_LIGHT
};
states fan_state = FAN_OFF;


float humidity = 0;
float temperature;
float humidity_prev = 0;
#define myvNet_address ip_address[3] // The last byte of the IP address (77) is also the vNet address
#define myvNet_subnet 0xFF00
EthernetUDP Udp;
char timeServer[] = "time.nist.gov"; // time.nist.gov NTP server

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

void setup()
{
	Initialize();

	// Get the IP address from DHCP
	Souliss_SetIPAddress(ip_address, subnet_mask, ip_gateway);

	SetAsGateway(myvNet_address); // Set this node as gateway for SoulissApp
	Udp.begin(8888);
	Serial.begin(9600);
	ssDHT_Begin(DHT_id1);

	pinMode(9, OUTPUT);
	pinMode(4, OUTPUT);
	pinMode(LIGHTPIN, INPUT);
	Set_Humidity(HUMIDITY);
	Set_Temperature(TEMP0);
	Set_SimpleLight(FAN_HIGH);
	Set_SimpleLight(FAN_LOW);
	Set_DigitalInput(LIGHT);


	// init time request

	sendNTPpacket(timeServer);
	Serial.println("packet sent");
	delay(3000);
	if (Udp.parsePacket())
	{
		Udp.read(packetBuffer, NTP_PACKET_SIZE);
		unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
		unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
		unsigned long secsSince1900 = highWord << 16 | lowWord;
		Serial.print("Seconds since Jan 1 1900 = ");
		Serial.println(secsSince1900);
		Serial.print("Unix time = ");
		const unsigned long seventyYears = 2208988800UL;
		unsigned long epoch = secsSince1900 - seventyYears;
		Serial.println(epoch);
		Serial.print("The houre is ");
		hour = (epoch % 86400L) / 3600 + 3;
		Serial.println(hour);
	}
}

void loop()
{

	// Here we start to play
	EXECUTEFAST()
	{
		UPDATEFAST();

		// Execute the code every 1 time_base_fast
		FAST_10ms()
		{

			// Just process communication as fast as the logics
			ProcessCommunication();
			Logic_SimpleLight(FAN_HIGH);
			Logic_SimpleLight(FAN_LOW);
			DigOut(4, Souliss_T1n_Coil, FAN_LOW);
			DigOut(9, Souliss_T1n_Coil, FAN_HIGH);
		}

		FAST_50ms()
		{

			
               if ((digitalRead(LIGHTPIN==HIGH)&&light_prev==LOW )) {
                    // light going on we need to start counter
                   light_on_millis = millis();
                   light_state = 1;
				   Serial.print("Light ON front");
               }
                 
               if  ( (digitalRead(LIGHTPIN)==HIGH) && ((millis()-light_on_millis)>LIGHT_TIME ) && (light_state==1) )
                              {
                                   Serial.println("light 0n send command");
                                   light_state=0;
                                   if (fan_state!=FAN_ON_HUMI) {
                                        if (hour>7 && hour <23) 
                                          mInput(FAN_HIGH)=0x30+6*5;
                                            else mInput(FAN_LOW) = 0x30+ 6*5;
                                   }

                              }


               light_prev==digitalRead(LIGHTPIN);

              /*
               if (fan_state != FAN_ON_HUMI)
			{

				if (hour > 7 && hour < 23)
				{
					DigInHoldCustom(5, Souliss_T1n_OffCmd, 0x30 + 6 * 5, FAN_HIGH, 30000);
				}

				else
					DigInHoldCustom(5, Souliss_T1n_OffCmd, 0x30 + 6 * 5, FAN_LOW, 30000);
			}
               */
		}

		FAST_11110ms()
		{

			Timer_SimpleLight(FAN_HIGH);
			Timer_SimpleLight(FAN_LOW);
		}


		// Process the other Gateway stuffs
		FAST_GatewayComms();
	}
	EXECUTESLOW()
	{
		UPDATESLOW();

		SLOW_10s()
		{

			Serial.println("fan_STATUS=");
			Serial.print(mInput(FAN_HIGH));
			Serial.print(mOutput(FAN_HIGH));
			Serial.println(mAuxiliary(FAN_HIGH));
			Serial.println(hour);
			Serial.print(mInput(FAN_LOW));
			Serial.print(mOutput(FAN_LOW));
			Serial.println(mAuxiliary(FAN_LOW));
			Serial.println(fan_state);
		}




		

		SLOW_50s()
		{

	       		temperature = ssDHT_readTemperature(DHT_id1);
      		    humidity = ssDHT_readHumidity(DHT_id1);

//			humidity = dht.readHumidity();
	      //dht.readTemperature(false);
			//if (!isnan(humidity) || !isnan(temperature)) {
			ImportAnalog(HUMIDITY, &humidity);
			ImportAnalog(TEMP0, &temperature);
			Logic_Temperature(TEMP0);
			Logic_Humidity(HUMIDITY);
			Serial.println(temperature);
			Serial.println(humidity);
			Serial.println(Souliss_SinglePrecisionFloating(&mOutput((HUMIDITY))));
			if (humidity > 80 && fan_state == FAN_OFF)
			{
				// day and use fan high
				fan_state = FAN_ON_HUMI;
				if (hour > 7 && hour < 23)
				{
					mInput(FAN_HIGH) = Souliss_T1n_OnCmd;
				}
				else
				{
					mInput(FAN_LOW) = Souliss_T1n_OnCmd;
				}

			} // if humidity

			if (humidity < 75 && fan_state == FAN_ON_HUMI)
			{

				mInput(FAN_HIGH) = Souliss_T1n_OffCmd;
				mInput(FAN_LOW) = 0x30 + 6 * 15;
				fan_state = FAN_OFF;
				// 7 пїЅпїЅпїЅпїЅпїЅ
			}
		}

		SLOW_30m()
		{

			sendNTPpacket(timeServer);
			Serial.println("packet sent");
			delay(3000);

			if (Udp.parsePacket())
			{
				// We've received a packet, read the data from it
				Udp.read(packetBuffer, NTP_PACKET_SIZE);
				unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
				unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
				// combine the four bytes (two words) into a long integer
				// this is NTP time (seconds since Jan 1 1900):
				unsigned long secsSince1900 = highWord << 16 | lowWord;
				// now convert NTP time into everyday time:
				// Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
				const unsigned long seventyYears = 2208988800UL;
				// subtract seventy years:
				unsigned long epoch = secsSince1900 - seventyYears;
				// print Unix time:
				// print the hour, minute and second:
				Serial.print("The houre is ");
				// UTC is the time at Greenwich Meridian (GMT)
				hour = (epoch % 86400L) / 3600 + 3;
				// print the hour (86400 equals secs per day)
				Serial.println(hour);
			}
		}
	}
}

void sendNTPpacket(char *address)
{
	// set all bytes in the buffer to 0
	memset(packetBuffer, 0, NTP_PACKET_SIZE);
	IPAddress ntp(194, 54, 80, 30);
	// Initialize values needed to form NTP request
	// (see URL above for details on the packets)
	packetBuffer[0] = 0b11100011; // LI, Version, Mode
	packetBuffer[1] = 0;		  // Stratum, or type of clock
	packetBuffer[2] = 6;		  // Polling Interval
	packetBuffer[3] = 0xEC;		  // Peer Clock Precision
								  // 8 bytes of zero for Root Delay & Root Dispersion
	packetBuffer[12] = 49;
	packetBuffer[13] = 0x4E;
	packetBuffer[14] = 49;
	packetBuffer[15] = 52;

	// all NTP fields have been given values, now
	// you can send a packet requesting a timestamp:
	Udp.beginPacket(ntp, 123); //NTP requests are to port 123
	Udp.write(packetBuffer, NTP_PACKET_SIZE);
	Udp.endPacket();
}
