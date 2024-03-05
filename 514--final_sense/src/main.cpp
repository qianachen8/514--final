#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <stdlib.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
unsigned long previousMillis = 0;
const long interval = 1000;

// Constants for sensor pins
const int pH_Pin = 2; // Adjust if using a different pin
const int turbidity_Pin = 3; // Adjust if using a different pin

// Number of samples for moving average
const int numSamples = 10;
float pHReadings[numSamples];
float turbidityReadings[numSamples];
int readIndex = 0;

// TODO: Change the UUID to your own (any specific one works, but make sure they're different from others'). You can generate one here: https://www.uuidgenerator.net/
#define SERVICE_UUID        "e7fb9c3e-afe8-4172-9b49-0da0b2eb3167"
#define CHARACTERISTIC_UUID "f2fb033c-6ac8-4b5e-9dcf-dfa367e1ee89"

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};
void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE work!");

    // TODO: add codes for handling your sensor setup (pinMode, etc.)
    pinMode(pH_Pin, INPUT);
    pinMode(turbidity_Pin, INPUT);

    // Initialize the sensor readings array
    for (int i = 0; i < numSamples; i++) {
        pHReadings[i] = 0.0;
        turbidityReadings[i] = 0.0;
    }

    // TODO: name your device to avoid conflictions
    BLEDevice::init("XIAO_ESP32S3");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setValue("Hello World");
    pService->start();
    // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("Characteristic defined! Now you can read it in your phone!");
}

// TODO: add DSP algorithm functions here

// Function to read and process the pH sensor data
float readPHSensor() {
    int sensorValue = analogRead(pH_Pin);
    float voltage = sensorValue * (3.3 / 4095.0); // Convert to voltage
    float rawPH = (voltage - 2.5) * -1; // Convert voltage to pH value (example conversion)

    // Add the reading to the array and increment the index
    pHReadings[readIndex] = rawPH;
    
    // Calculate the average of the readings
    float sum = 0.0;
    for (int i = 0; i < numSamples; i++) {
        sum += pHReadings[i];
    }
    float averagePH = sum / numSamples;

    // You can add calibration code here if needed

    return averagePH;
}

// Function to read and process the turbidity sensor data
float readTurbiditySensor() {
    int sensorValue = analogRead(turbidity_Pin);
    float voltage = sensorValue * (3.3 / 4095.0); // Convert to voltage
    float rawTurbidity = voltage * 10; // Convert voltage to turbidity value (example conversion)

    // Add the reading to the array and increment the index
    turbidityReadings[readIndex] = rawTurbidity;

    // Calculate the average of the readings
    float sum = 0.0;
    for (int i = 0; i < numSamples; i++) {
        sum += turbidityReadings[i];
    }
    float averageTurbidity = sum / numSamples;

    // You can add calibration code here if needed

    return averageTurbidity;
}

void loop() {
    // Check if a BLE device is connected
    if (deviceConnected) {
        // Read sensor values
        float pH = readPHSensor(); // Implement this function to read from your pH sensor
        float turbidity = readTurbiditySensor(); // Implement this function to read from your turbidity sensor

        // Process sensor data and package it for BLE transmission
        // Here, we are simply packaging the raw sensor data, but you could
        // include any additional processing or DSP algorithms as needed.
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= interval) {
            previousMillis = currentMillis;

            // Package the sensor data into a string
            String sensorData = "pH:" + String(pH, 2) + ",Turbidity:" + String(turbidity, 2);

            // Set the characteristic's value to the sensor data
            pCharacteristic->setValue(sensorData.c_str());

            // Notify the connected client
            pCharacteristic->notify();

            // Debug output to the serial
            Serial.println("Notify value: " + sensorData);
        }
    }

    // Check if the BLE device has disconnected
    if (deviceConnected) {
        // Send new readings to database
        // TODO: change the following code to send your own readings and processed data
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= interval) {
        pCharacteristic->setValue("Hello World");
        pCharacteristic->notify();
        Serial.println("Notify value: Hello World");
        }
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500);  // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising();  // advertise again
        Serial.println("Start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
    delay(1000);
}
// No switch logic is needed since it's a physical power control