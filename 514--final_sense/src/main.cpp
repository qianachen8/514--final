#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID        "9421d390-7612-4ea9-ab96-c28e14e38fdf"
#define CHARACTERISTIC_UUID "1b3a6e07-fcdc-416a-961b-7c2ed7482778"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
const int turbidityPin = 1; // pin
const int pHPin = 2;    // pin


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

    sensors.begin(); // sensor begin work

    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    BLEDevice::startAdvertising();
    Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void loop() {
    if (deviceConnected) {
    int phSensorValue = analogRead(phSensorPin);
    float phValue = (float)phSensorValue * (5.0 / 1023.0); // Example conversion formula for pH value

    int turbiditySensorValue = analogRead(turbiditySensorPin);
    float turbidity = (float)turbiditySensorValue * (5.0 / 1023.0); // Example conversion formula for turbidity

    char buffer[64]; // Buffer

    // Modify the message based on the pH value
    if (phValue < 7) {
        snprintf(buffer, sizeof(buffer), "Acid\nTurbidity: %.2f NTU", turbidity);
    } else if (phValue > 7) {
        snprintf(buffer, sizeof(buffer), "Basic\nTurbidity: %.2f NTU", turbidity);
    } else {
        snprintf(buffer, sizeof(buffer), "Natural\nTurbidity: %.2f NTU", turbidity);
    }

    pCharacteristic->setValue(buffer);
    pCharacteristic->notify();
    Serial.println(buffer);
}
    
    if (!deviceConnected && oldDeviceConnected) {
        delay(500);
        pServer->startAdvertising();
        Serial.println("Start advertising");
        oldDeviceConnected = deviceConnected;
    }
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }
    delay(1000);
}
