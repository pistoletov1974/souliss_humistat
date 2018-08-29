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

// Include framework code and libraries
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Adafruit_NeoPixel.h>

//debug mode
//#define SERIALPORT_INSKETCH
//#define LOG Serial

// Include sensor libraries (from Adafruit) Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22 // DHT 22 (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
#define DHTPIN            2     //data pin for dht sensor
#define DHTTYPE           DHT22
#define LIGHTPIN          5
#define LIGHT_TIME        20000UL // time before fan goes on in millis
////#define DHTPIN 2 // what digital pin we're connected to

//#define DHTPIN 2 // what digital pin we're connected to
#define Valve_Open_PIN  6
#define Valve_Close_PIN 7
#define pixel_pin       3
#define Airwick_pin     8 
/*** All configuration includes should be above this line ***/
#include "Souliss.h"

#define HUMIDITY 0 // Leav8e 2 slots for T53
#define TEMP0 2	// Leave 2 slots for T52
#define FAN_LOW 4
#define FAN_HIGH 5
#define LIGHT 6
#define HUMISET 8
#define Cold_Valve 10
#define NEO_PIXEL  11
#define AirWick    15 



// DHT sensor
DHT dht(DHTPIN, DHTTYPE); // for ESP8266 use dht(DHTPIN, DHTTYPE, 11)

uint8_t ip_address[4] = {192, 168, 0, 78};
uint8_t subnet_mask[4] = {255, 255, 255, 0};
uint8_t ip_gateway[4] = {192, 168, 0, 1};
uint8_t hour = 23;
//uint32_t led_colour[4];
uint8_t  led_num=0;

enum states
{
	FAN_OFF,
	FAN_ON_HUMI,
	FAN_ON_LIGHT
};
states fan_state = FAN_OFF;
const int light_pin = 5;
uint8_t light_state=0, light_state_prev=0;
uint8_t dead_time=0;

