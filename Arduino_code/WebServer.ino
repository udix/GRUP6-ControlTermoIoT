#include <WiFi101.h>

void webConfiguration(WiFiClient &client, MQTTClient &mqttClient) {

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");  // the connection will be closed after completion of the response
    //client.println("Refresh: 5");  // refresh the page automatically every 5 sec
    client.println();
    client.println("<!DOCTYPE HTML>");
    client.println("<html>");

    client.println("<head>");
    client.println("<title>Water Heater configuration and status</title>");
    client.println("</head>");

    // Style parameters
    //client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
    //client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
    //client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
    //client.println(".button2 {background-color: #555555;}</style></head>");

    client.println("<body>");
    client.println("<h3>STATUS</h3>");

    client.println("<table>");
    client.println("<tbody>");

    client.println("<tr>");
    client.println("<td>WIFI NETWORK</td>");
    client.println("</tr>");

    client.println("<tr>");
    client.println("</tr>");

    client.println("<tr>");
    client.println("<td>Status:</td>");
    client.print("<td>");
    client.print(WiFi.status());
    client.print("</td>");
    client.println("</tr>");

    client.println("<tr>");
    client.println("<td>SSID:</td>");
    client.print("<td>");
    client.print(WiFi.SSID());
    client.print("</td>");
    client.println("</tr>");

    client.println("<tr>");
    client.println("<td>RSSI:</td>");
    client.print("<td>");
    client.print(WiFi.RSSI());
    client.print("</td>");
    client.println("</tr>");

    client.println("<tr>");
    client.println("</tr>");

    client.println("<tr>");
    client.println("<td>MQTT</td>");
    client.println("</tr>");

    client.println("<tr>");
    client.println("</tr>");

    client.println("<tr>");
    client.println("<td>Host:</td>");
    client.print("<td>");
    client.print(String(FS_mqttHost.read()));
    client.print(":1883");
    client.print("</td>");
    client.println("</tr>");

    client.println("<tr>");
    client.println("<td>Status:</td>");
    client.print("<td>");
    client.print((mqttClient.connected() ? "CONECTED" : "DISCONNECTED"));
    client.print("</td>");
    client.println("</tr>");

    client.println("</tbody>");
    client.println("</table>");

    client.println("<p>&nbsp;</p>");

    client.println("<h3>CONFIGURATION</h3>");

    client.println("<form method='get' action='saveConfig'>");
    client.println("MQTT Host: <input type='text' name='mqttHost'><br>");
    client.println("<p>&nbsp;</p>");
    client.println("MQTT Port: <input type='number' name='mqttPort'><br>");
    client.println("<p>&nbsp;</p>");
    client.println("MQTT User: <input type='text' name='mqttUser'><br>");
    client.println("<p>&nbsp;</p>");
    client.println("MQTT Password: <input type='password' name='mqttPass'><br>");
    client.println("<p>&nbsp;</p>");
    client.println("<input type='submit' value='Submit'>");
    client.println("</form>");

    // Reset button
    //client.println("<p><a href=\"/reset\"><button class=\"button\">RESET</button></a></p>");

    client.println("</body>");

    client.println("</html>");
}

void webSaveConfig(String data) {
  
    int startIdx = data.indexOf('?') + 1;
    int endIdx = data.indexOf("HTTP/1.1") - 1;

    String arr_params[10];

    String parameters = data.substring(startIdx, endIdx);

    int pStartIdx = 0;

    while (parameters.length() > 0) {

        int idx = parameters.indexOf('=');

        if (parameters.substring(pStartIdx, idx) == "mqttHost") {
            // remove parameter name
            parameters.remove(0, idx + 1);
            // copy parameter value in an array of chars
            char buf[parameters.indexOf('&') + 1];
            parameters.toCharArray(buf, parameters.indexOf('&') + 1);
            // save parameter value in flash memory
            FS_mqttHost.write(buf);
            Serial.println(String(buf));
        } else if (parameters.substring(pStartIdx, idx) == "mqttPort") {
            // remove parameter name
            parameters.remove(0, idx + 1);
            // copy parameter value in an array of chars
            char buf[parameters.indexOf('&')];
            parameters.toCharArray(buf, parameters.indexOf('&'));
            // save parameter value in flash memory
            //FS_mqttHost.write(buf);
        } else if (parameters.substring(pStartIdx, idx) == "mqttUser") {
            // remove parameter name
            parameters.remove(0, idx + 1);
            // copy parameter value in an array of chars
            char buf[parameters.indexOf('&')];
            parameters.toCharArray(buf, parameters.indexOf('&'));
            // save parameter value in flash memory
            FS_mqttUser.write(buf);
        } else if (parameters.substring(pStartIdx, idx) == "mqttPass") {
            // remove parameter name
            parameters.remove(0, idx + 1);
            // copy parameter value in an array of chars
            char buf[parameters.indexOf('&')];
            parameters.toCharArray(buf, parameters.indexOf('&'));
            // save parameter value in flash memory
            FS_mqttPass.write(buf);
        }

        // remove value from parameters string
        if (parameters.indexOf('&') > 0) {
            parameters.remove(0,parameters.indexOf('&') + 1);
        } else {
            parameters.remove(0, parameters.length());
        }
        
        Serial.println(String(FS_mqttHost.read()));
        
    }

/*
  //Reads parameters introduced by user
  String qsid;
  qsid = server.arg("ssid");
  qsid.replace("%2F", "/");
  Serial.println("Got SSID: " + qsid);
  esid = (char*) qsid.c_str();

  String qpass;
  qpass = server.arg("pass");
  qpass.replace("%2F", "/");
  Serial.println("Got pass: " + qpass);
  epass = (char*) qpass.c_str();

  String qalert;
  qalert = server.arg("alert");
  qalert.replace("%2F", "/");
  Serial.println("Got type of alert: " + qalert);
  alert = (char*) qalert.c_str();

  Serial.print("Settings written ");
  //Save configuration to flash memory
  saveConfig() ? Serial.println("sucessfully.") : Serial.println("not succesfully!");
  Serial.println("Restarting!");
  delay(1000);
  ESP.reset();
*/
}
