/* Be sure to set numHandles = 60 in this file:
C:\Users\Jeff\.platformio\packages\framework-arduinoespressif32\libraries\BLE\src\BLEServer.h
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <string>

//BLE configuration *************************************************************

BLEServer* pServer = NULL;
BLECharacteristic* pControl1Characteristic = NULL;
BLECharacteristic* pControl2Characteristic = NULL;
BLECharacteristic* pControl3Characteristic = NULL;
BLECharacteristic* pBrightnessCharacteristic = NULL;
BLECharacteristic* pSpeedCharacteristic = NULL;
BLECharacteristic* pPaletteCharacteristic = NULL;
bool deviceConnected = false;
bool wasConnected = false;

#define SERVICE_UUID                   "19b10000-e8f2-537e-4f6c-d104768a1214"
#define CONTROL1_CHARACTERISTIC_UUID   "19b10001-e8f2-537e-4f6c-d104768a1214"
#define CONTROL2_CHARACTERISTIC_UUID   "19b10002-e8f2-537e-4f6c-d104768a1214"
#define CONTROL3_CHARACTERISTIC_UUID   "19b10003-e8f2-537e-4f6c-d104768a1214"
#define BRIGHTNESS_CHARACTERISTIC_UUID "19b10004-e8f2-537e-4f6c-d104768a1214"
#define SPEED_CHARACTERISTIC_UUID      "19b10005-e8f2-537e-4f6c-d104768a1214"
#define PALETTE_CHARACTERISTIC_UUID    "19b10006-e8f2-537e-4f6c-d104768a1214"

BLEDescriptor pBrightnessDescriptor(BLEUUID((uint16_t)0x2901));
BLEDescriptor pSpeedDescriptor(BLEUUID((uint16_t)0x2902));
BLEDescriptor pPaletteDescriptor(BLEUUID((uint16_t)0x2903));

//Control functions***************************************************************

void brightnessAdjust(uint8_t newBrightness) {
   BRIGHTNESS = newBrightness;
   brightnessChanged = true;
   FastLED.setBrightness(BRIGHTNESS);
   pBrightnessCharacteristic->setValue(String(BRIGHTNESS).c_str());
   Serial.print("Brightness: ");
   Serial.println(BRIGHTNESS);
}

void speedAdjust(uint8_t newSpeed) {
   SPEED = newSpeed;
   speedfactor = SPEED/6; 
   pSpeedCharacteristic->setValue(String(SPEED).c_str());
   pSpeedCharacteristic->notify();
   Serial.print("Speed: ");
   Serial.println(SPEED);
   }

void startWaves() {
   if (rotateWaves) { gCurrentPaletteNumber = random(0,gGradientPaletteCount); }
   CRGBPalette16 gCurrentPalette( gGradientPalettes[gCurrentPaletteNumber] );
   pPaletteCharacteristic->setValue(String(gCurrentPaletteNumber).c_str());
   pPaletteCharacteristic->notify();
   Serial.print("Color palette: ");
   Serial.println(gCurrentPaletteNumber);
   gCurrentPaletteNumber = addmod8( gCurrentPaletteNumber, 1, gGradientPaletteCount);
   gTargetPalette = gGradientPalettes[ gCurrentPaletteNumber ];
   }

//Callbacks***********************************************************************

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    wasConnected = true;
    Serial.println("Device Connected");
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    wasConnected = true;
  }
};

class Control1CharacteristicCallbacks : public BLECharacteristicCallbacks {
 void onWrite(BLECharacteristic *characteristic) {
    String value = characteristic->getValue();
    if (value.length() > 0) {
       uint8_t receivedValue = value[0];
       Serial.print("C1: ");
       Serial.println(receivedValue);
       if (receivedValue == 1) {
          displayOn = false;
       }
       if (receivedValue == 2) {
          displayOn = true;
          runPride = true;
          runWaves = false;
          program = 2;
       }
       if (receivedValue == 3) {
          displayOn = true;
          runPride = false;
          runWaves = true;
          program = 2;
          startWaves();
       }
       if (receivedValue == 4) {
          displayOn = true;
          runPride = false;
          runWaves = false;
          program = 1;
       }
       if (receivedValue == 5) {
          displayOn = true;
          runPride = false;
          runWaves = false;
          program = 3;
       }
    }
   }
};

class Control2CharacteristicCallbacks : public BLECharacteristicCallbacks {
 void onWrite(BLECharacteristic *characteristic) {
    String value = characteristic->getValue();
    if (value.length() > 0) {
       uint8_t receivedValue = value[0]; 
       Serial.print("C2: ");
       Serial.println(receivedValue);
       if (receivedValue == 1) {
          uint8_t newBrightness = min(BRIGHTNESS + brightnessInc,255);
          brightnessAdjust(newBrightness);
       }
       if (receivedValue == 2) {
          uint8_t newBrightness = max(BRIGHTNESS - brightnessInc,0);
          brightnessAdjust(newBrightness);
       }
       if (receivedValue == 3) {
          uint8_t newSpeed = min(SPEED+1,10);
          speedAdjust(newSpeed);
       }
       if (receivedValue == 4) {
          uint8_t newSpeed = max(SPEED-1,0);
          speedAdjust(newSpeed);
       }
    }
 }
};

class Control3CharacteristicCallbacks : public BLECharacteristicCallbacks {
 void onWrite(BLECharacteristic *characteristic) {
    String value = characteristic->getValue();
    if (value.length() > 0) {
       uint8_t receivedValue = value[0]; 
       Serial.print("C3: ");
       Serial.println(receivedValue);
       if (receivedValue == 100) {
          rotateWaves = true;
       }
       if (receivedValue == 101) {
          rotateWaves = false;
       }
    }
 }
};

class PaletteCharacteristicCallbacks : public BLECharacteristicCallbacks {
 void onWrite(BLECharacteristic *characteristic) {
    String value = characteristic->getValue();
    if (value.length() > 0) {
       uint8_t receivedValue = value[0]; 
       Serial.print("Palette: ");
       Serial.println(receivedValue);
          //int newPaletteNumber = receivedValue;
          gTargetPalette = gGradientPalettes[ receivedValue ];
          pPaletteCharacteristic->setValue(String(receivedValue).c_str());
          pPaletteCharacteristic->notify();
          //CRGBPalette16 gCurrentPalette( gGradientPalettes[newPaletteNumber] );
    }
 }
};

//*******************************************************************************************

void bleSetup() {

 BLEDevice::init("Aurora Portal");

 pServer = BLEDevice::createServer();
 pServer->setCallbacks(new MyServerCallbacks());

 BLEService *pService = pServer->createService(SERVICE_UUID);

 pControl1Characteristic = pService->createCharacteristic(
                      CONTROL1_CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_WRITE
                    );
 pControl1Characteristic->setCallbacks(new Control1CharacteristicCallbacks());
 pControl1Characteristic->addDescriptor(new BLE2902());

 pControl2Characteristic = pService->createCharacteristic(
                      CONTROL2_CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_WRITE
                    );
 pControl2Characteristic->setCallbacks(new Control2CharacteristicCallbacks());
 pControl2Characteristic->addDescriptor(new BLE2902());

 pControl3Characteristic = pService->createCharacteristic(
                      CONTROL3_CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_WRITE
                    );
 pControl3Characteristic->setCallbacks(new Control3CharacteristicCallbacks());
 pControl3Characteristic->addDescriptor(new BLE2902());

 pBrightnessCharacteristic = pService->createCharacteristic(
                      BRIGHTNESS_CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
 pBrightnessCharacteristic->addDescriptor(new BLE2902());
 pBrightnessDescriptor.setValue("Brightness");
 pBrightnessCharacteristic->setValue(String(BRIGHTNESS).c_str()); 

 pSpeedCharacteristic = pService->createCharacteristic(
                      SPEED_CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
 pSpeedCharacteristic->addDescriptor(new BLE2902());
 pSpeedDescriptor.setValue("Speed"); 
 pSpeedCharacteristic->setValue(String(SPEED).c_str());

 pPaletteCharacteristic = pService->createCharacteristic(
                      PALETTE_CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
 pPaletteCharacteristic->setCallbacks(new PaletteCharacteristicCallbacks());
 pPaletteCharacteristic->addDescriptor(new BLE2902());
 pPaletteDescriptor.setValue("Palette"); 
 pPaletteCharacteristic->setValue(String(gCurrentPaletteNumber).c_str());

 pService->start();

 BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
 pAdvertising->addServiceUUID(SERVICE_UUID);
 pAdvertising->setScanResponse(false);
 pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
 BLEDevice::startAdvertising();
 Serial.println("Waiting a client connection to notify...");

}