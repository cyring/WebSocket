# WebSocket
## Purpose
This is an example of a WebSocket server written in C using the [libwebsockets library](https://github.com/warmcat/libwebsockets).
Every second, the server pushes the Linux system information toward the Javascript clients.
Any client can suspend the server activity whom state is broadcasted to other clients.
A rudimentary JSON composes the bidirectional message protocol.
## Build
The program is delivered with a [Code::Blocks](http://www.codeblocks.org) project file.
## Run
 # Execute the server
```
./bin/Release/WebSocket
```
 # Open the client script file in a browser with shortcuts [CTRL]+[O]
```
WebClient.html
```
# Regards
_`CyrIng`_

 Paris ;-)