float humidity = 0;
float humidity_prev = 0;
#define myvNet_address ip_address[3] // The last byte of the IP address (77) is also the vNet address
#define myvNet_subnet 0xFF00
EthernetUDP Udp;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, pixel_pin, NEO_GRB + NEO_KHZ800);
//const PROGMEM uint32_t magenta = strip.Color(255, 0, 255);
//const PROGMEM uint32_t yellow = strip.Color(255, 255, 0); 
//const PROGMEM uint32_t white = strip.Color(255, 255, 255);
//const PROGMEM uint32_t deep_blue = strip.Color(51, 51, 255);
//const PROGMEM uint32_t orange = strip.Color(255, 128, 0);
//const PROGMEM uint32_t green = strip.Color(50, 205, 50);


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
	dht.begin(); // initialize temperature sensor
	
	 
	pinMode(9, OUTPUT);
	pinMode(4, OUTPUT);
	pinMode(8,OUTPUT);
	pinMode(light_pin, INPUT);
    digitalWrite(light_pin,LOW);
	Set_Humidity(HUMIDITY);
	Set_Temperature(TEMP0);
	Set_SimpleLight(FAN_HIGH);
	Set_SimpleLight(FAN_LOW);
	Set_DigitalInput(LIGHT);
	Set_Humidity_Setpoint(HUMISET);
	Set_T22(Cold_Valve);
	Set_T16(NEO_PIXEL);
	Set_T14(AirWick);



	//пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅ

	sendNTPpacket();

	Serial.println("packet sent");

    Serial.println("Verion 2.17");
   
	strip.begin();

	strip.setPixelColor(0,strip.Color(255,179,0)); //RGB
	strip.show();
	delay(2000);


	if (Udp.parsePacket())
	{
		// We've received a packet, read the data from it
		Udp.read(packetBuffer, NTP_PACKET_SIZE);
		// read the packet into the buffer

		// the timestamp starts at byte 40 of the received packet and is four bytes,
		// or two words, long. First, extract the two words:

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
	else {
		strip.setPixelColor(0,strip.Color(200,0,0));
		strip.show();
	}
	mInput(AirWick)=Souliss_T1n_OnCmd;

	
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


			ProcessCommunication();

		}

		FAST_50ms()
		{

			if (fan_state != FAN_ON_HUMI)
			{

				if (hour > 7 && hour < 23)
				{
					LowDigInHoldCustom(5, Souliss_T1n_OffCmd, 0x30 + 6 * 5, FAN_HIGH, 40000UL);
					
				}

				else
					LowDigInHoldCustom(5, Souliss_T1n_OffCmd, 0x30 + 6 * 5, FAN_LOW, 40000UL);
			}

			//Souliss_DigInHold(5, Souliss_T1n_OffCmd, Souliss_T1n_OnCmd, LIGHT, 10000);

			Logic_SimpleLight(FAN_HIGH);
			Logic_SimpleLight(FAN_LOW);

			//Logic_Humidity_Setpoint(HUMISET);
			Logic_Humidity(HUMIDITY);
			Logic_Temperature(TEMP0);
			Logic_T22(Cold_Valve);
			DigOut(Valve_Open_PIN,Souliss_T2n_Coil_Open,Cold_Valve);
			DigOut(Valve_Close_PIN,Souliss_T2n_Coil_Close,Cold_Valve);
			DigOut(4, Souliss_T1n_Coil, FAN_LOW);
			DigOut(9, Souliss_T1n_Coil, FAN_HIGH);
			DigOut(8,Souliss_T1n_OnCoil,AirWick);
			Logic_T16(NEO_PIXEL);
		}

		FAST_90ms()
		{
		}

		// пїЅпїЅпїЅпїЅпїЅпїЅ  пїЅ 10 пїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅ
		FAST_11110ms()
		{

			Timer_SimpleLight(FAN_HIGH);
			Timer_SimpleLight(FAN_LOW);
			
		
		}

        // default time 0xC0-0xA0 * 210 ms app 6 sec
		FAST_210ms()
		{
				
				Timer_T22(Cold_Valve);
			

		}

	      	FAST_710ms()
		 {
           // airwick work only in day if light goes on and fan_high not run
		   if ((hour>7)&&(hour<23) && (mOutput(FAN_HIGH)==Souliss_T1n_OffCoil) && (dead_time==0) )  {
		   
		   light_state=digitalRead(light_pin);
		   if ((light_state==LOW) && (light_state!=light_state_prev)) 
		   {
                     mInput(AirWick)=Souliss_T1n_OnCmd;
					 dead_time=1;
		   }
		       	             
		    light_state_prev=light_state;	
				   
		   }
		   Logic_T14(AirWick);
		 }


		FAST_510ms()
		{
			LowDigIn(5,Souliss_T1n_OnCmd,AirWick);
		}


		


		FAST_2110ms()
		 {
               // led colours rotation (see readme.md)
			   Serial.println(led_num);



			 





			   switch (led_num)
			   
			    {
                       case 0:
                       strip.setPixelColor(0,255,255,255); //white
                       break;
					   
					   case 1:
					   if ((hour>=7) && (hour<=23)) strip.setPixelColor(0,50,205,50);       //green
                            else strip.setPixelColor(0,255,255,0);                         //yellow
					   break;

					   case 2:
					   if (fan_state == FAN_ON_HUMI)  strip.setPixelColor(0,255,0,255);     // magenta
				           else  {
						        if (mOutput(FAN_HIGH)==Souliss_T1n_OnCoil) 
								   strip.setPixelColor(0,120,30,30);  //dark red
						             else   strip.setPixelColor(0,255,128,0);      
									 }                   //orange;
					  break;

					  case 3:
					  
						strip.setPixelColor(0, 10, 255-(uint8_t)(humidity*2),(uint8_t)(humidity*2));                      					  
					  break;

					  default:
                             strip.setPixelColor(0,0,0,0);
					  break;

			   }			

			
			   led_num++;
			   led_num=led_num & 0x03;
			  // if (led_num==4) led_num=0;
			   strip.show();
		 }





		// Process the other Gateway stuffs
		FAST_GatewayComms();
	}
	EXECUTESLOW()
	{
		UPDATESLOW();

		SLOW_10s()
		{

			Serial.print("STATE HIGH LOW INPUT_LIGHT FAN_STATE VAlVE, Airwick:,");
			Serial.print(mInput(FAN_HIGH));
			Serial.print(mOutput(FAN_HIGH));
			Serial.print(mAuxiliary(FAN_HIGH));
			Serial.print(",");
			Serial.print(mInput(FAN_LOW));
			Serial.print(mOutput(FAN_LOW));
			Serial.print(mAuxiliary(FAN_LOW));
			
      Serial.print(",");
      int sensor_read=digitalRead(5);
			Serial.print(sensor_read);
      Serial.print(",");
			Serial.print(fan_state);
			Serial.print(",");
			Serial.print(mInput(Cold_Valve));
			//Serial.print(",");
			Serial.print(mOutput(Cold_Valve));
			//Serial.print(",");
			Serial.print(mAuxiliary(Cold_Valve));
			Serial.println(mInput(AirWick));
		}

		SLOW_50s()
		{

			humidity = dht.readHumidity();
			float temperature = dht.readTemperature(false);
			//if (!isnan(humidity) || !isnan(temperature)) {
			ImportAnalog(HUMIDITY, &humidity);
			ImportAnalog(TEMP0, &temperature);
      Serial.print("TEMP HUMI:,");
			Serial.print(temperature);
      Serial.print(",");
			Serial.println(humidity);
			Logic_Humidity(HUMIDITY);
			//Serial.println(Souliss_SinglePrecisionFloating(&mOutput((HUMIDITY))));
			// high humidity
			//пїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅ 75% пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅ
			if ((humidity > 85 && fan_state == FAN_OFF) || (humidity>95))
			{
				// day and use fan high
				fan_state = FAN_ON_HUMI;
				//Serial.println(fan_state);

				if (hour > 7 && hour < 23)
				{
					mInput(FAN_HIGH) = Souliss_T1n_OnCmd;
				}
				else
				{
					mInput(FAN_LOW) = Souliss_T1n_OnCmd;
				}

			} // if humidity

			//пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅ пїЅпїЅпїЅпїЅпїЅ пїЅпїЅ 60  пїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅ 7 пїЅпїЅпїЅпїЅпїЅ
			if (humidity < 75 && fan_state == FAN_ON_HUMI)
			{

				mInput(FAN_HIGH) = Souliss_T1n_OffCmd;
				mInput(FAN_LOW) = 0x30 + 6 * 15;
				fan_state = FAN_OFF;
				mInput(AirWick)=Souliss_T1n_OnCmd;
				dead_time=1;
				// 7 пїЅпїЅпїЅпїЅпїЅ
			}
		}

        SLOW_510s()
		 {
           dead_time=0; 
		}

		SLOW_15m()
		{

			sendNTPpacket();

			//Serial.println("packet sent");
            
			delay(2000);

			if (Udp.parsePacket())
			{
				// We've received a packet, read the data from it
				Udp.read(packetBuffer, NTP_PACKET_SIZE);
				// read the packet into the buffer

				// the timestamp starts at byte 40 of the received packet and is four bytes,
				// or two words, long. First, extract the two words:

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
				if (hour==7)  
				   	mInput(AirWick)=Souliss_T1n_OnCmd;


			}
		}
	}
}

void sendNTPpacket()
{
	// set all bytes in the buffer to 0
	memset(packetBuffer, 0, NTP_PACKET_SIZE);
	IPAddress ntp(192, 168, 0, 1);
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