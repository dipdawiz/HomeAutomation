# Home Automation (Wi-Fi Relay Board)
Wemos D1 mini code and prototype design for a remotely controllable (using HTTP) array of relays, meant to be concealed inside standard switchboards.

This is basically a "Smart Switch" project, with the following features:

1. It is open source!
2. It has an array of 4 relays controlled by a Wemos D1 Mini MCU.
3. Control is possible by HTTP API call 
4. The system provides on-demand feedback regarding the state of the connected devices/appliances.
5. It is meant to be compact, so it can be installed inside standard switchboards
6. The relays can be easily wired to existing wall switches. They can either be connected in parallel to the existing switches
7. The system exposes a basic Web-UI to let the user configure the Wi-Fi router and also show status of the "loads" connected, and to control them.
8. There is no "cloud component" or other app dependencies for using this system. All network traffic is local.
9. The state of loads connected to the relay board is persisted on the Wemos, each time any state changes. This saved state can be optionally used to restore the load states after a power-cycle (reboot) of the system.
10. What I used is active low relay which takes a HIGH to shut the relay off. 


## Configuration
Upon first boot, the Wemos starts up in AP mode with SSID **RelayBoard** and the Web-UI is accessible at `http://relay.local/` OR `http://192.168.4.1` from a computer or mobile phone connected to the RelayBoard Wi-Fi network. The home router SSID and password and other details can be entered and saved (persisted on the Wemos). Upon restarting, the Wemos should connect to the home router and get an IP address from there. Now the Web-UI should still be accessible at `http://relay.local/`  or at `http://<whatever IP address the Wemos got from the router>/`


## Web-User Interface

Web interface looks like this

![Web](https://github.com/dipdawiz/HomeAutomation/raw/master/ui.jpg)


## Control API

### HTTP Control API

To turn ON the 4th Load (connected to the 4th Relay), the URL is:
```
http://<IP_Address_of_Wemos>/relay-control?l=4,1
```
Note that the query parameter is "l", for "'l'oad".

To turn OFF the 4th Load (connected to the 4th Relay), the URL is:
```
http://<IP_Address_of_Wemos>/relay-control?l=4,0
```
To turn ON all 5 Loads, the URL is:
```
http://<IP_Address_of_Wemos>/relay-control?l=0,1
```
To turn OFF all 5 Loads, the URL is:
```
http://<IP_Address_of_Wemos>/relay-control?l=0,0
```

One can toggle the state of a load. For example, to toggle load #4, the URL is:
```
http://<IP_Address_of_Wemos>/relay-control?t=4
```


### Feedback
Whenever the state of any load changes, /getconfig will return a response with currentstate as
```0,0,0,1```

In the above example, the 4th Load is ON, with all other Loads OFF, and also,

A response with currentstate like ```0,1,0,0``` indicates that the 2nd Load is ON, all other Loads are OFF


### Wemos, Relays and Power

Relay: 5v 4 channel relay.
https://www.amazon.in/REES52-Optocoupler-Channel-Control-Arduino/dp/B01HXM1G9Q/
(Amazon.in search for 4 channel relay)

Power block: Hi-link 5v 5watt power block
https://www.amazon.in/SunRobotics-Hi-Link-HLK-5M05-Step-Down-220V-5V/dp/B0734SG8QB
(Amazon.in search for hilink 5v power supply)

Wemos: Wemos/Lolin D1 mini (or its replica)
https://www.amazon.in/Arduino-D1-Mini-V2-Development/dp/B01FMJ0H2Y
(Amazon.in search for wemos d1 mini)



## Prototype

![Prototype](https://github.com/dipdawiz/HomeAutomation/raw/master/prototype.jpg)

## More
For two(or three) way switch, self design PCB, with more option for control using MQTT, see https://github.com/ajithvasudevan/HomeAutomation 
