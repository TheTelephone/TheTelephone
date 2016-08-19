/**
@file nodejs_websocket_server_message.js
@author Dennis Guse, Frank Haase
@date 2016-08-19
@license GPLv3 or later

A small websocket server for demo purposes of websocket_recv.
Starts at port 8080 and on client-connect sends immediately two message (see last two lines).

Uses [ws](https://www.npmjs.com/package/ws):

```bash
npm install ws
```

Start with:

```bash
nodejs nodejs_websocket_server_message.js
```

*/

var wss = new (require('ws')).Server({port: 8080});
console.log("Server Listening on port " + wss.options["port"]);
console.log("===============================================");

wss.on('connection', function (ws){
    ws.origin = ws.upgradeReq.headers['sec-websocket-key'];

    console.log("The client "+ ws.origin + " has just connected. ");

    ws.on('message', function (msg) {
        console.log("User '" + ws.origin + "' sent the message '" + msg + "'");
    }.bind(this));

    ws.on('close', function (){
        console.log("The client "+ ws.origin + " has just disconnected. ");
    }.bind(this));

    ws.send('{"key": 3.14}');
    ws.send('{"key": "Some useful message might be handy"}');
});
