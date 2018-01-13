# lightSwitches
Repository containing NodeJS and Arduino code for remote controlling Orvibo S10/20 smart switches.

## Contents
This repository contains 4 directories. They are as follows
* NodeJS - this is a server/mediator that provides a WebSocket, MQTT, HTTP and WeMo protocol interface to the S10/S20 smart sockets.
* lightSwitchv3 - this is the Arduino code for Version 3 of the smartswitch, pictured below:
![lightSwitchv3](http://new.alexshakespeare.com/wp-content/uploads/2018/01/v3.jpg)
<br/>The Fritzing schematic for this switch is v3.fzz.
* lightSwitchv4 - this is the Arduino code for Version 4 of the smartswitch, pictured below:
![lightSwitchv4](http://new.alexshakespeare.com/wp-content/uploads/2018/01/v4.jpg)
<br/>index.html is the uncompressed version of the page this switch serves from PROGMEM - compress it using an online tool of your choice if you want to customise.
<br/>The Fritzing schematic for this switch is v4.fzz.
* pebble - this is a Cloudpebble .zip backup of the Pebble client for these switches

Further details on this project are available at [alexshakespeare.com](http://new.alexshakespeare.com/self-actuated-programmable-switch/)
