#include <SPI.h>
#include <WiFi101.h>
#include <NewPing.h>
#include <Arduino_JSON.h>

#include "arduino_secrets.h" 
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;        // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                 // your network key Index number (needed only for WEP)

// Static IP
IPAddress staticIP(192,168,0,30);
IPAddress GatewayIP(192,168,0,1);
IPAddress DNSIP(8,8,8,8);
IPAddress SubnetMask(255,255,255,0);

int status = WL_IDLE_STATUS;
WiFiServer server(80);

// LED Lights
const int LED_RED_PIN = 8;
const int LED_YELLOW_PIN = 7;
const int LED_GREEN_PIN = 6;

// Distance sensor

const int SONAR_NUM = 3; // Number of sensors.
const int PING_INTERVAL = 50; // Milliseconds between sensor pings (29ms is about the min to avoid cross-sensor echo).
const int ITERATIONS_PINGS = 50;
const int MAX_DISTANCE = 400;

unsigned long pingTimer[SONAR_NUM]; // Holds the times when the next ping should happen for each sensor.
unsigned int cm[SONAR_NUM];         // Where the ping distances are stored.
uint8_t currentSensor = 0;          // Keeps track of which sensor is active.

NewPing sonars[SONAR_NUM] = {     // Sensor object array.
  NewPing(0, 1, MAX_DISTANCE), // Each sensor's trigger pin, echo pin, and max distance to ping.
  NewPing(11, 12, MAX_DISTANCE),
  NewPing(4, 5, MAX_DISTANCE)
};

NewPing sonar(1,2,300);

int measuredValues[SONAR_NUM] = {0,0,0};

int tolerance[SONAR_NUM][2] = {
  {5,50},
  {5,50},
  {5,50}
};

const int FOOD_STATE_LOW = 0;
const int FOOD_STATE_CRITICAL = 1;
const int FOOD_STATE_NORMAL = 2;

int foodState = FOOD_STATE_NORMAL;

void setup() {
  Serial.begin(9600);      // initialize serial communication
  Serial.println("Controller started");

  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_YELLOW_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);

  pingTimer[0] = millis() + 75;           // First ping starts at 75ms, gives time for the Arduino to chill before starting.
  for (uint8_t i = 1; i < SONAR_NUM; i++) // Set the starting time for each sensor.
    pingTimer[i] = pingTimer[i - 1] + PING_INTERVAL;

  ConnectToWiFi();
  server.begin();
  printWiFiStatus();
  //Test to see if we can get an accessToken
  //sslClient.setInsecure(BearSSLClient::SNI::Insecure);
}

void ConnectToWiFi(){
  status = WiFi.status();
 // check for the presence of the shield:
  if (status == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    while (true);       // don't continue
  }

  bool JustConnected = false;
  // attempt to connect to WiFi network:
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);                   // print the network name (SSID);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);


    // wait 2 seconds for connection:
    delay(2000);
    if(status == WL_CONNECTED){
      JustConnected = true;
    }
  }
  if(JustConnected){
    WiFi.config(staticIP,DNSIP,GatewayIP,SubnetMask); // Set static ip
  }


}

void loop() {
  //ConnectToWiFi();
  //CheckForHTTPClient();
  Measure();
  //StateCheck();
}

void StateCheck(){
  int lightColor = 3;
  int _tempFoodState = FOOD_STATE_NORMAL;  

  for(int i = 0; i < SONAR_NUM; i++){
    if(measuredValues[i] < 15){
      lightColor = 1;
      _tempFoodState = FOOD_STATE_LOW;
      break;
    }else if (measuredValues[i] > 15 && measuredValues[i] < 25 && lightColor == 3){
      lightColor = 2;
      _tempFoodState = FOOD_STATE_CRITICAL;
    }
  }
  foodState = _tempFoodState;
  SetLight(lightColor);
}

void Measure(){
  
  for (int i = 0; i < SONAR_NUM; i++) {
    measuredValues[i] = CalculatePrecent(i,GetDistance(i));
    delay(PING_INTERVAL);
  }
  /*
  for (uint8_t i = 0; i < SONAR_NUM; i++) { // Loop through all the sensors.
    if (millis() >= pingTimer[i]) {         // Is it this sensor's time to ping?
      pingTimer[i] += PING_INTERVAL * SONAR_NUM;  // Set next time this sensor will be pinged.
      if (i == 0 && currentSensor == SONAR_NUM - 1) oneSensorCycle(); // Sensor ping cycle complete, do something with the results.
      sonars[currentSensor].timer_stop();          // Make sure previous timer is canceled before starting a new ping (insurance).
      currentSensor = i;                          // Sensor being accessed.
      cm[currentSensor] = 0;                      // Make distance zero in case there's no ping echo for this sensor.
      sonars[currentSensor].ping_timer(echoCheck); // Do the ping (processing continues, interrupt will call echoCheck to look for echo).
    }
  }
  */
}

