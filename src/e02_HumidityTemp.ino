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
#include <avr/wdt.h>
#include <avr/eeprom.h>
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
#define DHTPIN 2 //data pin for dht sensor
#define DHTTYPE DHT22
#define LIGHTPIN 5
#define LIGHT_TIME 20000UL // time before fan goes on in millis
////#define DHTPIN 2 // what digital pin we're connected to

//#define DHTPIN 2 // what digital pin we're connected to
#define Valve_Open_PIN 6
#define Valve_Close_PIN 7
#define pixel_pin 3
#define Airwick_pin 8
/*** All configuration includes should be above this line ***/
#include "Souliss.h"

#define HUMIDITY 0 // Leav8e 2 slots for T53
#define TEMP0 2	   // Leave 2 slots for T52
//#define Humi_Setpoint 4
#define FAN_HIGH 5
#define LIGHT 6
#define HUMISET 8
#define Cold_Valve 10
//#define NEO_PIXEL  12
#define AirWick 11

// DHT sensor
DHT dht(DHTPIN, DHTTYPE); // for ESP8266 use dht(DHTPIN, DHTTYPE, 11)

uint8_t ip_address[4] = {192, 168, 0, 78};
uint8_t subnet_mask[4] = {255, 255, 255, 0};
uint8_t ip_gateway[4] = {192, 168, 0, 1};
uint8_t hour = 23;
uint8_t led_num = 0;

enum states
{
	FAN_OFF,
	FAN_ON_HUMI,
	FAN_ON_LIGHT
};
states fan_state = FAN_OFF;
const int light_pin = 5;
uint8_t light_state = 1, light_state_prev = 1;
uint8_t dead_time = 0;
uint8_t humi_light;
uint8_t isDay = 0;
uint8_t light_on_cycles = 0;
uint8_t cycle_state = 0;

float humidity = 0;
float humidity_prev = 0;
float humi_SET = 85;
word humi_eeprom;
#define myvNet_address ip_address[3] // The last byte of the IP address (78) is also the vNet address
#define myvNet_subnet 0xFF00
EthernetUDP Udp;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, pixel_pin, NEO_GRB + NEO_KHZ800);

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

