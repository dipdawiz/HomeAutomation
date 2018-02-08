# Home Automation (Wi-Fi Relay Board)
     -- Wemos D1 mini code and circuit design for a remotely controllable (using HTTP and MQTT) array of relays, meant to be concealed inside standard switchboards.

This is basically a "Smart Switch" project, with the following features:

1. It is open source!
2. It has an array of 5 relays controlled by a Wemos D1 Mini MCU.
3. Control is possible by HTTP API call and by MQTT messaging (simultaneously, if required).
4. The system provides on-demand and periodic feedback regarding the state of the connected devices/appliances.
5. It is meant to be compact, so it can be installed inside standard switchboards
6. The PCB is designed to be "scalable". So it can be literally cut to the user's needs. So if the switchboard is small, with fewer than 5 switches that need to be automated, then the board can be cut to just include the needed number of relays.
7. The relays can be easily wired to existing wall switches. They can either be connected in parallel to the existing switches, or be connected in a "staircase wiring" configuration to 2-way switches that replace existing 1-way switches, allowing independent control of the lights/fans both electronically (remotely) and manually.
8. The system exposes a basic Web-UI to let the user configure the Wi-Fi router and MQTT Server IP address details and also show status of the "loads" connected, and to control them.
9. There is no "cloud component" or other app dependencies for using this system. All network traffic is local.
10. The state of loads connected to the relay board is persisted on the Wemos, each time any state changes. This saved state can be optionally used to restore the load states after a power-cycle (reboot) of the system.
11. MQTT Broker server is optional and can be disabled from the Web-UI.

## Control API

### HTTP Control API

To turn ON the 4th Load (connected to the 4th Relay), the URL is:
```
http://<IP_Address_of_WeMos>/relay-control?l=4,1
```
Note that the query paraleter is "l", for "'l'oad".

To turn OFF the 4th Load (connected to the 4th Relay), the URL is:
```
http://<IP_Address_of_WeMos>/relay-control?l=4,0
```
To turn ON all 5 Loads, the URL is:
```
http://<IP_Address_of_WeMos>/relay-control?l=0,1
```
To turn OFF all 5 Loads, the URL is:
```
http://<IP_Address_of_WeMos>/relay-control?l=0,0
```

One can toggle the state of a load. For example, to toggle load #4, the URL is:
```
http://<IP_Address_of_WeMos>/relay-control?t=4
```


### MQTT Control

All command messages are to be sent to the topic ```relay-control```

To turn ON the 4th Load (connected to the 4th Relay), the MQTT message to be sent is:
```
l,4,1
```
So the mosquitto_pub command would be:
```
mosquitto_pub -q 1 -h 172.20.1.120 -t relay-control -m l,4,1
```
where 172.20.1.120 is the MQTT Broker Server IP address

Similarly, to turn OFF all 5 Loads, the command would be
```
mosquitto_pub -q 1 -h 172.20.1.120 -t relay-control -m l,0,0
```

To toggle load #4, the command is:
```
mosquitto_pub -q 1 -h 172.20.1.120 -t relay-control -m t,4
```


Replacing 'l' with 'r' (for "'r'elay") in the above 'l' commands and messages would control the relay state. In this case, the state of the actual load is not considered. The actual load can be OFF while the relay is ON, and vice-versa due to the "staircase wiring".


### PCB Design
The PCB is designed in KiCAD and the associated project files are [zipped and included](https://github.com/ajithvasudevan/HomeAutomation/raw/master/HA_KiCAD%20-%20PCB%20Design%20and%20Schematic.zip) in this project.


## Web-User Interface

<img src="https://github.com/ajithvasudevan/HomeAutomation/raw/master/HomeAutomation%20-%20UI.png" alt="Drawing" width="400px"/>



## Prototype

![Prototype](https://github.com/ajithvasudevan/HomeAutomation/raw/master/HomeAutomation%20-%20Prototype.jpg)

