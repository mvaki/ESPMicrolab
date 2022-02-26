/*
 *  Instruction index:
 *  
    addSensor:"SensorName"      -> Add a sensor Name
    APStart                     -> Uses the provided SSID and Password to create an Access Point
    baudrate:"Your baudrate"    -> Resets the Serial to the required speed
    configuration               -> Reports your current configuration
    clientTransmit              -> Sends all SensorValues to the connected server
    connect                     -> Uses the provided SSID and Password to connect to the WiFi network
    debug:"true or false"       -> Every command will send extra debug information if set to true
    help                        -> Lists all commands
    getAllValues                -> Sends the values of all Sensors
    getValue:"SensorName"       -> Send the value of the specified Sensor
    hostIP:"Your HostIP"        -> Your Host's IP address
    password:"Your Password"    -> Password to be used for the connection
    payload:[Your payload]      -> The payload to post to the Host. Must be in brackets
    ready:"true or false"       -> Your Team's status
    restart                     -> Restarts the ESP
    sensorValue:"Sensor"[Value] -> Saves value to specified sensor. Values shouldn't contain '=' and ',' 
    ssid:"Your SSID"            -> SSID to be used for the connection
    teamname:"Your Teamname"    -> Your Team's name
    transmit                    -> Transmits the specified payload to the url
    url:"Your url"              -> The url to transmit to

    getMAC                      -> Prints MAC address to Serial
    NOWstart                    -> Starts ESP-NOW communication
    NOWstop                     -> Stops ESP-NOW communication
    NOWsetRole:"Role"           -> Sets device's role (either "Sender" or "Receiver"). Default role is "COMBO"
    NOWaddPeer:"MAC"            -> Adds paired device with specified MAC address
    NOWmessage:"MAC"[message]   -> Sends message to particular peer
    NOWbroadcast                -> Sends all sensor values to peers 
*
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <espnow.h>

// Setup the connection defaults
#ifndef DefaultSSID
#define DefaultSSID "Microlab_IoT"
#define DefaultPassword "microlab"
#define DefaultHttpPort 80 //int
#define DefaultHostIP "192.168.4.1"
#endif


String ssid = DefaultSSID;
String password = DefaultPassword;
String hostIP = DefaultHostIP;
int httpPort = DefaultHttpPort;
String response = "";
int baud = 9600;
String teamname = "No team";
String url = "http://" + hostIP + "/api/students/" + teamname + "/sensors";
String sensorName[10] = {"","","","","","","","","",""};  // Up to 10 sensors can be added
String sensorValue[10] = {"","","","","","","","","",""}; // The value of each sensor is stored here
int sensorCount = 0;
bool connectionStarted = false;
bool serverOnline = false;
String receivedPayload = "[]", inputString = "", isReady = "false";
bool debug = false, LEDState = false;
bool newValues = false;

String NOWvalue = "";

WiFiServer server(httpPort);
bool serverMode = false;

//client for access point additions
WiFiClient wificlient;

typedef struct message {
    char message_info[32];
    int value;
} message;

// Create a struct_message called myData
message sent;
message received;

// setup, loop =============================================================================================================================

void setup() {
    pinMode(2, OUTPUT); // Testing LED is connected on 2
    Serial.begin(baud);

    WiFi.mode(WIFI_STA);

    Serial.println();
    Serial.print("ESP8266 Board MAC Address:  ");
    Serial.println(WiFi.macAddress());
  
    Serial.print("\nESP8266: Waiting for command\n");
}


void loop() {
    ReadData();             // Reads Data
    LEDState = !LEDState;   // The program gets here when newline is read to change the LED state
    digitalWrite(2, LEDState);
}


// Functions =============================================================================================================================

void ReadData() {   // Read and decode one line of data
    ReadDataLine();
    searchForCommand(inputString);
    inputString = "";
}


void ReadDataLine() {   // Read one line of data
    bool ReadLine = false;
    while (!ReadLine) {
        if (serverOnline) {
            WiFiClient wificlient = server.available();
            if (wificlient) {
                if (wificlient.connected()) serverReceiveData(wificlient); //this needs to be here to be excecuted all the time
                wificlient.stop();
            }
        }
        while (Serial.available() > 0) {
            char inputChar = Serial.read();
            inputString += inputChar;
            if (inputChar == '\n') ReadLine = true;
        }
    }
}

// ====================================================================================================================================

int ESP8266connectWiFi(String ssid, String password, bool STAMode) { // Handles the WiFi connection
    if (WiFi.status() == WL_CONNECTED) {
        if (debug) Serial.print("ESP8266: Connected\n");
        else Serial.print("\"Success\"\n");
        return 1;
    }
    int timecounter = 0;
    if (STAMode) WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (debug) Serial.print("ESP8266: Connecting");
    while (WiFi.status() != WL_CONNECTED) { // Check for the connection
        delay(2000);
        timecounter++;
        if (debug) Serial.print(".");
        if (timecounter > 10) {
            timecounter = 0;
            if (debug) Serial.print("ESP8266: Could not connect to network. Please check your credentials\n");
            else Serial.print("\"Fail\"\n");
            return 0;
        }
    }
    if (debug) Serial.print("Connected\n");
    else Serial.print("\"Success\"\n");
    return 1;
}


String getValue(String DataString, String valueName, char charBeforeValue, char charAfterValue) { // Finds the Value in a string formated valueName "Value"
    int startIndex, endIndex = 0;
    startIndex = DataString.indexOf(valueName, startIndex);
    if (startIndex >= 0) {
        startIndex = DataString.indexOf(charBeforeValue, startIndex + valueName.length());
        if (valueName == "url") endIndex = DataString.lastIndexOf(charAfterValue); // Url may contain "" inside so find last index
        else endIndex = DataString.indexOf(charAfterValue, startIndex + 1);
        if (startIndex >= 0 && endIndex > startIndex) {
            //if(endIndex=startIndex+1) Serial.println("\"Success\"");             // If string is really empty print success
            return DataString.substring(startIndex + 1, endIndex);
        } else {
            if (debug) Serial.print("ESP8266: Wrong data format for " + valueName + '\n');
            else Serial.print("\"Fail\"\n");
        }
        return "noValueFound";
    } else return "noValueFound";
}

void serverReceiveData(WiFiClient wificlient) { //Get data from the server
    String message = "", requestedData = ""; //,temp="";
    String incoming = wificlient.readStringUntil('\n');
    wificlient.flush();
    for (int i = 0; i < sensorCount; ++i) {
        String temp = getValue(incoming, sensorName[i], '=', ',');
        if (temp == "request") requestedData += sensorName[i] + "=" + sensorValue[i] + ",";  // If value is requested
        else if (temp != "noValueFound") {
            sensorValue[i] = temp;   // Else value is received
            newValues = true;
        }
        message += sensorName[i] + "=" + sensorValue[i] + ",";
    }
    if (requestedData != "") {
        if (debug) Serial.print("ESP8266: answering:" + requestedData + "\n");
        //server.send(200, "text/plain", "Data OK "+requestedData+"\n");
        //server.send(200, "text/plain", "Data OK ");
        //server.send(200, "text/plain", "Data OK FML\n ");
        wificlient.print(requestedData + "\n");
    } else wificlient.print("Data OK\n");
    //server.send(200, "text/html", "Data OK\n");
    if (debug) Serial.print("ESP8266:Values: " + message + "\n");
    else Serial.print("ServedClient\n");
}


inline void clientTransmitData(String url, String hostIP, int port) {
    // This will send the request to the server
    String toSend = "";
    for (int i = 0; i < sensorCount; ++i) {
        toSend += sensorName[i];
        toSend += "=";
        toSend += sensorValue[i];
        toSend += ",";
        //if(i!=(sensorCount-1))url+="&";
    }
    if (debug) Serial.print("ESP8266: Sending to server: " + toSend + "\n");
    wificlient.print(toSend + "\n");
    unsigned long timeout = millis();
    while (wificlient.available() == 0) {
        if (millis() - timeout > 5000) {
            if (debug) Serial.print("ESP8266: Client Timeout\n");
            else Serial.print("\"Fail\"\n");
            wificlient.stop();
            return;
        }
    }
    while (wificlient.available()) {
        String line = wificlient.readStringUntil('\n'); //read and print response
        if (line == "Data OK") break;
        for (int i = 0; i < sensorCount; ++i) {
            String temp = getValue(line, sensorName[i], '=', ',');
            if (temp != "noValueFound") {
                sensorValue[i] = temp; // Else value is received
            }
        }
        if (debug) Serial.print("ESP8266: Received from server:" + line + "\n");
        else Serial.print("\"Success\"\n");
        wificlient.flush();
        wificlient.stop();
    }
}


inline void transmitData(String url, String hostIP, String mypayload) { //Sends Data to the Server
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        // If something changes in the url here it should be also changed in the configuration command
        // url =  "http://" + hostIP + "/api/students/"+teamname+"/sensors/?isReady="+isReady;

        http.begin(wificlient, url);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("Accept", "application/json");
        int httpResponseCode = http.POST(mypayload);

        if (httpResponseCode > 0) {
            if (debug) {
                String response = http.getString();
                int heap = ESP.getFreeHeap();
                Serial.print("Server: HTTP code:" + (String) httpResponseCode + "\n" + "Server: " + response + '\n');
            } else { /////////////////////////////////////////////////////////////////////////////////////
                // For some reason if we receive http code 400 the ESP8266 crashes if we run http.getString(). 
                // This is why we set the server to only send http code 200 (which is normally wrong)
                // And the ESP to run http.getString() only if httpResponseCode == 200
                ////////////////////////////////////////////////////////////////////////////////////////
                //if(httpResponseCode==200) Serial.println("HTTP:" + (String)httpResponseCode + " " + http.getString());
                if (httpResponseCode == 200) Serial.print(http.getString() + '\n');
                else Serial.print("HTTP:" + (String) httpResponseCode + " BAD REQUEST\n");
            }
        } else {
            if (debug) Serial.print("ESP8266: Error on sending POST: ");
            else Serial.print("\"Fail on POST\"\n");
        }
        http.end();
    } else {
        if (debug) Serial.print("ESP8266: Error in WiFi connection\n");
        else Serial.print("\"Fail\"\n");
    }
}


void updateURL(String ReceivedURL, bool clientMode) { // If a url is set we use that one. Otherwise we built it from the data
    if (ReceivedURL == "") {
        if (clientMode) {
            url = "/data/?";
            for (int i = 0; i < sensorCount; ++i) {
                //if(sensorName[i]=="") break;      // No need since we have sensorCount
                url += sensorName[i];
                url += "=";
                url += sensorValue[i];
                if (i != (sensorCount - 1)) url += "&";
            }
        } else url = "http://" + hostIP + "/api/students/" + teamname + "/sensors/?isReady=" + isReady;
    } else url = ReceivedURL;
}

// ESP NOW =============================================================================================================================

// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  if (sendStatus == 0){
    Serial.print("\"Success\"\n");
  }
  else{
    Serial.print("\"Fail\"\n");
  }
}

void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&received, incomingData, sizeof(received));
  if (debug) {
    Serial.print("Bytes received: ");
    Serial.println(len);
    Serial.print("Info: ");
    Serial.println(received.message_info);
    Serial.print("Value: ");
    Serial.println(received.value);
  }
  Serial.print("\"Success\"\n");
  // Forward message to other peers
  int stat = esp_now_send(0, (uint8_t *) &received, sizeof(received));
  if (stat != 0) {
    if (debug) Serial.println("Failed to forward received message");
    return;
  }
  if (debug) Serial.println("Successfully forwarded message");
}

// =====================================================================================================================================

void searchForCommand(String DataString) {    // Looks for a command in a line of Data
    if (!(DataString.indexOf("ESP:") >= 0)) { // If sentence doesn't contain ESP: don't answer
        if (debug) Serial.print("ESP8266: should start with ESP:\n");
    } else if (DataString.indexOf("APStart") >= 0) { // Make sure values are set
        // Need server ssid, server pass
        WiFi.softAP(ssid, password);
        IPAddress myIP = WiFi.softAPIP();
        server.begin();
        serverOnline = true;
        if (debug) {
            Serial.print("ESP8266: Server IP=");
            Serial.println(myIP);
        } else Serial.print("\"Success\"\n");
    } else if (DataString.indexOf("addSensor:") >= 0) { // Make sure values are set
        // Need server ssid, server pass
        String temp = getValue(DataString, "addSensor", '\"', '\"');
        for (int i = 0; i < sensorCount; ++i) {
            if (temp == sensorName[i]) {
                if (debug) Serial.print("ESP8266: sensor already exists!\n");
                else Serial.print("\"Fail\"\n");
                return;
            }
        }
        sensorName[sensorCount] = temp;
        sensorCount++;
        if (debug) Serial.print("ESP8266: Sensor " + temp + " added" + '\n');
        else Serial.print("\"Success\"\n");
    } else if (DataString.indexOf("baudrate:") >= 0) {
        if (getValue(DataString, "baudrate:", '\"', '\"') != "noValueFound") {
            baud = getValue(DataString, "baudrate:", '\"', '\"').toInt();
            if (debug) Serial.print("ESP8266: Changing baud rate to " + (String) baud + '\n');
            else Serial.print("\"Success\"\n");
            delay(100); // Wait for last characters to be transmitted
            Serial.end();
            Serial.begin(baud);
        }
    } else if (DataString.indexOf("configuration") >= 0) {
        String temp = "ESP8266: Configuration:\n";
        temp += "Baudrate: " + (String) baud + "\n";
        temp += "YourIP: " + WiFi.localIP().toString() + "\n";
        temp += "SSID: " + ssid + "\n";
        temp += "Password: " + password + "\n";
        temp += "Teamname: " + teamname + "\n";
        temp += "hostIP: " + hostIP + "\n";
        temp += "url: " + url + "\n";
        temp += "Payload: " + receivedPayload + "\n";
        Serial.print(temp);
    } else if (DataString.indexOf("clientTransmit") >= 0) {
        if (!wificlient.connect(hostIP, httpPort)) {
            if (debug) Serial.println("ESP8266: client not connected");
            else Serial.print("\"Fail\"\n");
        } else if (url == "") {
            if (sensorCount == 0) {
                if (debug) Serial.print("ESP8266: No sensors added and no url provided\n");
                else Serial.print("\"Fail\"\n");
            }
        } else {
            clientTransmitData(url, hostIP, httpPort);
        }
    } else if (DataString.indexOf("connect") >= 0) {
        if (ESP8266connectWiFi(ssid, password, true)) connectionStarted = true;
        else connectionStarted = false;
    } else if (DataString.indexOf("debug:") >= 0) {
        String debugString = getValue(DataString, "debug:", '\"', '\"');
        if (debugString == "true") debug = true;
        else if (debugString == "false") debug = false;
        else Serial.print("\"Fail\"\n");
        Serial.print("ESP8266: Debug set to:" + debugString + '\n');
    } else if (DataString.indexOf("disconnect:") >= 0) {
        WiFi.disconnect();
        WiFi.mode(WIFI_OFF);
    } else if (DataString.indexOf("getValue:") >= 0) {
        String temp = getValue(DataString, "getValue:", '\"', '\"'); //sensorName
        bool found = false;
        for (int i = 0; i < sensorCount; ++i) {
            if (temp == sensorName[i]) {
                found = true;
                newValues = false; // Caution! If just one is read successful we assume the values have been read.
                if (debug) Serial.print("ESP8266: " + temp + "=\"" + sensorValue[i] + "\"\n");
                else Serial.print(temp + "=\"" + sensorValue[i] + "\"\n");
            }
        }
        if (!found) Serial.print(temp + "=\"not found" + "\"\n");
    } else if (DataString.indexOf("getAllValues") >= 0) {
        newValues = false;
        for (int i = 0; i < sensorCount; ++i) {
            if (debug) Serial.print("ESP8266: " + sensorName[i] + "=\"" + sensorValue[i] + "\"\n");
            else Serial.print(sensorName[i] + "=\"" + sensorValue[i] + "\"\n");
        }
    } else if (DataString.indexOf("help") >= 0) {
        String temp = "ESP8266: commands list:\n";
        temp += "-> General and WiFi commands:\n";
        temp += "addSensor:\"SensorName\"  #Add a sensor Name\n";
        temp += "APStart  #Uses the provided SSID and Password to create an Access Point\n";
        temp += "baudrate:\"Your baudrate\"  #Resets the Serial to the required speed\n";
        temp += "configuration  #Reports your current configuration\n";
        temp += "clientTransmit  #Sends all SensorValues to the connected server\n";
        temp += "connect  #Uses the provided SSID and Password to connect to the WiFi network\n";
        temp += "debug:\"true or false\"  #Every command will send extra debug information if set to true\n";
        temp += "help  #Lists all commands\n";
        temp += "getAllValues  #Sends the values of all Sensors\n";
        temp += "getValue:\"SensorName\"  #Send the value of the specified Sensor\n";
        temp += "hostIP:\"Your HostIP\"  #Your Host's IP address\n";
        temp += "password:\"Your Password\"  #Password to be used for the connection\n";
        temp += "payload:[Your payload]  #The payload to post to the Host. Must be in brackets\n";
        temp += "ready:\"true or false\"  #Your Team's status\n";
        temp += "restart  #Restarts the ESP\n";
        temp += "sensorValue:\"SensorName\"[SensorValue]  #Saves value to specified sensor. Values shouldn't contain = and , \n";
        temp += "ssid:\"Your SSID\"  #SSID to be used for the connection\n";
        temp += "teamname:\"Your Teamname\"  #Your Team's name\n";
        temp += "transmit  #Transmits the specified payload to the url\n";
        temp += "url:\"Your url\"  #The url to transmit to. Example: http:\\\\192.168.1.2:8382\\service\n";
        temp += "#If you want to set up your own url don't use the commands teamname, HostIP, ready\n";
        temp += "-> ESP-NOW commands:\n";
        temp += "getMAC  #Prints MAC address to Serial\n";
        temp += "NOWstart  #Starts ESP-NOW communication\n";
        temp += "NOWstop  #Stops ESP-NOW communication\n";
        temp += "NOWsetRole:\"Role\"  #Sets device's role (either \"Sender\" or \"Receiver\"). Default role is peer\n";                     
        temp += "NOWaddPeer:[MAC]  #Adds paired device with specified MAC address\n";
        temp += "NOWmessage:\"MAC\"[message]  #Sends message to particular peer\n";
        temp += "NOWbroadcast  #Sends all sensor values to peers\n";
        Serial.print(temp); // Minimize Serial transmits
    } else if (DataString.indexOf("hostIP:") >= 0) {
        hostIP = getValue(DataString, "hostIP:", '\"', '\"');
        if (hostIP != "noValueFound") {
            updateURL("", false);
            if (debug) Serial.print("ESP8266: hostIP: " + hostIP + '\n');
            else Serial.print("\"Success\"\n");
        }
    } else if (DataString.indexOf("password") >= 0) { //extract password
        password = getValue(DataString, "password:", '\"', '\"');
        if (password != "noValueFound") {
            if (debug) Serial.print("ESP8266: Password: " + password + '\n');
            else Serial.print("\"Success\"\n");
        }
    } else if (DataString.indexOf("payload:") >= 0) {
        receivedPayload = "[" + getValue(DataString, "payload:", '[', ']') + "]";
        if (receivedPayload != "[noValueFound]") {
            if (debug) Serial.print("ESP8266: receivedPayload: " + receivedPayload + '\n');
            else Serial.print("\"Success\"\n");
        }
    } else if (DataString.indexOf("ready:") >= 0) {
        isReady = getValue(DataString, "ready:", '\"', '\"');
        if (isReady != "noValueFound") {
            if ((isReady != "true") && (isReady != "false")) {
                if (debug) Serial.print("ESP8266: Should be ready: \"true\" or ready: \"false\"\n");
                else Serial.print("\"Fail\"\n");
                isReady = "false";
            } else {
                updateURL("", false);
                if (debug) Serial.print("ESP8266: ready set to:" + isReady + '\n');
                else Serial.print("\"Success\"\n");
            }
        }
    } else if (DataString.indexOf("restart") >= 0) {      // Extract ssid
        ESP.restart();
    } else if (DataString.indexOf("sensorValue:") >= 0) { //sensorValue:"Temperature"=[23.78]
        // Need server ssid, server pass
        String temp = getValue(DataString, "sensorValue:", '\"', '\"'); //sensorName
        for (int i = 0; i < sensorCount; ++i) {
            if (temp == sensorName[i]) {
                sensorValue[i] = getValue(DataString, sensorName[i], '[', ']'); //sensorValue
                if (debug) Serial.print("ESP8266: " + sensorValue[i] + " value for " + sensorName[i] + "\n");
                else Serial.print("\"Success\"\n");
                updateURL("", true);
                return;
            }
        }
        if (debug) Serial.print("ESP8266: Sensor " + temp + " not found" + '\n');
        else Serial.print("\"Fail\"\n");
    } else if (DataString.indexOf("ssid:") >= 0) { //extract ssid
        if (ssid != "noValueFound") {
            ssid = getValue(DataString, "ssid:", '\"', '\"');
            if (debug) Serial.print("ESP8266: SSID: " + ssid + '\n');
            else Serial.print("\"Success\"\n");
        }
    } else if (DataString.indexOf("teamname:") >= 0) {
        teamname = getValue(DataString, "teamname:", '\"', '\"');
        if (teamname != "noValueFound") {
            updateURL("", false);
            if (debug) Serial.print("ESP8266: Teamname set to: " + teamname + '\n');
            else Serial.print("\"Success\"\n");
        }
    } else if (DataString.indexOf("transmit") >= 0) {
        if (teamname == "No team" || teamname == "") {
            if (debug) Serial.print("ESP8266: You must set up your teamname first\n");
            else Serial.print("\"Fail\"\n");
            return;
        } else if (WiFi.status() != WL_CONNECTED) {
            if (debug) Serial.print("ESP8266: You must connect to a WiFi network first\n");
            else Serial.print("\"Fail\"\n");
            return;
        }
        if (debug) Serial.print("ESP8266: Transmitting Data\n");
        //else Serial.println("\"Success\"");
        transmitData(url, hostIP, receivedPayload);

    } else if (DataString.indexOf("url:") >= 0) {
        String ReceivedURL = getValue(DataString, "url:", '\"', '\"');
        if (ReceivedURL != "noValueFound") {
            updateURL(ReceivedURL, false); // Doesn't matter if client or not since we receive the url
            if (debug) Serial.print("ESP8266: URL set to: " + url + '\n');
            else Serial.print("\"Success\"\n");
        }
    } else if (DataString.indexOf("autoTransmit") >= 0) {
        Serial.print("ESP8266: Auto Transmiting Data\n");
        //int mycounter=0;
        do {
            //mycounter++;
            //receivedPayload.replace("\"Counter\",\"value\": 128","\"Counter\",\"value\": "+String(mycounter));
            transmitData(url, hostIP, receivedPayload);
            //receivedPayload.replace("\"Counter\",\"value\": "+String(mycounter),"\"Counter\",\"value\": 128");
            delay(1000);
        } while (Serial.available() <= 0);
    }
    
// =====================================================================================================================================    
    
    else if (DataString.indexOf("getMAC") >= 0) {
        Serial.print("ESP8266 Board MAC Address:  ");
        Serial.println(WiFi.macAddress());
    } else if (DataString.indexOf("NOWstart") >= 0) {
        if (debug) Serial.print("ESP8266: Starting ESP-NOW protocol in a peer-to-peer network...\n"); 
        if (esp_now_init() != 0) {
          if (debug) {
            Serial.println("Error initializing ESP-NOW");
          }
          else Serial.print("\"Fail\"\n");
        } else {
          esp_now_register_send_cb(OnDataSent);
          esp_now_register_recv_cb(OnDataRecv);
          // Peer to peer network
          esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
          Serial.print("\"Success\"\n");
        }
    } else if (DataString.indexOf("NOWstop") >= 0) {
        if (esp_now_deinit() != 0) {
          if (debug) {
            Serial.println("Error terminating ESP-NOW");
          }
          else Serial.print("\"Fail\"\n");
        } else {
          Serial.print("\"Success\"\n");
        }
    } else if (DataString.indexOf("NOWsetRole:") >= 0) {
        String role = getValue(DataString, "NOWsetRole:", '\"', '\"');
        int stat;
        if (role == "Sender") {
          stat = esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
          esp_now_register_send_cb(OnDataSent);
        }
        else if (role == "Receiver") {
          stat = esp_now_set_self_role(ESP_NOW_ROLE_SLAVE); 
          esp_now_register_recv_cb(OnDataRecv);
        }
        else {
          if (debug) Serial.print("ESP8266: Role should be either \"Sender\", or \"Receiver\".\n");
          else Serial.print("\"Fail\"\n");
          return;
        } 
        if (stat == 0) {
            if (debug) Serial.print("ESP8266: ESP-NOW role set to: " + role + '\n');
            else Serial.print("\"Success\"\n");
        }
        else {
            if (debug) Serial.print("ESP8266: Error setting role\n");
            else Serial.print("\"Fail\"\n");
        }
    } else if (DataString.indexOf("NOWmessage:") >= 0) {
        NOWvalue = getValue(DataString, "NOWmessage:", '[', ']');
        String input = "Input from Serial.";
        input.toCharArray(sent.message_info, sizeof(sent.message_info));
        sent.value = NOWvalue.toInt();
        // Send message to all peers via ESP-NOW
        esp_now_send(0, (uint8_t *) &sent, sizeof(sent));

        if (NOWvalue != "") {
            if (debug) Serial.print("ESP8266: sending message: " + NOWvalue + '\n');
            else Serial.print("\"Success\"\n");
        }
      
    } else if (DataString.indexOf("NOWbroadcast") >= 0) {
        int success = 1;
        for (int i = 0; i < sensorCount; ++i) {
            sensorName[i].toCharArray(sent.message_info, sizeof(sent.message_info));
            // Check for null sensorValue
            if (sensorValue[i] == "") {
              if (debug) Serial.print("sensorValue cannot be empty\n");
              else Serial.print("\"Fail\"\n");
              return;
            }
            sent.value = sensorValue[i].toInt();
            // Send message to all peers via ESP-NOW
            int temp = esp_now_send(0, (uint8_t *) &sent, sizeof(sent));
            success = success || temp;
        }
        if (success != 0) {
          Serial.print("\"Success\"\n");
        }
        else Serial.print("\"Fail\"\n");
    } else if (DataString.indexOf("NOWaddPeer:") >= 0) {
        String macstr = getValue(DataString, "NOWaddPeer:", '\"', '\"');
        uint8_t mac[6]; 
        char macchar[18];
        macstr.toCharArray(macchar, sizeof(macchar));
      
        char *ptr = macchar;
        char *next = NULL;
        // Convert char array to actual MAC address
        for (int i = 0; i < 6; i++) {
          int j = i + 2 + (i != 0);
          uint8_t ret = strtoul(ptr, &next, 16);
          strcpy(ptr, next + 1);
          mac[i] = ret;
        }
        // Add peer
        int stat = esp_now_add_peer(mac, ESP_NOW_ROLE_COMBO, 4, NULL, 0);
        if (stat == 0) {
            if (debug) Serial.print("ESP8266: Added peer with MAC address: " + macstr + '\n');
            else Serial.print("\"Success\"\n");
        }
        else {
            if (debug) Serial.print("ESP8266: Error adding peer with MAC address: " + macstr + '\n');
            else Serial.print("\"Fail\"\n");
        }
    }
}
