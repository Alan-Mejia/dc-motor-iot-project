#include <Arduino.h>
#include <WiFiMulti.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <timer.h>


#define WIFI_SSID "HUAWEI-106V40"
#define WIFI_PASSWORD "NewPwds*/@12"

#define WEBSERVER_HOST "192.168.3.10"//IP address of backend server
#define WEBSERVER_PORT 8080 // Port of backend service
#define STOMP_SERVER_ENDPOINT "/iot-dc-motor-controller/"//the endpoint to subscribe to stomp server

#define JSON_DOCUMENT_SIZE 2040
#define DEVICE_NAME "alancho"
#define REACT_SERVER_NAME "react-ui"

#define EACH_REQUEST_CHANNEL "/iot-websocket/"
#define EACH_SUBSCRIPTION_PREFIX "/connection/"

#define CHECK_CONNECTION_CHANNEL "status/"
#define DEVICE_SYSTEM_INFO_CHANNEL "device-system-info/"
#define CHANGE_RPM_CHANNEL "change-rpms/"

int motorPinOutput = 5;
int maximumMotorRpm = 7000; // "Here we have to specify the maximum number of revolution of the motor at max supported voltage"
int pwm = 0;
int currentRpm = (pwm * maximumMotorRpm) / 255;

WebSocketsClient webSocketsClient;
uniuno:: Timer timer;


void connectToWebSocket();
void handleWebSocketEvent(WStype_t type, uint8_t *payload, size_t length); // To recieve the paramets of any message in this callback
void subscribeToChannel(String channelName, String deviceListening);
void processJsonDataInMessageRecieved(String messageRecieved);
String extractObjectFromMessage(String messageRecieved);
void startControlSystem();
void sendMessage(String channelName, String payload, String deviceListening);


void setup() {
  Serial.begin(921600);
  pinMode(LED_BUILTIN, OUTPUT); //To turn on blue led for wifi connection
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED){// If we have not stablished a connection yet we are going to try to connect over and over again
    Serial.println("Trying to connect to wifi again...");
    delay(100);
  }

  Serial.println("Connected to wifi");

  pinMode(motorPinOutput, OUTPUT);

  connectToWebSocket();
  timer.set_interval(startControlSystem, 1000); // We are going to send the information 5 each second
  timer.attach_to_loop();
  
}

void loop() {
  digitalWrite(LED_BUILTIN, WiFi.status() == WL_CONNECTED); // To show the blue led when wifi connection is stablished
  webSocketsClient.loop();
  timer.tick();
}

void connectToWebSocket(){
  //Building the format of transport request URL http://host:port/myApp/myEndpoint/{server-id}/{session-id}/{transport}
  String urlFormat = STOMP_SERVER_ENDPOINT;
  urlFormat += random(0,999); //server id choosen by the client
  urlFormat += "/";
  urlFormat += random(0,999999); //session id this must be a unique value for all the clients
  urlFormat += "/websocket"; // To learn more about the format of URL request visit: "https://sockjs.github.io/sockjs-protocol/sockjs-protocol-0.3.3.html"

  webSocketsClient.begin(WEBSERVER_HOST, WEBSERVER_PORT, urlFormat);
  webSocketsClient.setExtraHeaders();
  webSocketsClient.onEvent(handleWebSocketEvent);



}

