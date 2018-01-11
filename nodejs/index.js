'use strict';

var fetch = require('node-fetch');
var Orvibo = require("./orvibo.js")
var o = new Orvibo()
var mqtt = require('mqtt')

// Four timers for repeating messages, as UDP doesn't guarantee delivery
// These timers are cancelled once
var timer1 // This timer is used for discovering devices
var timer2 = [] // This timer is used to subscribe to a device
var timer3 = [] // This timer is used to query a device
var timer4 = [] // This timer is used to toggle a socket or learn / emit IR

var hosts = [];
var ws = require("nodejs-websocket");
var http = require('http');
var request = require('request');
var previousSend;

var mqttclient  = mqtt.connect({host: '192.168.1.2', username:'xxx', password:'xxx', debug: true, protocolId: 'MQIsdp', protocolVersion: 3});
 
mqttclient.on('connect', function () {
  mqttclient.subscribe('manualswitch');
  console.log('Connected to mqtt');
})

mqttclient.on('error', function (error) {
  console.log(error);
})
 
mqttclient.on('message', function (topic, message) {

  if (message.toString() == 'discover') {

     mqttclient.publish('manualswitch',getStateString());

  } else {

      var command=message.toString().split(':');
      console.log(command);
      
      if (isNumeric(command[0]) && (command[1] == 'on' || command[1] == 'off')) {

        var hostTemp = o.devices.sort(function(a,b) {return (a.name > b.name) ? 1 : ((b.name > a.name) ? -1 : 0);} );

        for (var i=0; i<20; i++) {
          var setState = (command[1] == 'on') ? true : false;
          o.setState({ device: hostTemp[command[0]], state: setState});
        }
      }
    }
});

const FauxMo = require('fauxmojs');

let fauxMo = new FauxMo(
  {
    ipAddress: '192.168.1.2',
    devices: [
      {
        name: 'kitchen lights',
        port: 11000,
        handler: (action) => {
          fetch("http://192.168.1.2/?kitchencounter:" + action);
          fetch("http://192.168.1.2/?kitchensurround:" + action);
        }
      },
      {
        name: 'kitchen counter',
        port: 11001,
        handler: (action) => {
          fetch("http://192.168.1.2/?kitchensurround:" + action);
          o.setState({device: hosts['kitchencounter'], state: action});
        }
      },
      {
        name: 'kitchen surround',
        port: 11002,
        handler: (action) => {
          fetch("http://192.168.1.2/?kitchensurround:" + action);
        }
      },
      {
        name: 'conservatory fan',
        port: 11003,
        handler: (action) => {
          fetch("http://192.168.1.2/?conservatoryfan:" + action);
        }
      },
      {
        name: 'living room lamp',
        port: 11004,
        handler: (action) => {
          fetch("http://192.168.1.2/?livingroomlamp:" + action);
        }
      },
      {
        name: 'basement fan',
        port: 11005,
        handler: (action) => {
          fetch("http://192.168.1.2/?basementfan:" + action);
        }
      },
      {
        name: 'basement lights',
        port: 11006,
        handler: (action) => {
          fetch("http://192.168.1.2/?basementlights:" + action);
        }
      },
      {
        name: 'pool table',
        port: 11007,
        handler: (action) => {
          fetch("http://192.168.1.2/?pooltable:" + action);
        }
      },
      {
        name: 'bedroom light',
        port: 11008,
        handler: (action) => {
          fetch("http://192.168.1.2/?bedroomlight:" + action);
        }
      }
    ]
  });
 
console.log('started..');

var webServer = http.createServer(function(request, response) {

    console.log((new Date()) + ' Received request for ' + request.url);

    var message=request.url.substring(2,request.url.length+2);
    response.setHeader('Access-Control-Allow-Origin', '*');
    response.writeHead(200);
    
    if (message == 'discover') {

    var hostArray = [];
    var hostTemp = o.devices.sort(function(a,b) {return (a.name > b.name) ? 1 : ((b.name > a.name) ? -1 : 0);} );

    for (var i=0; i<hostTemp.length; i++) { 
  console.log(hostTemp[i]);
        hostArray.push({ name: hostTemp[i].name, state: hostTemp[i].state, ip:hostTemp[i].ip});
    }

    response.write(JSON.stringify({ hosts : hostArray}));
    response.end();
    } else {
      var command=message.split(':');
      if (command[1]) {
        for (var i=0; i<o.devices.length; i++) { 
          hosts[o.devices[i].name]=o.devices[i];
        }
        for (var i=0; i<5; i++) {
        var setState = (command[1] == 'on') ? true : false;
        o.setState({ device: hosts[command[0]], state: setState});
        }
        response.write(JSON.stringify({status:"OK"}));
        response.end();
      }
    }
});

