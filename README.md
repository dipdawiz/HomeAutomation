# Home Automation 

This is for someone who are looking for steps towards thier first home automation attempt. I have used an Amazon Echo Dot for voice command system and a Raspberry PI 3B+ for hosting smart switch logic and a Wemos D1 Mini to control relay to do the actual electrical switching job. All connected over home wifi. 

## How it works

You give command to Alexa (Echo); A emulator of famous Belkin Wemo switch, would receive your voice command from Alexa and make appropriate HTTP call; we have a relay control that can control a switch (relay) by HTTP command; as relay switches on your device is switches on/off.

![how-it-works](https://github.com/dipdawiz/HomeAutomation/raw/master/How-it-works.png)

Now following things we have to do

1. build and program a relay control

2. setup a raspberry pi for node-red (to emulate wemo switch)

3. set up Alexa with devices

4. connect the relay control with real electrical switches


## Relay Control

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


### Configuration
Upon first boot, the Wemos starts up in AP mode with SSID **RelayBoard** and the Web-UI is accessible at `http://relay.local/` OR `http://192.168.4.1` from a computer or mobile phone connected to the RelayBoard Wi-Fi network. The home router SSID and password and other details can be entered and saved (persisted on the Wemos). Upon restarting, the Wemos should connect to the home router and get an IP address from there. Now the Web-UI should still be accessible at `http://relay.local/`  or at `http://<whatever IP address the Wemos got from the router>/`


### Web-User Interface

Web interface looks like this

![Web](https://github.com/dipdawiz/HomeAutomation/raw/master/ui.jpg)


### Control API

#### HTTP Control API

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


#### Feedback
Whenever the state of any load changes, /getconfig will return a response with currentstate as
```0,0,0,1```

In the above example, the 4th Load is ON, with all other Loads OFF, and also,

A response with currentstate like ```0,1,0,0``` indicates that the 2nd Load is ON, all other Loads are OFF




## Wemo Emulator on Raspberry PI

Setup your Raspberry PI and enable node-red (a library that helps you do your home automation https://nodered.org/docs/getting-started/). Once you are done with, this is the easiest part. Install wemo emulator by this command

```
$ npm install node-red-contrib-wemo-emulator
```
For more info how to configure this node, here is the repo for this node - https://github.com/biddster/node-red-contrib-wemo-emulator. With default configuration of your node-red on your PI, you can now access the node-red url from
```
http://<raspberry-pi-ip>:1880
```

Now drag and drop your wemo emulator on the blank flow and set up along with a http request node in node-red. you need as many wemo emu node as many device. Here I shown with 4 as my living room has 4 device to control.

Overall flow 
![node-red-flow](https://github.com/dipdawiz/HomeAutomation/raw/master/Node-red-flow.png)


Wemo emulator config
You need to give unique id across your devices, a unique port across your devices and a friendly name.
Now "On Payload" and "Off Payload" should be your relay controls on and off parameters. (see image for example)

![wemo-emu](https://github.com/dipdawiz/HomeAutomation/raw/master/Node-config.png)

HTTP request node config
This node makes a HTTP call to our relay control 
![http-node](https://github.com/dipdawiz/HomeAutomation/raw/master/http-call-config.png)

Now deploy the flow and it would be ready to use.

## Setup Amazon Echo

As soon as you deploy your node-red flow in previous step, you are ready to set up your Echo (Dot/Plus). On your mobile, open Alexa app. From the menu, click on Smart Home. And add a device. it would search and find out as many wemo-emu node you added in your flow. Add them and you are good to give voice command to Alexa/Echo/Computer. Like in my example, if I say, "Alexa, switch on Living room lights", it switch on relay for the lights in living room.

#### Fun with Routine

You can combine multiple commands in a routine and achieve all in a single voice command. Setup a routine (from Alexa App> Menu> routine). Add Routine, Select "Voice", Write down the voice command text, Add individual command. You are done.

![alexa-app](https://github.com/dipdawiz/HomeAutomation/raw/master/alexa-app.png)



## The Dirty Job

This is where you actually connect the relay control to your physical switch board. Before you start I am expecting you are aware with basic electric switch's working and you are comfortable with handling real electrical connections without messing it up. Else take help.

Here is how the connection would look
![elecrtic-connection](https://github.com/dipdawiz/HomeAutomation/raw/master/electric-connection.png)

#### Wemos, Relays, Power, Raspberry Pi and Echo

Relay: 5v 4 channel relay.
https://www.amazon.in/REES52-Optocoupler-Channel-Control-Arduino/dp/B01HXM1G9Q/
(Amazon.in search for 4 channel relay)

Power block: Hi-link 5v 5watt power block
https://www.amazon.in/SunRobotics-Hi-Link-HLK-5M05-Step-Down-220V-5V/dp/B0734SG8QB
(Amazon.in search for hilink 5v power supply)

Wemos: Wemos/Lolin D1 mini (or its replica)
https://www.amazon.in/Arduino-D1-Mini-V2-Development/dp/B01FMJ0H2Y
(Amazon.in search for wemos d1 mini)

Echo Dot: Amazon Echo Dot 
https://www.amazon.in/Echo-Dot-Voice-control-weather/dp/B071NB4PGV

Raspberry Pi: Raspberry PI 3b+
https://www.amazon.in/Raspberry-Pi-3B-plus-Motherboard/dp/B07BDR5PDW/

Go light with Raspberry PI Zero W
https://www.amazon.in/Raspberry-Pi-Zero-Development-Built/dp/B06XFZC3BX

## Prototype

![Prototype](https://github.com/dipdawiz/HomeAutomation/raw/master/prototype.jpg)

## More
For two(or three) way switch, self design PCB, with more option for control using MQTT, see https://github.com/ajithvasudevan/HomeAutomation 
