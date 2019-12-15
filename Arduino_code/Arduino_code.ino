//#include <FlashAsEEPROM.h>
//#include <FlashStorage.h>
#include <Adafruit_SleepyDog.h>
#include <Arduino_MKRENV.h>
#include <WiFi101.h>
#include <RTCZero.h>
#include <MQTT.h>
#include <MQTTClient.h>
#include "arduino_secrets.h"
#include <SD.h>
#include <ArduinoJson.h>

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int keyIndex = 0;
int status = WL_IDLE_STATUS;
char mqttHost[] = SECRET_MQTT_HOST;
char mqttClientId[] = SECRET_MQTT_CLIENT_ID;
//long randNumber;
char mqttUser[] = SECRET_MQTT_USER;
char mqttPass[] = SECRET_MQTT_PASS;
unsigned long lastMillis = 0;
unsigned long lastMillisFiles = 0;
int MAX_FILES_TO_READ = 5;

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
RTCZero rtc; // create an RTC object

const int GMT = 1; //change this to adapt it to your time zone
const int chipSelect = 4;
int ledState = LOW;

void initializeSDCard()
{
    Serial.print("Initializing SD card...");

    // see if the card is present and can be initialized:
    if (!SD.begin(chipSelect))
    {
        Serial.println("Card failed, or not present");
        // don't do anything more:
        while (1)
            ;
    }
    Serial.println("card initialized.");
}

String getFileNameForMinute()
{
    //Returns the name of the file considering the
    //epoch value
    String fileName = "";
    fileName += String(String(rtc.getDay()) + "_" + String(rtc.getHours()) + "_" + String(rtc.getMinutes()) + ".txt");
    return (fileName);
}

//The name of the file where we store the measures will change every minute
void saveMeasureWithTimeStamp(float temperature)
{
    String fileName = getFileNameForMinute();
    String dataString = "";
    dataString += String(String(rtc.getEpoch()) + "-" + String(temperature) + "\n");
    writeToSDCard(dataString, fileName);
}

void sendTemperature(String strEpoch, String strTemperature)
{
    Serial.println();
    Serial.println("Sending MQTT message with the following values");
    Serial.println(strEpoch);
    Serial.println(strTemperature);

    StaticJsonDocument<200> doc;
    doc["value"] = strTemperature;
    //InfluxDB uses nanoseconds epoch and epoch comes in seconds
    doc["time"] = String(strEpoch + "000000000");
    char JSONmessageBuffer[100];
    serializeJson(doc, JSONmessageBuffer);
    mqttClient.publish("homie/mkr1000/waterHeater/temperature", JSONmessageBuffer);

}

void writeToSDCard(String dataString, String fileName)
{
    File dataFile = SD.open(fileName.c_str(), FILE_WRITE);

    // if the file is available, write to it:
    if (dataFile)
    {
        Serial.println("Writing data to SD Card");
        dataFile.print(dataString);
        dataFile.close();
    }
    // if the file isn't open, pop up an error:
    else
    {
        Serial.println("ERROR opening file");
    }
}

// Reads the files and sends the measures contained in it
// Read a maximum of MAX_FILES_TO_READ on every loop iteration to avoid blocking the execution flow
void readCacheAndSend()
{
    if (!mqttClient.connected())
    {
        Serial.println("Storing information offline, MQTT server not available.");
        return;
    }
    if (ledState == LOW)
        ledState = HIGH;
    else
        ledState = LOW;
    digitalWrite(LED_BUILTIN, ledState);
    File dir = SD.open("/");

    int numFilesRead = 0;
    while (numFilesRead < MAX_FILES_TO_READ)
    {
        File entry = dir.openNextFile();
        if (!entry)
        {
            // no more files
            // return to the first file in the directory
            //Serial.println("No more files available in the SD card");
            dir.rewindDirectory();
            break;
        }
        //Serial.println (String ("Reading file " + String(entry.name())));
        if (entry.isDirectory())
        {
            //    Serial.println("Directory found");
            numFilesRead++;
        }
        else
        {
            // files have sizes, directories do not
            String buffer = "";
            String epoch = "";
            String temperature = "";
            while (entry.available())
            {
                char c = entry.read();
                if (c == '-')
                {
                    epoch = String(buffer);
                    buffer = "";
                }
                else
                {
                    if (c == '\n')
                    {
                        temperature = String(buffer);
                        buffer = "";
                        sendTemperature(epoch, temperature);
                    }
                    else
                    {
                        buffer += c;
                    }
                }

                Serial.write(c);
            }
            // close the file:
            entry.close();
            SD.remove(entry.name());
            Serial.println(String("File removed " + String(entry.name())));
            numFilesRead++;
        }
    }
    mqttClient.disconnect();
}