webServer.listen(80, function() {
    console.log((new Date()) + ' Server is listening on port 80');
});

var server = ws.createServer(function (conn) {
    console.log("New WS connection")

    conn.on("text", function (str) {
      if (str == 'discover') {
          conn.sendText(JSON.stringify({ hosts : o.devices.sort(function(a,b) {return (a.name > b.name) ? 1 : ((b.name > a.name) ? -1 : 0);} )}));
      } else {
        var command=str.split(':');
        var hosts=[];
        for (var i=0; i<o.devices.length; i++) { 
          hosts[o.devices[i].name]=o.devices[i];
        }
        for (var i=0; i<5; i++) {
        var setState = (command[1] == 'on') ? true : false;
        o.setState({ device: hosts[command[0]], state: setState});
        }
      }    
    })

    conn.on("close", function (code, reason) {
        console.log("Connection closed")
    })

}).listen(3000);

// We've listened, and now we're ready to go.
o.on("ready", function() {
  timer1 = setInterval(function() { // Set up a timer to search for sockets every second until found
    o.discover()
  }, 1000)
})

// A device has been found and added to our list of devices
o.on("deviceadded", function(device) {
//  clearInterval(timer1) // Clear our first timer, as we've found at least one socket
  o.discover() // Ask around again, just in case we missed something
  timer2[device.macAddress] = setInterval(function() { // Set up a new timer for subscribing to this device. Repeat until we get confirmation of subscription
    o.subscribe(device)
  }, 1000)
})

// We've asked to subscribe (control) a device, and now we've had a response.
// Next, we will query the device for its name and such
o.on("subscribed", function(device) {
  clearInterval(timer2[device.macAddress]) // Stop the second subscribe timer for this device
  timer3[device.macAddress] = setInterval(function() { // Set up another timer, this time for querying
    o.query({
      device: device, // Query the device we just subscribed to
      table: "04" // See PROTOCOL.md for info. "04" = Device info, "03" = Timing info
    })
  }, 1000)
})

// Our device has responded to our query request
o.on("queried", function(device, table) {
  clearInterval(timer3[device.macAddress]) // Stop the query timer
  timer2[device.macAddress] = setInterval(function() { // Set up a new timer for subscribing to this device. Repeat until we get confirmation of subscription
    o.subscribe(device)
  }, 250000)
})

o.on("statechangeconfirmed", function(device) {
  console.log("Socket %s confirming state change to", device.macAddress, device.state)

  server.connections.forEach(function (conn) {
        conn.sendText(JSON.stringify({ hosts : o.devices.sort(function(a,b) {return (a.name > b.name) ? 1 : ((b.name > a.name) ? -1 : 0);} )}));
  });


  if (getStateString() != previousSend) {
    previousSend = getStateString();
    mqttclient.publish('manualswitch', previousSend);
  }

})

o.on("externalstatechanged", function(device) {
  console.log("Socket %s confirming state change to", device.macAddress, device.state);

  server.connections.forEach(function (conn) {
        conn.sendText(JSON.stringify({ hosts : o.devices.sort(function(a,b) {return (a.name > b.name) ? 1 : ((b.name > a.name) ? -1 : 0);} )}));
  });

  if (getStateString() != previousSend) {
    previousSend = getStateString();
    mqttclient.publish('manualswitch', previousSend);
  }

})


o.listen();

function isNumeric(n) {
  return !isNaN(parseFloat(n)) && isFinite(n);
}

function getStateString() {

    var hostState="";
    var hostTemp = o.devices.sort(function(a,b) {return (a.name > b.name) ? 1 : ((b.name > a.name) ? -1 : 0);} );

    for (var i=0; i<hostTemp.length; i++) { 
      hostState+=(hostTemp[i].state == true ? "1":"0");
    }

    return (hostState);

}