void echoCheck() { // If ping received, set the sensor distance to array.
  if (sonars[currentSensor].check_timer())
    cm[currentSensor] = sonars[currentSensor].ping_result / US_ROUNDTRIP_CM;
}

void oneSensorCycle() { // Sensor ping cycle complete, do something with the results.
  // The following code would be replaced with your code that does something with the ping results.
  for (uint8_t i = 0; i < SONAR_NUM; i++) {
    Serial.print(i);
    Serial.print("=");
    Serial.print(cm[i]);
    Serial.print("cm ");
  }
  Serial.println();
}

void CheckForHTTPClient(){
  WiFiClient client = server.available();   // listen for incoming clients

  Serial.println("Checking for client");
  if (client) {                             
    Serial.println("new client");
    // To see if the client has any data in the request body
    bool currentLineIsBlank = true;
    String currentLine = "";    
    while (client.connected()) {
      if (client.available()) {            
        char c = client.read();             
        Serial.write(c);
        if (c != '\r' && c != '\n') {
          currentLine += c;
        }

        if (currentLine.endsWith("GET /GetFoodPercent")) {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: application/json");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println();


          client.println(CreateJSONString());

          client.println();
          break;
        }

        if (currentLine.endsWith("GET /GetFoodState")) {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: application/json");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println();


          client.println("{\"FoodState\": "  + String(foodState) + " }");

          client.println();
          break;
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("");
    Serial.println("client disonnected");
  }


}

String CreateJSONString(){
  String json = "{";

  for(int i = 0; i < SONAR_NUM; i++){
 
    json = String(json + "\"Container");
    json = String(json + (i + 1));
    json = String(json + "\": { \"precent\": ");
    json = String(json + measuredValues[i]);
    json = String(json + "}");

    if(i+1 != SONAR_NUM){
      json = String(json + ", ");
    }


  }
  json = String(json + "}");

  return json;
}

int GetDistance(int index){
  int timeSpent = sonars[index].ping_median(ITERATIONS_PINGS);
  int distance = sonars[index].convert_cm(timeSpent);
  
  //int distance = sonars[index].ping_cm();
  
  return distance;
}

int CalculatePrecent(int index, float distance){

  Serial.print("Distance: ");
  Serial.println(distance);

  if(distance == 0){
    return 0;
  }

  // Small distance means that food is close
  if(distance < tolerance[index][0]){
    return 100;
  }

  // Long distance means that food is gone
  if(distance > tolerance[index][1]){
    return 0;
  }

  // Calcualte the precent of the container
  return ((tolerance[index][1] - distance)/(tolerance[index][1] - tolerance[index][0])) * 100;
}

void SetLight(int color){
  switch (color) {
    case 1:
      digitalWrite(LED_RED_PIN, HIGH);
      digitalWrite(LED_YELLOW_PIN, LOW);
      digitalWrite(LED_GREEN_PIN, LOW);
      break;
    case 2:
      digitalWrite(LED_RED_PIN, LOW);
      digitalWrite(LED_YELLOW_PIN, HIGH);
      digitalWrite(LED_GREEN_PIN, LOW);
      break;
    case 3:
      digitalWrite(LED_RED_PIN, LOW);
      digitalWrite(LED_YELLOW_PIN, LOW);
      digitalWrite(LED_GREEN_PIN, HIGH);
      break;
  }

  Serial.print("Light Status: ");
  Serial.println(color);
  
}

void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);

  IPAddress Gateway = WiFi.gatewayIP(); 
  Serial.print("Gateway IP: ");
  Serial.println(Gateway);

  IPAddress subnetMask = WiFi.subnetMask();
  Serial.print("Subnet: ");
  Serial.println(subnetMask);

  Serial.print("Status: ");
  Serial.println(WiFi.status());

}

bool checkTime(int* lastCheck){
  unsigned long currentTime = millis();
  if (currentTime - *lastCheck >= 10 * 60 * 1000UL) {  // Check if 2 hours have passed                                     // Get time from the internet every 2 hours
    *lastCheck = currentTime;
    return true;
  }
  return false;
}
