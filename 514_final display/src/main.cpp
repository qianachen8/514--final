#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SwitecX25.h>

// standard X25.168 ranges from 0 to 315 degrees, at 1/3 degree increments
#define STEPS 945
static int nextPos = 0;
bool setFlag = false;

// refreshClk variable to set display refresh rate
int refreshClk = 0;

// There is a active-low button switch connected to the pin D8
#define BUTTON_PIN D9

// There is a 128x32 OLED display connected via I2C (SDA, SCL pins)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// For motors connected to D0, D1, D2, D3
SwitecX25 motor1(STEPS, D0, D1, D2, D3);

// Constants for sensor pins
const int pH_Pin = 2; // Adjust if using a different pin
const int turbidity_Pin = 3; // Adjust if using a different pin

// BLE Server
BLEServer* pServer = NULp[L;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;

// UUIDs
static BLEUUID serviceUUID("e7fb9c3e-afe8-4172-9b49-0da0b2eb3167");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("f2fb033c-6ac8-4b5e-9dcf-dfa367e1ee89");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;


static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    // TODO: add codes to handle the data received from the server, and call the data aggregation function to process the data

    // TODO: change the following code to customize your own data format for printing
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("data: ");
    Serial.write(pData, length);
    Serial.println();
}

// Function definitions
float readPHSensor() {
    // Assuming the pH sensor is connected to analog pin A0
    int sensorValue = analogRead(2);
    float voltage = sensorValue * (5.0 / 1023.0); // Convert the analog reading to voltage
    float pH = (voltage - 2.5) * -1; // Convert voltage to pH value (example conversion, adjust as needed)
    return pH;
}

float readTurbiditySensor() {
    // Assuming the turbidity sensor is connected to analog pin A1
    int sensorValue = analogRead(3);
    float voltage = sensorValue * (5.0 / 1023.0); // Convert the analog reading to voltage
    float turbidity = voltage * 10; // Convert voltage to turbidity value (example conversion, adjust as needed)
    return turbidity;
}

// Move stepper motor based on pH value
void moveStepperMotor(float pH) {
    int targetPosition = map(pH, 1, 14, 0, STEPS); // Map pH value to stepper position
    motor1.setPosition(targetPosition); // Use setPosition instead of moveTo
    while (motor1.currentStep != targetPosition) { // Check against currentStep instead of distanceToGo
        motor1.update();
        delay(1); // Short delay to allow motor to step
    }
}

// Reset stepper motor when pushbutton is pressed
void resetStepperMotor() {
    if (digitalRead(BUTTON_PIN) == LOW) {
        motor1.setPosition(0); // Use setPosition to set the motor position to 0
        while (motor1.currentStep != 0) { // Check against currentStep
            motor1.update();
            delay(1); // Short delay to allow motor to step
        }
    }
}

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};

/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");
  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  // Display sensor values on OLED display
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("Turbidity: ");
    display.println(readTurbiditySensor());
    display.print("pH: ");
    display.println(readPHSensor());
    display.display();
    Serial.println("OLED display initialized");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
} // End of setup.


class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());

    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");
    pClient->setMTU(517); //set client to request maximum MTU from server (default is 23 otherwise)

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    // Read the value of the characteristic.
    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }

    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
    return true;
}



// This is the Arduino main loop function.
void loop() {
  // Check if we need to connect to the BLE Server
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
      connected = true; // Set the connected flag to true
    } else {
      Serial.println("We have failed to connect to the server; there is nothing more we will do.");
      connected = false; // Set the connected flag to false
    }
    doConnect = false;
  }
    if (deviceConnected) {
        
        // Read sensor values
        float pH = readPHSensor();
        float turbidity = readTurbiditySensor();
        int pHsensorValue = 0;
        int turbiditySensorValue = 0;

        // Display sensor values on Serial Monitor
        Serial.print("Turbidity: ");
        Serial.println(turbidity);
        Serial.print("pH: ");
        Serial.println(pH);
        Serial.print("Turbidity Sensor Value: ");
        Serial.println(turbiditySensorValue);
        Serial.print("pH Sensor Value: ");
        Serial.println(pHsensorValue);


        // Construct the string to send over BLE
        String sensorData = "pH:" + String(pH) + ", Turbidity:" + String(turbidity);

        // Send sensor values over BLE by writing to the characteristic
        pRemoteCharacteristic->writeValue(sensorData.c_str(), sensorData.length());
        Serial.println("Sent data to BLE Server: " + sensorData);
    } else if (doScan) {
        // Restart scanning if we are not connected
        BLEDevice::getScan()->start(0); // 0 means continuous scanning
    }

    delay(1000); // Delay a second between loops.
}

