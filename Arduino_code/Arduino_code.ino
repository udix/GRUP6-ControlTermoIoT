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

// vars for water heater control
float temp_sp = 0;
float temp_hyst = 0;
bool man_control = false;
bool man_value = false;
float temperature = 0;

WiFiClient net;
MQTTClient mqttClient;
RTCZero rtc; // create an RTC object

const int GMT = 1; //change this to adapt it to your time zone
const int chipSelect = 4;

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
//    fileName += String(String(rtc.getDay()) + "_" + String(rtc.getMonth()) + "_" + String(rtc.getYear()) + "_");
    fileName += String(String(rtc.getDay()) + "_" + String(rtc.getHours()) + "_" + String(rtc.getMinutes()) + ".txt");
    Serial.println("Filename has the following value");
    Serial.println(fileName);
    return (fileName);
}

//
//Store measure every second
void saveMeasureWithTimeStamp(float temperature)
{
    // Check if the file exists for writing
    // YES
    // Open the file,
    //save the measure Epochs-Measure,
    //Close the file
    // NO
    // Create the file
    String fileName = getFileNameForMinute();
    String dataString = "";
    dataString += String(String(rtc.getEpoch()) + "-" + String(temperature) + "\n");
    writeToSDCard(dataString, fileName);
}

void sendTemperature(String strEpoch, String strTemperature)
{
    Serial.println("Sending MQTT message");
    Serial.println (strEpoch);
    Serial.println (strTemperature);
    //char json[] = "{\"sensor\":\"gps\",\"time\":1351824120,\"data\":[48.756080,2.302038]}";

    mqttClient.publish("homie/mkr1000/waterHeater/temperature", strTemperature);
}

void writeToSDCard(String dataString, String fileName)
{

//    File dataFile = SD.open("datatest.txt", FILE_WRITE);
//    File dataFile = SD.open("22_49.txt", FILE_WRITE);
    File dataFile = SD.open(fileName.c_str(), FILE_WRITE);

    // if the file is available, write to it:
    if (dataFile)
    {
        Serial.println();
        Serial.println("Writing data to SD Card");
        Serial.println();
        dataFile.print(dataString);
        dataFile.close();
        // print to the serial port too:
        Serial.println(dataString);
    }
    // if the file isn't open, pop up an error:
    else
    {
        Serial.println("ERROR opening file");
    }
}

void readCacheAndSend()
{
    File dir = SD.open("/");

    //Read all the files
    //For every file
    // Check if it's more than 24hours
    //YES : Discard the file and move to the next one
    //NO : Read the file
    // For every line, send the measure and time saveMeasureWithTimeStamp
    // If the message was sent
    while (true)
    {
        File entry = dir.openNextFile();
        if (!entry)
        {
            // no more files
            // return to the first file in the directory
            dir.rewindDirectory();
            break;
        }
        //        for (uint8_t i = 0; i < numTabs; i++)
        //        {
        //            Serial.print('\t');
        //        }
        Serial.println (String ("Reading file " + String(entry.name())));
        if (entry.isDirectory())
        {
            Serial.println("/");
            //printDirectory(entry, numTabs + 1);
        }
        else
        {
            // files have sizes, directories do not
            String buffer = "";
            String epoch = "";
            String temperature = "";
            Serial.print("\t\t");
            Serial.println(entry.size(), DEC);
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
//            SD.remove(entry.name());   
            Serial.println (String ("File removed " + String(entry.name())));
        }
    }
}

//Store one file per getMinutes
//Send measures if files available
// Need to know the current minute

//FlashStorage(storage, "env_readings.csv");

void setRTCwithNTP()
{
    unsigned long epoch;
    int numberOfTries = 0, maxTries = 6;
    do
    {
        Serial.print("getting time...");
 //       epoch = WiFi.getTime();
        epoch = 1576100985;
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

void printReading(float temperature,
                  float humidity,
                  float pressure,
                  float illuminance,
                  float uva,
                  float uvb,
                  float uvIndex)
{
    printTime();

    // print each of the sensor values
    Serial.print("Temperature = ");
    Serial.print(temperature);
    Serial.println(" °C");

    Serial.print("Humidity    = ");
    Serial.print(humidity);
    Serial.println(" %");

    Serial.print("Pressure    = ");
    Serial.print(pressure);
    Serial.println(" kPa");

    Serial.print("Illuminance = ");
    Serial.print(illuminance);
    Serial.println(" lx");

    Serial.print("UVA         = ");
    Serial.println(uva);

    Serial.print("UVB         = ");
    Serial.println(uvb);

    Serial.print("UV Index    = ");
    Serial.println(uvIndex);
    // print an empty line
    Serial.println();
}

void connectMqttServer()
{
    Serial.print("checking wifi...");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(1000);
    }
    /*
    randomSeed(analogRead(0));
    randNumber = random(300);
    char mqttClientId[] = String(randNumber);
    */
    // MQTT client connection request
    mqttClient.begin(mqttHost, net);
    Serial.print("\nconnecting to MQTT server...");
    while (!mqttClient.connect(mqttClientId, mqttUser, mqttPass))
    {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("\nconnected!");

    mqttClient.subscribe("/hello");
}

void messageReceived(String &topic, String &payload)
{
    Serial.println("incoming: " + topic + " - " + payload);
}

void setup()
{

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
//    mqttClient.begin(mqttHost, net);
//    mqttClient.onMessage(messageReceived);
//    connectMqttServer();
    Watchdog.reset();
}

void loop()
{
    // MQTT client:
    //mqttClient.loop();

    //if (!mqttClient.connected())
    //{
    //    connectMqttServer();
    //}

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

        saveMeasureWithTimeStamp(temperature);

        lastMillis = millis();
        //        mqttClient.publish("/mkr1000_2/temperature", String(temperature));
    }
    readCacheAndSend();
    Watchdog.reset();
}