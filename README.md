# souliss_humistat
souliss node control fan depending on humidity in room
This is a simple test of SmartHome framework Souliss.
This node check temp & humidity from dht22 sensor and switch ventilation fam in high or low mode depending
on hour of day. Fan also controlled by light switch in wc room.
UPD 10/07/18  
This souliss node now control water ball valve thru OpenHAB, get time from NTP server to set day/night modes
Used Pins:
10-CS,
11-MOSi
12-MISO
13-SCK
0-serial
1-serial
2-DHT22 sensor
3-neopixel pin
4-FAN_LOW
5-light sensor
6-Valve_Open
7-Valve_close
8-aromadiffusor (future)
9-FAN_HIGH