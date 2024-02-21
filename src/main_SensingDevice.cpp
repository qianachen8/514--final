#include <WiFi.h>

// WiFi credentials
const char *ssid = "your_wifi_ssid";
const char *password = "your_wifi_password";

// Sensor pins
const int turbidityPin = A0;
const int pHpin = A1;

// Server IP or hostname of the display device
const char* server = "display_device_ip";

WiFiClient client;

void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Read sensor values
  int turbidityValue = analogRead(turbidityPin);
  int turbidity = map(turbidityValue, 0, 4095, 100, 0);
  float pHValue = analogRead(pHpin);
  float voltage = pHValue * (3.3 / 4095.0);
  float pH = (3.3 * voltage);

  // Send sensor data to display device
  if (client.connect(server, 80)) {
    client.print("GET /update?tur=" + String(turbidity) + "&ph=" + String(pH) + " HTTP/1.1\r\n");
    client.print("Host: " + String(server) + "\r\n");
    client.print("Connection: close\r\n\r\n");
    client.stop();
  }

  delay(2000); // Adjust the delay as needed
}
