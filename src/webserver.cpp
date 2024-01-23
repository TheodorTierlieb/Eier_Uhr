#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "webserver.h"

AsyncWebServer server(80);

String webMenu[10];
int webMenLength;
unsigned long webSubMenuTimes[10];

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
  <head>
    <title>Tick Tick Ei</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
  </head>
  <style>
    table, th, td {
      border:1px solid black;
    }
  </style>
  <body>
    <center>
      <h2>Tick Tick Ei</h2>
      <table>
        <tr>
          <th>Timer</th>
          <th>Zeit</th>
        </tr>
        %TIMERPLACEHOLDER%
      </table>
    </center>
    <iframe style="display:none" name="self_page"></iframe>
  </body>
</html> )rawliteral";

void notFound(AsyncWebServerRequest *request) 
{
  request->send(404, "text/plain", "The page not found");
}

String processor(const String& var)
{
  if(var == "TIMERPLACEHOLDER")
  {
    String zeilen = "";
    for (int i = 0; i <webMenLength; i++)
    {
      zeilen += "<tr><td>" + webMenu[i] + "</td><td>" + String(webSubMenuTimes[i]) + "</td></tr>";
    }
    return zeilen;
  }
  return String();
}

void startWebserver(char *menu[], unsigned long *subMenuTimes, int menLength)
{

Serial.println("starting Webserver");

  webMenLength = menLength;
  
  for (int i = 0; i < menLength; i++) 
  {
    webMenu[i] = String(menu[i]);
    webSubMenuTimes[i] = subMenuTimes[i];
  }


  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send_P(200, "text/html", index_html, processor);
  });

  server.onNotFound(notFound);

  server.begin();
}