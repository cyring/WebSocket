# WebSocket
## Purpose
This is an example of a WebSocket server written in C using the [libwebsockets library](https://github.com/warmcat/libwebsockets).
Every second, the server pushes the Linux system information toward the Javascript clients.
Any client can suspend the server activity whom state is broadcasted to other clients.
A rudimentary JSON composes the bidirectional message protocol.
## Build
Just enter `make` or rebuild as below
```
make clean all
```
## Run
 1- Execute the server
```
./bin/WebSocket [port]
```
 2- Open a browser and point to the server url with a given hostname:port
```
http://localhost:8080
```
 3- You can also start the client script with the browser shortcut [CTRL]+[O]
```
WebClient.html
```

## Enjoy
_`CyrIng`_

 Paris ;-)
