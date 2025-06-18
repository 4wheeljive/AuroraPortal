/* Be sure to set numHandles = 60 in this file:
C:\Users\Jeff\.platformio\packages\framework-arduinoespressif32\libraries\BLE\src\BLEServer.h
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <string>
#include "fx/2d/blend.h"

//BLE configuration *************************************************************

BLEServer* pServer = NULL;
BLECharacteristic* pProgramCharacteristic = NULL;
BLECharacteristic* pModeCharacteristic = NULL;
BLECharacteristic* pBrightnessCharacteristic = NULL;
BLECharacteristic* pSpeedCharacteristic = NULL;
BLECharacteristic* pPaletteCharacteristic = NULL;
BLECharacteristic* pControlCharacteristic = NULL;
bool deviceConnected = false;
bool wasConnected = false;

#define SERVICE_UUID                  	"19b10000-e8f2-537e-4f6c-d104768a1214"
#define PROGRAM_CHARACTERISTIC_UUID   	"19b10001-e8f2-537e-4f6c-d104768a1214"
#define MODE_CHARACTERISTIC_UUID   		"19b10002-e8f2-537e-4f6c-d104768a1214"
#define BRIGHTNESS_CHARACTERISTIC_UUID "19b10003-e8f2-537e-4f6c-d104768a1214"
#define SPEED_CHARACTERISTIC_UUID 		"19b10004-e8f2-537e-4f6c-d104768a1214"
#define PALETTE_CHARACTERISTIC_UUID 	"19b10005-e8f2-537e-4f6c-d104768a1214"
#define CONTROL_CHARACTERISTIC_UUID    "19b10006-e8f2-537e-4f6c-d104768a1214"

//BLEDescriptor pBrightnessDescriptor(BLEUUID((uint16_t)0x2901));
//BLEDescriptor pSpeedDescriptor(BLEUUID((uint16_t)0x2902));
//BLEDescriptor pPaletteDescriptor(BLEUUID((uint16_t)0x2903));

//Control functions***************************************************************

void programAdjust(uint8_t newProgram) {
   pProgramCharacteristic->setValue(String(newProgram).c_str());
   if (debug) {
      Serial.print("Program: ");
      Serial.println(newProgram);
   }
}

void modeAdjust(uint8_t newMode) {
   pProgramCharacteristic->setValue(String(newMode).c_str());
   if (debug) {
      Serial.print("Mode: ");
      Serial.println(newMode);
   }
}

void brightnessAdjust(uint8_t newBrightness) {
   BRIGHTNESS = newBrightness;
   //brightnessChanged = true;
   FastLED.setBrightness(BRIGHTNESS);
   pBrightnessCharacteristic->setValue(String(BRIGHTNESS).c_str());
   if (debug) {
      Serial.print("Brightness: ");
      Serial.println(BRIGHTNESS);
   }
}

void speedAdjust(uint8_t newSpeed) {
   SPEED = newSpeed;
   speedfactor = SPEED/6; 
   pSpeedCharacteristic->setValue(String(SPEED).c_str());
   pSpeedCharacteristic->notify();
   if (debug) {
      Serial.print("Speed: ");
      Serial.println(SPEED);
   }
}

void startWaves() {
   if (rotateWaves) { gCurrentPaletteNumber = random(0,gGradientPaletteCount); }
   CRGBPalette16 gCurrentPalette( gGradientPalettes[gCurrentPaletteNumber] );
   pPaletteCharacteristic->setValue(String(gCurrentPaletteNumber).c_str());
   pPaletteCharacteristic->notify();
   if (debug) {
      Serial.print("Color palette: ");
      Serial.println(gCurrentPaletteNumber);
   }
   gCurrentPaletteNumber = addmod8( gCurrentPaletteNumber, 1, gGradientPaletteCount);
   gTargetPalette = gGradientPalettes[ gCurrentPaletteNumber ];
}

//Callbacks***********************************************************************

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    wasConnected = true;
    if (debug) {Serial.println("Device Connected");}
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    wasConnected = true;
  }
};

class ProgramCharacteristicCallbacks : public BLECharacteristicCallbacks {
 void onWrite(BLECharacteristic *characteristic) {
    String value = characteristic->getValue();
    if (value.length() > 0) {
       uint8_t receivedValue = value[0];
       if (debug) {
         Serial.print("Program: ");
         Serial.println(receivedValue);
       }
       
       if (receivedValue != 99) {
       
         if (receivedValue == 1) {//rainbowmatrix
            displayOn = true;
            PROGRAM = 1;
         }
         if (receivedValue == 2) { //pride
            displayOn = true;
            runPride = true;
            runWaves = false;
            PROGRAM = 2;
         }
         if (receivedValue == 3) { //waves
            displayOn = true;
            runPride = false;
            runWaves = true;
            PROGRAM = 2;
            startWaves();
         }
         if (receivedValue == 4) { //soapbubble
            displayOn = true;
            PROGRAM = 3;
         }
         if (receivedValue == 5) { //dots
            displayOn = true;
            PROGRAM = 4;
         }
         if (receivedValue == 6) { //fxWave2d
            displayOn = true;
            PROGRAM = 5;
         }
         if (receivedValue == 7) { //radii
            displayOn = true;
            PROGRAM = 6;
         }
         if (receivedValue == 8) { //waving cells
            displayOn = true;
            PROGRAM = 9;
         }
	      
         programAdjust(PROGRAM);
       
       }

       if (receivedValue == 99) { //off
          displayOn = false;
       }
	
      }
   }
};

class ModeCharacteristicCallbacks : public BLECharacteristicCallbacks {
 void onWrite(BLECharacteristic *characteristic) {
    String value = characteristic->getValue();
    if (value.length() > 0) {
       uint8_t receivedValue = value[0]; 
       if (debug) {
         Serial.print("Mode: ");
         Serial.println(receivedValue);
       }
       radiiMode = receivedValue;
       modeAdjust(radiiMode);
	}
 }
};

class BrightnessCharacteristicCallbacks : public BLECharacteristicCallbacks {
 void onWrite(BLECharacteristic *characteristic) {
    String value = characteristic->getValue();
    if (value.length() > 0) {
       uint8_t receivedValue = value[0]; 
       if (debug) {
         Serial.print("Brightness adjust: ");
         Serial.println(receivedValue);
       }
       if (receivedValue == 1) {
          uint8_t newBrightness = min(BRIGHTNESS + brightnessInc,255);
          brightnessAdjust(newBrightness);
       }
       if (receivedValue == 2) {
          uint8_t newBrightness = max(BRIGHTNESS - brightnessInc,0);
          brightnessAdjust(newBrightness);
       }
    }
 }
};

class SpeedCharacteristicCallbacks : public BLECharacteristicCallbacks {
 void onWrite(BLECharacteristic *characteristic) {
    String value = characteristic->getValue();
    if (value.length() > 0) {
       uint8_t receivedValue = value[0]; 
       if (debug) {
         Serial.print("Speed adjust: ");
         Serial.println(receivedValue);
       }
       if (receivedValue == 1) {
          uint8_t newSpeed = min(SPEED+1,10);
          speedAdjust(newSpeed);
       }
       if (receivedValue == 2) {
          uint8_t newSpeed = max(SPEED-1,0);
          speedAdjust(newSpeed);
       }
    }
 }
};

class PaletteCharacteristicCallbacks : public BLECharacteristicCallbacks {
 void onWrite(BLECharacteristic *characteristic) {
    String value = characteristic->getValue();
    if (value.length() > 0) {
       uint8_t receivedValue = value[0]; 
       if (debug) {
         Serial.print("Palette: ");
         Serial.println(receivedValue);
       }
       gTargetPalette = gGradientPalettes[ receivedValue ];
       pPaletteCharacteristic->setValue(String(receivedValue).c_str());
       pPaletteCharacteristic->notify();
    }
 }
};

class ControlCharacteristicCallbacks : public BLECharacteristicCallbacks {
 void onWrite(BLECharacteristic *characteristic) {
    String value = characteristic->getValue();
    if (value.length() > 0) {
      uint8_t receivedValue = value[0];
      if (debug) {
         Serial.print("Control: ");
         Serial.println(receivedValue);
       } 
	   if (receivedValue == 1) {
          fancyTrigger = true;
      }
      if (receivedValue == 100) {
          rotateWaves = true;
      }
      if (receivedValue == 101) {
         rotateWaves = false;
	  }
    }
 }
};

//*******************************************************************************************

void bleSetup() {

 BLEDevice::init("Aurora Portal");

 pServer = BLEDevice::createServer();
 pServer->setCallbacks(new MyServerCallbacks());

 BLEService *pService = pServer->createService(SERVICE_UUID);

 pProgramCharacteristic = pService->createCharacteristic(
                      PROGRAM_CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_WRITE |
					  BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
 pProgramCharacteristic->setCallbacks(new ProgramCharacteristicCallbacks());
 //pProgramCharacteristic->addDescriptor(new BLE2902());


 pModeCharacteristic = pService->createCharacteristic(
                      MODE_CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_WRITE |
					  BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
 pModeCharacteristic->setCallbacks(new ModeCharacteristicCallbacks());
 //pModeCharacteristic->addDescriptor(new BLE2902());

 pBrightnessCharacteristic = pService->createCharacteristic(
                      BRIGHTNESS_CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
 pBrightnessCharacteristic->setValue(String(BRIGHTNESS).c_str()); 
 //pBrightnessCharacteristic->addDescriptor(new BLE2902());
 //pBrightnessDescriptor.setValue("Brightness");

 pSpeedCharacteristic = pService->createCharacteristic(
                      SPEED_CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
 pSpeedCharacteristic->setValue(String(SPEED).c_str());
 //pSpeedCharacteristic->addDescriptor(new BLE2902());
 //pSpeedDescriptor.setValue("Speed"); 

 pPaletteCharacteristic = pService->createCharacteristic(
                      PALETTE_CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
 pPaletteCharacteristic->setCallbacks(new PaletteCharacteristicCallbacks());
 pPaletteCharacteristic->setValue(String(gCurrentPaletteNumber).c_str());
 //pPaletteCharacteristic->addDescriptor(new BLE2902());
 //pPaletteDescriptor.setValue("Palette"); 

 pControlCharacteristic = pService->createCharacteristic(
                      CONTROL_CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_WRITE |
					  BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
 pControlCharacteristic->setCallbacks(new ControlCharacteristicCallbacks());
 //pControlCharacteristic->addDescriptor(new BLE2902());

//**************************************************************************************

 pService->start();

 BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
 pAdvertising->addServiceUUID(SERVICE_UUID);
 pAdvertising->setScanResponse(false);
 pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
 BLEDevice::startAdvertising();
 if (debug) {Serial.println("Waiting a client connection to notify...");}

}