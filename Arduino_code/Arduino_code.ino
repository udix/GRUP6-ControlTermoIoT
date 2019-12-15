#include <Adafruit_SleepyDog.h>
#include <Arduino_MKRENV.h>
#include <WiFi101.h>
#include <MQTT.h>
#include <MQTTClient.h>
#include "arduino_secrets.h"
#include <ArduinoJson.h>
#include <FlashAsEEPROM.h>
#include <FlashStorage.h>
#include <string.h>
#include <cstring>

typedef struct {
    boolean valid;
    char mqttHost[50];
    int mqttPort;
    char mqttUser[50];
    char mqttPass[50];
} MQTTSvr_cred;

int status = WL_IDLE_STATUS;
char mqttClientId[] = SECRET_MQTT_CLIENT_ID;
unsigned long lastMillis = 0;

FlashStorage(myFS, MQTTSvr_cred);

MQTTSvr_cred myMQTTSvr;

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
WiFiServer HTTPserver(80);
MQTTClient mqttClient;

// Declare reset function
void(* resetFunc) (void) = 0;

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

void connectMqttServer() {

    if (WiFi.status() != WL_CONNECTED){ WiFi.begin(); }

    if (mqttClient.connected()) { return; }
    if (!myMQTTSvr.valid) { return; }
    
    // MQTT client connection request
    mqttClient.begin(myMQTTSvr.mqttHost, myMQTTSvr.mqttPort, net);
    Serial.println("\nconnecting to MQTT server...");
    Serial.println(myMQTTSvr.mqttHost);
    
    // Try to connect
    mqttClient.connect(mqttClientId, myMQTTSvr.mqttUser, myMQTTSvr.mqttPass);
    delay(1000);

    if(mqttClient.connected()) {
        Serial.println("\nconnected!");
        mqttClient.subscribe("homie/mkr1000/waterHeater/#");
        mqttClient.onMessage(messageReceived);
    } else {
        Serial.println("MQTT Host not reacheable!");
    }
    
}

void messageReceived(String &topic, String &payload) {

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
        //Serial.println("Light ON");
        digitalWrite(relayPin, LOW); 
       
      }else{
        //Serial.println("Light OFF");
        digitalWrite(relayPin, HIGH);
   
      }     

  delay(100);
  }

void setup()
{
    //read mqtt configuration
    myMQTTSvr = myFS.read();

    //init push button ON,OFF
    pinMode(pbuttonPin, INPUT_PULLUP); 
    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, HIGH);
   
    // initialize digital pin 1 as an output for resistence control.
    pinMode(1, OUTPUT);

    //int watch_dog = Watchdog.enable(300000);
    Serial.begin(9600);
    
    Serial.print("Enabled the watchdog with max countdown of ");
    //Serial.print(watch_dog, DEC);
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

    // Start Wifi in provisioning mode:
    //  1) This will try to connect to a previously associated access point.
    //  2) If this fails, an access point named "wifi101-XXXX" will be created, where XXXX
    //     is the last 4 digits of the boards MAC address. Once you are connected to the access point,
    //     you can configure an SSID and password by visiting http://wifi101/
    WiFi.beginProvision();

    while (WiFi.status() != WL_CONNECTED)
        Watchdog.reset();

    // Start HTTP server
    HTTPserver.begin();

    printWiFiStatus(); // you're connected now, so print out the status:

    // MQTT client connection
    connectMqttServer();
    Watchdog.reset();

}


void loop() {
    //call manual push button relay
    manualactivation();

    //connect to MQTT server if it is disconnected
    connectMqttServer();
    
    // MQTT client:
    mqttClient.loop();

    if (!man_control) {
        if (temperature > temp_sp + temp_hyst) {
            digitalWrite(1, false);
        } else if (temperature < temp_sp - temp_hyst) {
            digitalWrite(1, true);
        }       
    } else {
        digitalWrite(1, man_value);
    }

    WiFiClient HTTPclient = HTTPserver.available();

    if(HTTPclient) {
        while (HTTPclient.connected()) {
            
            if(HTTPclient.available()) {
                String str = HTTPclient.readString();

                Serial.println(str);

                // Reset device
                //if (str.indexOf("GET /reset")) { resetFunc(); }

                if (str.indexOf("/saveConfig")) { webSaveConfig(str); }
                
                webConfiguration(HTTPclient, mqttClient);

                break;

            }

        }
        
    }

    delay(1);

    HTTPclient.stop();
    
    // publish a message roughly every 10 seconds.
    if (millis() - lastMillis > 10000) {

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
