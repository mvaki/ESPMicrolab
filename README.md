# ESPMicrolab
Firmware for ESP8266 to get commands through Serial. 

# Commands
All commands should start with "ESP8266:"
This is in case there are multiple things connected to the Serial port to avoid crosstalking
| Command | Description |
| ------------- | ------------- |
|addSensor:"SensorName"      | Add a sensor Name |
|APStart                     | Uses the provided SSID and Password to create an Access Point |
|baudrate:"Your baudrate"    | Resets the Serial to the required speed|
|configuration               | Reports your current configuration|
|clientTransmit              | Sends all SensorValues to the connected server|
|connect                     | Uses the provided SSID and Password to connect to the WiFi network|
|debug:"true" or "false      | Every command will send extra debug information if set to true|
|help                        | Lists all commands|
|getAllValues                | Sends the values of all Sensors|
|getValue:"SensorName"       | Send the value of the specified Sensor|
|hostIP:"Your HostIP"        | Your Host's IP address|
|password:"Your Password"    | Password to be used for the connection|
|payload:[Your payload]      | The payload to post to the Host. Must be in brackets|
|ready:"true" or "false"     | Your Team's status|
|restart                     | Restarts the ESP|
|sensorValue:"Sensor"[Value] | Saves value to specified sensor. Values shouldn't contain '=' and ',' |
|ssid:"Your SSID"            | SSID to be used for the connection|
|teamname:"Your Teamname"    | Your Team's name|
|transmit                    | Transmits the specified payload to the url|
|url:"Your url"              | The url to transmit to|
|getMAC                      | Prints MAC address to Serial|
|NOWstart                    | Starts ESP-NOW communication|
|NOWstop                     | Stops ESP-NOW communication|
|NOWsetRole:"Role"           | Sets device's role (either "Sender" or "Receiver"). Default role is "COMBO"|
|NOWaddPeer:"MAC"            | Adds paired device with specified MAC address|
|NOWmessage:"MAC"[message]   | Sends message to particular peer|
|NOWbroadcast                | Sends all sensor values to peers |
|NOWsave:"true" or "false"   | Saves the sensor data it receives|

Example

````ESP8266:debug:"true"````