void setup()
{

	strip.begin();

	strip.setPixelColor(0, strip.Color(0, 0, 0));
	strip.show();
	// Get the IP address from DHCP
	delay(2000);
	wdt_enable(WDTO_8S);
	Initialize();

	// Get the IP address from DHCP
	Souliss_SetIPAddress(ip_address, subnet_mask, ip_gateway);

	SetAsGateway(myvNet_address); // Set this node as gateway for SoulissApp
	Udp.begin(8888);
	Serial.begin(57600);
	dht.begin(); // initialize temperature sensor

	pinMode(9, OUTPUT);
	pinMode(4, OUTPUT);
	pinMode(8, OUTPUT);
	pinMode(light_pin, INPUT);
	digitalWrite(light_pin, LOW);
	Set_Humidity(HUMIDITY);
	Set_Temperature(TEMP0);
	Set_SimpleLight(FAN_HIGH);
	//	Set_SimpleLight(Humi_Setpoint);
	Set_DigitalInput(LIGHT);
	Set_Humidity_Setpoint(HUMISET);
	Set_T22(Cold_Valve);
	//	Set_T16(NEO_PIXEL);
	Set_T14(AirWick);

	//пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅ

	sendNTPpacket();

	Serial.println("packet sent");
	//TODO: change version
	Serial.println("Verion 3.2.5");

	strip.setPixelColor(0, strip.Color(255, 179, 0)); //RGB
	strip.show();
	delay(3000);

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
		if ((hour >= 7) && (hour <= 21))
			isDay = 1;
		else
			isDay = 0;
	}
	else
	{
		strip.setPixelColor(0, strip.Color(200, 0, 0));
		strip.show();
	}

	wdt_enable(WDTO_1S);
	Serial.println("millis STATE HIGH LOW INPUT_LIGHT FAN_STATE VAlVE, Airwick,HumiSetpoint:,");
	delay(100);

	//read HUMIset from eeprom
	humi_eeprom = eeprom_read_word((const uint16_t)10);
	// humi_SET = Souliss_SinglePrecisionFloating((uint8_t*)&humi_eeprom);
	humi_SET = eeprom_read_word(10);
	Serial.println(humi_SET);
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

		FAST_210ms()
		{
			wdt_reset();

			light_state = digitalRead(light_pin);

			if ((light_state == 0) && (light_state_prev == 1))
			{
				light_on_cycles = 0; //reset cycles
			}
			// if light lighting
			if ((light_state == 0) && (light_state_prev == 0))
			{
				light_on_cycles++;
			}

			if ((light_on_cycles == 200) && (isDay == 1))
			{
				mInput(FAN_HIGH) = 0x30 + 6 * 4;
				light_on_cycles = 0;
			}

			// light off for short time
			if ((light_state == 1) && (light_state_prev == 0) && (light_on_cycles < 50))
				mInput(FAN_HIGH) = Souliss_T1n_OffCmd;

			// reset counter if light OFF
			if (light_state == 1)
				light_on_cycles = 0;
			light_state_prev = light_state;

			Logic_SimpleLight(FAN_HIGH);
			Logic_Humidity(HUMIDITY);
			Logic_Temperature(TEMP0);
			Logic_T22(Cold_Valve);

			DigOut(Valve_Open_PIN, Souliss_T2n_Coil_Open, Cold_Valve);
			DigOut(Valve_Close_PIN, Souliss_T2n_Coil_Close, Cold_Valve);
			DigOut(9, Souliss_T1n_Coil, FAN_HIGH);
			DigOut(8, Souliss_T1n_OnCoil, AirWick);
			Timer_T22(Cold_Valve);
			LowDigIn(5, Souliss_T1n_OnCmd, AirWick);
		}

		FAST_11110ms()
		{
			Timer_SimpleLight(FAN_HIGH);
		}

		FAST_510ms()
		{

			Logic_T14(AirWick);
		}

		FAST_2110ms()
		{

			switch (led_num)

			{
			case 0:
				strip.setPixelColor(0, 255, 255, 255); //white
				strip.setBrightness(255);
				break;

			case 1:
				if (isDay == 1)
					strip.setPixelColor(0, 50, 205, 50); //green
				else
					strip.setPixelColor(0, 0, 0, 150);
				strip.setBrightness(255); //blue
				break;

			case 2:
				if (fan_state == FAN_ON_HUMI)
					strip.setPixelColor(0, 155, 0, 255); // magenta
				else
				{
					if (mOutput(FAN_HIGH) == Souliss_T1n_OnCoil)
						strip.setPixelColor(0, 100, 10, 10); //dark red in light mode
					else
						strip.setPixelColor(0, 255, 128, 0); //MAGENTA IF fan humidity mode
				}											 //orange;
				strip.setBrightness(255);
				break;

			case 3:

				strip.setPixelColor(0, 0, 255 - (uint8_t)(((humi_light / 20) * 20) * 2.5), (uint8_t)(((humi_light / 20) * 20) * 2.5));
				strip.setBrightness(70);
				break;

			default:
				strip.setPixelColor(0, 0, 0, 0);
				break;
			}

			led_num++;
			led_num = led_num & 0x03;
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

			Serial.print(millis());
			Serial.print(",");
			Serial.print(mInput(FAN_HIGH));
			Serial.print(mOutput(FAN_HIGH));
			Serial.print(mAuxiliary(FAN_HIGH));
			Serial.print(",");
			Serial.print(mInput(HUMISET));
			Serial.print(mOutput(HUMISET));
			Serial.print(mAuxiliary(HUMISET));

			Serial.print(",");
			int sensor_read = digitalRead(5);
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
			Serial.print("Setpoint:");
			Serial.println(humi_SET);
		}

		SLOW_50s()
		{

			humidity = dht.readHumidity();
			humi_light = (uint8_t)(humidity);
			float temperature = dht.readTemperature(false);
			//halt for reset
			if (isnan(humidity) || isnan(temperature))
			{
				while (1)
				{
				};
			}

			if (isnan(Souliss_SinglePrecisionFloating(&mInput(HUMISET))))
			{

				humi_SET = eeprom_read_word(10);
			}
			else
			{

				if (humi_SET != Souliss_SinglePrecisionFloating(&mInput(HUMISET)))
				{
					//received new value from souliss
					humi_SET = Souliss_SinglePrecisionFloating(&mInput(HUMISET));
					eeprom_write_word(10, humi_SET);
				}
			}

			//if (!isnan(humidity) || !isnan(temperature)) {
			ImportAnalog(HUMIDITY, &humidity);
			ImportAnalog(TEMP0, &temperature);
			Serial.print("TEMP HUMI:,");
			Serial.print(temperature);
			Serial.print(",");
			Serial.print((uint8_t)(((humi_light / 20) * 20) * 2.5));
			Serial.print(",");
			Serial.println(humidity);
			Logic_Humidity(HUMIDITY);
			Logic_Humidity_Setpoint(HUMISET);
			//TODO: add command to ON fan if humi between on-off and fan stoped
			if ((humidity > humi_SET))
			{
				if (hour >= 7 && hour <= 23)
				{
					mInput(FAN_HIGH) = Souliss_T1n_OnCmd;
					fan_state = FAN_ON_HUMI;
				}

			} // if humidity
			else if ((humidity < (humi_SET - 10)) && (fan_state == FAN_ON_HUMI))
			{

				mInput(FAN_HIGH) = 0x30 + 6 * 1;

				fan_state = FAN_OFF;

				dead_time = 1;
				// 7 пїЅпїЅпїЅпїЅпїЅ
			}
		}

		SLOW_x10s(3)
		{
			dead_time = 0;
		}

		SLOW_110s()
		{

			if (cycle_state == 0)
			{

				sendNTPpacket();
				cycle_state = 1;
			}

			//Serial.println("packet sent");

			else
			{
				cycle_state = 0;
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
					//	if (hour==7)
					//	   	mInput(AirWick)=Souliss_T1n_OnCmd;

					if ((hour >= 7) && (hour <= 21))
						isDay = 1;
					else
						isDay = 0;

				} // if parsePacket

			} //else
		}

		//Serial.print("sl_out:");
		//Serial.println(millis());
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