void handleWebSocketEvent(WStype_t type, uint8_t *payload, size_t length){
  switch (type){
  case WStype_DISCONNECTED:
    Serial.println("Desconnected from websocket...");
    break;
  case WStype_CONNECTED: // 2nd if the crendtial are correct then we response with a payload
    {
      Serial.printf("[Open session message *from server ]: %s \n", payload);  
    }
    break;
  case WStype_TEXT:
    {
      String text = (char*) payload;
      if (payload[0] == 'h'){// In case of heartbeat
        Serial.println("Heartbeat!");
      }else if(payload[0] == 'o'){// 1st we send the connection message
        String connectMessage = "[\"CONNECT\\naccept-version:1.1,1.0\\nheart-beat:1000,1000\\n\\n\\u0000\"]";
        webSocketsClient.sendTXT(connectMessage);
        delay(100);
      }else if (text.startsWith("a[\"CONNECTED")){ // Inmediately when we stablish the connection with server, we have to susbcribe to some channels to start to revieve messages
        String deviceId = String(DEVICE_NAME);
        subscribeToChannel(String(CHECK_CONNECTION_CHANNEL), deviceId); // To check if the dive is connected to wifi and its current status working/not working
        delay(500);
        subscribeToChannel("deviceinfo/", deviceId); // To check the current status of the system
        delay(500);
        subscribeToChannel(String(CHANGE_RPM_CHANNEL), deviceId); // To mange how many rpsm we want the motor works
        delay(500);

      }else if (text.startsWith("a[\"MESSAGE")){ // This block will be executed whenever we recieve a message from stomp server
        processJsonDataInMessageRecieved(text);
      }
    }
    break;  

  }
}

void subscribeToChannel(String channelName, String deviceListening){
  String subscribeMessage = "[\"SUBSCRIBE\\nid:sub-0\\ndestination:" + String(EACH_SUBSCRIPTION_PREFIX) + channelName + deviceListening + "\\n\\n\\u0000\"]";
  Serial.println("Subscripcion: " + subscribeMessage);
  webSocketsClient.sendTXT(subscribeMessage);
}

void processJsonDataInMessageRecieved(String messageRecieved){
  String jsonObject = extractObjectFromMessage(messageRecieved);
  jsonObject.replace("\\","");
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, jsonObject);

  JsonObject contentRecieved = doc.as<JsonObject>();

  Serial.println(jsonObject); // Este esta dando null

  // We will only attend the requests that como from UI
  if (strcmp(contentRecieved["from"], "react-ui-alancho") == 0 && strcmp(contentRecieved["to"], "alancho") == 0){
    // Execute the order recieved from ui
    Serial.println("DENTRO DE LA PRIMER CONDICIONAL");
    if(strcmp(contentRecieved["action"], "changeRpm") == 0){
      Serial.println("Dentro de change rpms");
      currentRpm = int(contentRecieved["rpmDesired"]);
      pwm = int((currentRpm * 255)/maximumMotorRpm);
    }


  }else{
    Serial.println("Messaje recieved "+ jsonObject);
  }
  


}

String extractObjectFromMessage(String messageRecieved){
  char startingChar = '{';
  char finishingChar = '}';

  String tmpData = "";
  bool _flag = false;
  for (int i = 0; i < messageRecieved.length(); i++) {
    char tmpChar = messageRecieved[i];
    if (tmpChar == startingChar) {
      tmpData += startingChar;
      _flag = true;
    }
    else if (tmpChar == finishingChar) {
      tmpData += finishingChar;
      break;
    }
    else if (_flag == true) {
      tmpData += tmpChar;
    }
  }

  return tmpData;
}

//To send the information of the system to UI
void startControlSystem(){
    //Serial.println(pwm);
    analogWrite(motorPinOutput, pwm);
    
    String deviceId = String(DEVICE_NAME);
    String action = "systemInfo";
    String to = "react-ui-alancho";
    float voltageOutput = (pwm * 3.3)/255;

    String deviceInfoPayload = "{\\\"from\\\":\\\"" +
                                    deviceId + "\\\",\\\"to\\\":\\\"" +
                                    to + "\\\",\\\"action\\\":\\\"" +
                                    action + "\\\",\\\"pinVoltageOutput\\\":\\\"" +
                                    voltageOutput + "\\\",\\\"rpmWorking\\\":\\\"" +
                                    currentRpm + "\\\"}";
              
    sendMessage(String(DEVICE_SYSTEM_INFO_CHANNEL), deviceInfoPayload, String(DEVICE_NAME));                
}

void sendMessage(String channelName, String payload, String deviceListening){
  String message = "[\"SEND\\ndestination:" + String(EACH_REQUEST_CHANNEL) + channelName + deviceListening + "\\n\\n" + payload + "\\n\\n\\u0000\"]";
  webSocketsClient.sendTXT(message);
}
