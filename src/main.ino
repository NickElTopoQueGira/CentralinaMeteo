// Import required libraries
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Adafruit_BMP085.h>
#include <Wire.h>

const char* ssid     = "ESP8266-Access-Point";
const char* password = "123456789";

//-----Definizione pins----------------------------------------------------------------------------

#define DHTPIN 14     // Digital pin connected to the DHT sensor
const int analogInPin = A0;  // ESP8266 Analog Pin ADC0 = A0

// Uncomment the type of sensor in use:
//#define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);
//Adafruit_BMP085 bmp;

// current temperature & humidity, updated in loop()
float t = 0.0;    //temperatura
float h = 0.0;    //umidità
float ws = 0.0;   //velocità del vento
int wd = 0;     //direzione del vento
float r = 0.0;    //pioggia
float pr = 0.0;   //pressione atmosferica
int change = 0;
float newPR = 1157.14;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;    // will store last time DHT was updated

// Updates DHT readings every 10 seconds
const long interval = 10000;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<title>Centralina Meteo</title>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>Centralina Meteo</h2>
<!--Temperatura-->
  <p>
    <span class="dht-labels">Temperatura</span>
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&deg;C</sup><!--&deg,&#176 fa si che si veda  °-->
  </p>
<!--Umidità-->
  <p>
    <span class="dht-labels">Umidita'</span>
    <span id="humidity">%HUMIDITY%</span>
    <sup class="units">%</sup>
  </p>
  <!--Pressione-->
  <p>
    <span class="dht-labels">Pressione Atm.</span>
    <span id="pressure">%PRESSURE%</span>
    <sup class="units">mBar</sup>
  </p>
  <!--Velocità Vento-->
  <p>
    <span class="dht-labels">Velocita' Vento</span>
    <span id="windspeed">%WINDSPEED%</span>
    <sup class="units">Km/h</sup>
  </p>
  <!--Direzione Vento -->
  <p>
    <span class="dht-labels">Direzione Vento</span>
    <span id="winddirection">%WINDDIRECTION%</span>
    <sup class="units">&#176;</sup>
  </p>
  <!--Pioggia-->
  <p>
    <span class="dht-labels">Pioggia</span>
    <span id="rain">%RAIN%</span>
    <sup class="units">mm</sup>
  </p>
</body>
<script>
    <!--temperatura-->
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 10000 ) ;
    <!--umidità -->
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 10000 ) ;
  <!-- Pressione -->
setInterval(function ( ){
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("pressure").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/pressure", true);
  xhttp.send();
}, 10000 ) ;
  <!-- velocità del vento -->
setInterval(function ( ){
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("windspeed").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/windspeed", true);
  xhttp.send();
}, 10000 ) ;
    <!--direzione del vento -->
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("winddirection").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/winddirection", true);
  xhttp.send();
}, 10000 ) ;
    <!--pioggia-->
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("rain").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/rain", true);
  xhttp.send();
}, 10000 ) ;
</script>
</html>)rawliteral";

// Replaces placeholder with DHT values
String processor(const String& var)
{
  //Serial.println(var);
  if (var == "TEMPERATURE") {
    return String(t);
  }
  else if (var == "HUMIDITY") {
    return String(h);
  }
  else if (var == "PRESSURE") {
    return String(pr);
  }
  else if (var == "WINDSPEED") {
    return String(ws);
  }
  else if (var == "WINDDIRECTION") {
    return String(wd);
  }
  else if (var == "RAIN") {
    return String(r);
  }
 return String();
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  //  dht.begin();
  //  if (!bmp.begin()) {
  //    Serial.println("Non trovato un valido sensore BMP085/BMP180, controllare le connessioni!");
  //    while (1) {}
  //  }
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Print ESP8266 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", String(t).c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", String(h).c_str());
  });
  server.on("/windspeed", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", String(ws).c_str());
  });
  server.on("/winddirection", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", String(wd).c_str());
  });
  server.on("/rain", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", String(r).c_str());
  });
  server.on("/pressure", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", String(pr).c_str());
  });

  // Start server
  server.begin();
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you updated the DHT values
    previousMillis = currentMillis;
    //----------- Lettura temperatura Celsius (the default)------
    float newT = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    //float newT = dht.readTemperature(true);
    // if temperature read failed, don't change t value
    if (isnan(newT)) {
      Serial.println("Errore lettura sensore DHT temperatura!");
    }
    else {
      t = newT;
      Serial.println(t);
    }
    // ------------ Lettura Umidità-------------------------------
    float newH = dht.readHumidity();
    // if humidity read failed, don't change h value
    if (isnan(newH)) {
      Serial.println("Errore lettura sensore DHT umidità!");
    }
    else {
      h = newH;
      Serial.println(h);
    }
    // -------------- Lettura Pressione atmosferica-------------
    //    float newPR = bmp.readPressure();
    //float newPR = 1157.14;
    //}
    if (isnan(newPR)) {
      Serial.println("Errore lettura sensore BMP180!");
    }
    else {
      pr = newPR;
      Serial.println(pr);
      newPR ++;
    }


    // Read Windspeed
    //---------------------------------------------------------------------------------------------
    //float newWS = dht.readHumidity();
    float newWS = 15.0;
    // if humidity read failed, don't change h value
    if (isnan(newWS)) {
      Serial.println("Errore lettura sensore HALL!");
    }
    else {
      ws = newWS;
      Serial.println(ws);
    }
    // Read winddirection
    //---------------------------------------------------------------------------------------------
    int sensorValue = analogRead(analogInPin);
    // map it to the range of 360°
    sensorValue = 256;
    int newWD = map(sensorValue, 0, 1024, 0, 360);
    //
    if (isnan(newWD)) {
      Serial.println("Errore lettura sensore direzione vento!");
    }
    else {
      wd = newWD;
      Serial.println(wd);
    }
    // Read Rain
    //---------------------------------------------------------------------------------------------
    //float newR = dht.readHumidity();
    float newR = 0.0;
    // if humidity read failed, don't change h value
    if (isnan(newR)) {
      Serial.println("Errore lettura pluviometro !");
    }
    else {
      r = newR;
      Serial.println(r);
    }
  }
}
