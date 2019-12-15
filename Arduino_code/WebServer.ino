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
    client.print(mqttHost);
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

    // Reset button
    client.println("<p><a href=\"/reset\"><button class=\"button\">RESET</button></a></p>");

    client.println("</body>");

    client.println("</html>");
}
