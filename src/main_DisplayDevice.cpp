#include <WiFi.h>
#include <WebServer.h>
#include <LiquidCrystal_I2C.h>

// WiFi credentials
const char *ssid = "your_wifi_ssid";
const char *password = "your_wifi_password";

// Initialize LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Initialize web server on port 80
WebServer server(80);

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  
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

  // Set up web server routes
  server.on("/update", handleUpdate);
  server.begin();
}

void loop() {
  server.handleClient();
}

void handleUpdate() {
  String turbidity = server.arg("tur");
  String pH = server.arg("ph");

  // Display sensor values on LCD
  lcd.setCursor(0, 0);
  lcd.print("Turbidity:");
  lcd.setCursor(10, 0);
  lcd.print(turbidity);

  lcd.setCursor(0, 1);
  lcd.print("pH:");
  lcd.setCursor(3, 1);
  lcd.print(pH);

  server.send(200, "text/plain", "OK");
}
