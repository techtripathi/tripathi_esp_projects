
#include <WiFi.h>
#include <WebServer.h>
#include <DHT11.h>

// Wi-Fi credentials
const char* WIFI_SSID = "Vaibhav";
const char* WIFI_PASSWORD = "11223344";

// DHT11 sensor on GPIO 5
DHT11 dht11(5);

WebServer server(80);

// Store the latest valid readings
int temperature = 0;
int humidity = 0;
bool sensorOK = false;
unsigned long lastReadTime = 0;

// Web dashboard
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32-C3 Sensor Dashboard</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background: #f1f5f9;
      color: #1e293b;
      text-align: center;
      padding: 20px;
    }
    .container {
      max-width: 500px;
      margin: auto;
    }
    .card {
      background: white;
      padding: 25px;
      margin: 20px 0;
      border-radius: 14px;
      box-shadow: 0 4px 12px #0001;
    }
    .value {
      font-size: 42px;
      font-weight: bold;
      color: #087f8c;
    }
    .status {
      color: #64748b;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>DHT11 Monitor</h1>
    <p id="status" class="status">Connecting...</p>

    <div class="card">
      <h3>Temperature</h3>
      <div class="value"><span id="temp">--</span> &deg;C</div>
    </div>

    <div class="card">
      <h3>Humidity</h3>
      <div class="value"><span id="hum">--</span> %</div>
    </div>

    <p class="status">Updated: <span id="time">--</span></p>
  </div>

  <script>
    async function updateData() {
      try {
        const response = await fetch('/api/data', {
          cache: 'no-store'
        });

        if (!response.ok) throw new Error('Request failed');

        const data = await response.json();

        if (!data.ok) {
          document.getElementById('status').textContent =
            'DHT11 reading error';
          return;
        }

        document.getElementById('temp').textContent =
          data.temperature;
        document.getElementById('hum').textContent =
          data.humidity;

        document.getElementById('status').textContent =
          'Sensor connected';

        document.getElementById('time').textContent =
          new Date().toLocaleTimeString();

      } catch (error) {
        document.getElementById('status').textContent =
          'Connection lost - retrying';
      }
    }

    updateData();
    setInterval(updateData, 2000);
  </script>
</body>
</html>
)rawliteral";

// Serve the dashboard
void handleRoot() {
  server.send_P(200, "text/html", webpage);
}

// Serve the latest sensor readings as JSON
void handleSensorData() {
  String json = "{";
  json += "\"ok\":";
  json += sensorOK ? "true" : "false";
  json += ",\"temperature\":";
  json += String(temperature);
  json += ",\"humidity\":";
  json += String(humidity);
  json += "}";

  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi connected");
  Serial.print("Open dashboard: http://");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/data", HTTP_GET, handleSensorData);

  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();

  // Read DHT11 every 2 seconds
  if (millis() - lastReadTime >= 2000 ||
      lastReadTime == 0) {
    lastReadTime = millis();

    int newTemperature = 0;
    int newHumidity = 0;

    int result = dht11.readTemperatureHumidity(
      newTemperature, newHumidity
    );

    if (result == 0) {
      temperature = newTemperature;
      humidity = newHumidity;
      sensorOK = true;

      Serial.printf("Temperature: %d C | Humidity: %d %%\n",
                    temperature, humidity);
    } else {
      sensorOK = false;
      Serial.println(DHT11::getErrorString(result));
    }
  }
}