void setRTCwithNTP()
{
    unsigned long epoch;
    int numberOfTries = 0, maxTries = 6;
    do
    {
        Serial.print("getting time...");
        //        epoch = WiFi.getTime();
        //Workaround for issue with getTime returning 0
        epoch = 1576361581;
        delay(1000);
        Serial.println("done");
        numberOfTries++;
    } while ((epoch == 0) || (numberOfTries < maxTries));

    if (numberOfTries > maxTries)
    {
        Serial.print("NTP unreachable!!");
        WiFi.disconnect();
    }
    else
    {
        Serial.print("Epoch received: ");
        Serial.println(epoch);
        rtc.setEpoch(epoch);

        Serial.println();
    }
    rtc.setHours(rtc.getHours() + GMT);
}

void printTime()
{
    print2digits(rtc.getHours());
    Serial.print(":");
    print2digits(rtc.getMinutes());
    Serial.print(":");
    print2digits(rtc.getSeconds());
    Serial.println();
}

void printDate()
{
    print2digits(rtc.getDay());
    Serial.print("/");
    print2digits(rtc.getMonth());
    Serial.print("/");
    print2digits(rtc.getYear());

    Serial.println("");
}

void print2digits(int number)
{
    if (number < 10)
    {
        Serial.print("0");
    }
    Serial.print(number);
}

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

void connectMqttServer()
{
    int MAX_RETRIES = 3;
    Serial.print("checking wifi...");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(1000);
    }

    //randomSeed(analogRead(0));
    //randNumber = random(300);
    //char mqttClientId[] = String(randNumber);

    // MQTT client connection request
    mqttClient.begin(mqttHost, net);
    Serial.print("\nconnecting to MQTT server...");
    int num_tries = 0;
    unsigned int successful_connection = 0;
    while (num_tries < MAX_RETRIES && successful_connection == 0)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            successful_connection = mqttClient.connect(mqttClientId, mqttUser, mqttPass);
        }
        //Serial.print(".");
        //Serial.print(num_tries);
        num_tries++;
    }
    /*    while (!mqttClient.connect(mqttClientId, mqttUser, mqttPass) && num_tries < MAX_RETRIES)
    {
        Serial.print(".");
        Serial.print(num_tries);
        delay(1000);
        num_tries++;
    }
  */
    if (mqttClient.connected())
    {
        Serial.println("\nconnected!");
        mqttClient.subscribe("/hello");
    }
    else
    {
        Serial.println("\nMQTT server not available.");
    }
}

void messageReceived(String &topic, String &payload)
{
    Serial.println("incoming: " + topic + " - " + payload);
}

void setup()
{

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

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
    while (status != WL_CONNECTED)
    {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(ssid);
        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        status = WiFi.begin(ssid, pass);

        // wait 10 seconds for connection:
        delay(10000);
        Watchdog.reset();
    }
    printWiFiStatus(); // you're connected now, so print out the status:

    rtc.begin(); // initialize the RTC library
    Watchdog.reset();
    setRTCwithNTP(); // set the RTC time/date using epoch from NTP
    printTime();     // print the current time
    printDate();     // print the current date
    Watchdog.reset();
    initializeSDCard();

    // MQTT client connection
    mqttClient.begin(mqttHost, 1883, net);
    mqttClient.onMessage(messageReceived);
    connectMqttServer();
    Watchdog.reset();

}


void loop() {
    //call manual push button relay
    manualactivation();
    
    // MQTT client:
    mqttClient.loop();

    if (!mqttClient.connected())
    {
        connectMqttServer();
    }
    
    // publish a message roughly every second.
    if (millis() - lastMillis > 1000)
    {
        // read temperature sensor value
        temperature = ENV.readTemperature();

        String resistence_st = "false";
        String man_control_str = "false";

        if (digitalRead(1) == HIGH)
        {
            resistence_st = "true";
        }
        if (man_control)
        {
            man_control_str = "true";
        }
        if (mqttClient.connected())
        {
            //mqttClient.publish("homie/mkr1000/waterHeater/temperature", String(temperature));
            mqttClient.publish("homie/mkr1000/waterHeater/resistence", resistence_st);
            mqttClient.publish("homie/mkr1000/waterHeater/setpoint", String(temp_sp));
            mqttClient.publish("homie/mkr1000/waterHeater/hysteresis", String(temp_hyst));
            mqttClient.publish("homie/mkr1000/waterHeater/manCntrl", man_control_str);
        }
        saveMeasureWithTimeStamp(temperature);

        lastMillis = millis();
    }
    // Check if there are new measures to send every 0.5 ms
    if (millis() - lastMillisFiles > 2000)
    {
        readCacheAndSend();
        lastMillisFiles = millis();
    }
    Watchdog.reset();
}