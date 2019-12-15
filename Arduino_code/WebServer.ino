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
    client.print(String(myMQTTSvr.mqttHost));
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

    //------CONFIGURATION
    client.println("<form method='get' action='saveConfig'>");

    client.println("<table>");
    client.println("<tbody>");

    client.println("<tr>");
    client.println("<td>MQTT Host:</td>");
    client.println("<td><input type='text' name='mqttHost'><br></td>");
    client.println("</tr>");

    client.println("<tr>");
    client.println("<td>MQTT Port:</td>");
    client.println("<td><input type='number' name='mqttPort'><br></td>");
    client.println("</tr>");

    client.println("<tr>");
    client.println("<td>MQTT User:</td>");
    client.println("<td><input type='text' name='mqttUser'><br></td>");
    client.println("</tr>");

    client.println("<tr>");
    client.println("<td>MQTT Password:</td>");
    client.println("<td><input type='password' name='mqttPass'><br></td>");
    client.println("</tr>");

    client.println("</tbody>");
    client.println("</table>");

    client.println("<input type='submit' value='Save Configuration'>");

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
            // save parameter into myServerCredentials struct
            String host = parameters.substring(0, parameters.indexOf('&'));
            host.toCharArray(myMQTTSvr.mqttHost, 50);
        } else if (parameters.substring(pStartIdx, idx) == "mqttPort") {
            // remove parameter name
            parameters.remove(0, idx + 1);
            // save parameter into myServerCredentials struct
            int port = parameters.substring(0, parameters.indexOf('&')).toInt();
            myMQTTSvr.mqttPort = port;
        } else if (parameters.substring(pStartIdx, idx) == "mqttUser") {
            // remove parameter name
            parameters.remove(0, idx + 1);
            // save parameter into myServerCredentials struct
            String user = parameters.substring(0, parameters.indexOf('&'));
            user.toCharArray(myMQTTSvr.mqttUser, 50);
        } else if (parameters.substring(pStartIdx, idx) == "mqttPass") {
            // remove parameter name
            parameters.remove(0, idx + 1);
            // save parameter into myServerCredentials struct
            String pass = parameters.substring(0, parameters.length());
            pass.toCharArray(myMQTTSvr.mqttPass, 50);
        }

        // remove value from parameters string
        if (parameters.indexOf('&') > 0) {
            parameters.remove(0,parameters.indexOf('&') + 1);
        } else {
            parameters.remove(0, parameters.length());
        }

    }
    // set valid to true, because we have been saved values
    myMQTTSvr.valid = true;
    // Save in flash memory
    myFS.write(myMQTTSvr);

}
