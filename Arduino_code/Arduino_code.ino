#include <Adafruit_SleepyDog.h>
#include <Arduino_MKRENV.h>
#include <WiFi101.h>
#include <MQTT.h>
#include <MQTTClient.h>
//#include <vector>
#include "arduino_secrets.h"

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;
char mqttHost[] = SECRET_MQTT_HOST;
char mqttClientId[] = SECRET_MQTT_CLIENT_ID;
char mqttUser[] = SECRET_MQTT_USER;
char mqttPass[] = SECRET_MQTT_PASS;
unsigned long lastMillis = 0;

// vars for water heater control
float temp_sp = 0;
float temp_hyst = 0;
bool man_control = false;
bool man_value = false;
float temperature = 0;


//vars for relay push button

int pbuttonPin = 2;// connect output to push button
int relayPin = 10;// Connected to relay (LED)
int val = 0; // push value from pin 2
int lightON = 0;//light status
int pushed = 0;//push status


WiFiSSLClient net;
MQTTClient mqttClient;

struct outTopic
{
    String topicName;
    unsigned long lastUpgrade;
    String oldValue;
    String newValue;
};

//Vector to add all output topics, to check life time
//std::vector<outTopic> arrayOutTopics;

void printWiFiStatus()
{
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
}

void connectWiFi() {

    while (status != WL_CONNECTED) {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(ssid);
        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        status = WiFi.begin(ssid, pass);

        // wait 10 seconds for connection:
        delay(10000);
    }

}

void connectMqttServer()
{
    Serial.print("checking wifi...");
    if (WiFi.status() != WL_CONNECTED) { connectWiFi(); }

    // MQTT client connection request
    mqttClient.begin(mqttHost, 1883, net);
    Serial.print("\nconnecting to MQTT server...");
    while (!mqttClient.connect(mqttClientId, mqttUser, mqttPass))
    {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("\nconnected!");

    mqttClient.subscribe("homie/mkr1000/waterHeater/#");
}

void messageReceived(String &topic, String &payload)
{
    Serial.println("incoming: " + topic + " - " + payload);

    // New value for Manual control, read and save it
    if (topic.indexOf("manCntrl/set") > 0) { man_control = (payload == "true") ? true : false; }
    // New value for hysteresys temperature, read and save it
    if (topic.indexOf("hysteresis/set") > 0) { temp_hyst = payload.toFloat(); }
    // New value for temperature setpoint
    if (topic.indexOf("setpoint/set") > 0) { temp_sp = payload.toFloat(); }
    // New value for manual value
    if (topic.indexOf("/resistence/set") > 0) { man_value = (payload == "true") ? true : false; }

}


void manualactivation(){
  val = digitalRead(pbuttonPin);
  if(val == HIGH && lightON == LOW){
    pushed = 1-pushed;
    delay(100);
  }    

  lightON = val;
      if(pushed == HIGH){
        Serial.println("Light ON");
        digitalWrite(relayPin, LOW); 
       
      }else{
        Serial.println("Light OFF");
        digitalWrite(relayPin, HIGH);
   
      }     

  delay(100);
  }

void setup()
{
    //init push button ON,OFF
    pinMode(pbuttonPin, INPUT_PULLUP); 
    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, HIGH);
   
    // initialize digital pin 1 as an output for resistence control.
    pinMode(1, OUTPUT);

    int watch_dog = Watchdog.enable(180000);
    Serial.begin(9600);
    while (!Serial)
        ;

    Serial.print("Enabled the watchdog with max countdown of ");
    Serial.print(watch_dog, DEC);
    Serial.println(" milliseconds!");

    if (!ENV.begin())
    {
        Serial.println("Failed to initialize MKR ENV shield!");
        while (1)
            ;
    }
    Watchdog.reset();

    // check for the presence of the shield:
    if (WiFi.status() == WL_NO_SHIELD)
    {
        Serial.println("WiFi shield not present");
        // don't continue:
        while (true)
            ;
    }
    Watchdog.reset();

    // attempt to connect to WiFi network:
    connectWiFi();
    Watchdog.reset();

    printWiFiStatus(); // you're connected now, so print out the status:

    // MQTT client connection
    mqttClient.begin(mqttHost, net);
    mqttClient.onMessage(messageReceived);
    connectMqttServer();
    Watchdog.reset();

    //outTopic tmp = new outTopic();

    //tmp.topicName = "homie/mkrenv1/thermostat/temperature";
    //arrayOutTopics.push_back(tmp);

    //tmp = new outTopic();
    //tmp.topicName = "homie/mkr1000/waterHeater/resistance";
    //arrayOutTopics.push_back(tmp);

}



void loop() {
    //call manual push button relay
    manualactivation();
    
    // MQTT client:
    mqttClient.loop();

    if (!mqttClient.connected()) { connectMqttServer(); }

    if (!man_control) {
        if (temperature > temp_sp + temp_hyst) {
            digitalWrite(1, false);
        } else if (temperature < temp_sp - temp_hyst) {
            digitalWrite(1, true);
        }       
    } else {
        digitalWrite(1, man_value);
    }
    
    // publish a message roughly every second.
    if (millis() - lastMillis > 10000)
    {
        // read temperature sensor value
        temperature = ENV.readTemperature();
    
        String resistence_st = "false";
        String man_control_str = "false";

        if (digitalRead(1) == HIGH) { resistence_st = "true"; }
        if (man_control) { man_control_str = "true"; }
        
        mqttClient.publish("homie/mkr1000/waterHeater/temperature", String(temperature));
        mqttClient.publish("homie/mkr1000/waterHeater/resistence", resistence_st);
        mqttClient.publish("homie/mkr1000/waterHeater/setpoint", String(temp_sp));
        mqttClient.publish("homie/mkr1000/waterHeater/hysteresis", String(temp_hyst));
        mqttClient.publish("homie/mkr1000/waterHeater/manCntrl", man_control_str);

        lastMillis = millis();

    }

    Watchdog.reset();
}
