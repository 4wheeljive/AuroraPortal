/* TODO:
- much
*/

#pragma once

#include "FastLED.h"
#include <ArduinoJson.h>

/* If you use more than ~4 characteristics, you need to increase numHandles in this file:
C:\Users\...\.platformio\packages\framework-arduinoespressif32\libraries\BLE\src\BLEServer.h
Setting numHandles = 60 has worked for 7 characteristics.  
*/

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <string>

#include <FS.h>
#include "LittleFS.h"
#define FORMAT_LITTLEFS_IF_FAILED true 

bool displayOn = true;
bool debug = true;
bool pauseAnimation = false;

uint8_t dummy = 1;

// UI Elements *************************************************************************************

   using namespace ArduinoJson;

   bool rotateAnimations = false;
   bool rotateWaves = false; 
   bool fancyTrigger = false;

   uint8_t cFxIndex = 0;
   uint8_t cBright = 75;
   uint8_t cColOrd = 0;                  

   float cSpeed = 1.f;
   float cZoom = 1.f;
   float cScale = 1.f; 
   float cAngle = 1.f; 
   float cTwist = 1.f;
   float cRadius = 1.0f; 
   float cEdge = 1.0f;
   float cZ = 1.f; 
   float cRatBase = 0.0f; 
   float cRatDiff= 1.f; 
   float cOffBase = 1.f; 
   float cOffDiff = 1.f; 
   float cRed = 1.f; 
   float cGreen = 1.f; 
   float cBlue = 1.f; 

   bool Layer1 = true;
   bool Layer2 = true;
   bool Layer3 = true;
   bool Layer4 = true;
   bool Layer5 = true;

   struct Preset {
      String pPresetID;
      String pPresetName;
      uint8_t pFxIndex;
      uint8_t pBright;
      uint8_t pColOrd;
      float pSpeed;
      float pZoom;
      float pScale;	
      float pAngle;
      float pTwist;
      float pRadius;
      float pEdge;	
      float pZ;	
      float pRatBase;
      float pRatDiff;
      float pOffBase;
      float pOffDiff;
      float pRed;
      float pGreen;	
      float pBlue;   
   };

   Preset presetD = {
      .pPresetID = "PresetD",
      .pPresetName = "Default",
      .pFxIndex = 0,
      .pBright = 75,
      .pColOrd = 0, 
      .pSpeed = 1.f,
      .pZoom = 1.f,
      .pScale = 1.f,
      .pAngle = 1.f,
      .pTwist = 1.f,
      .pRadius = 1.f,
      .pEdge = 1.f,
      .pZ = 1.f,
      .pRatBase = 0.f,
      .pRatDiff = 1.f,
      .pOffBase = 1.f,
      .pOffDiff = 1.f,
      .pRed = 1.f,
      .pGreen = 1.f, 
      .pBlue = 1.f,
   
   };

   Preset preset1 = {.pPresetID = "Preset1"};
   Preset preset2 = {.pPresetID = "Preset2"};
   Preset preset3 = {.pPresetID = "Preset3"};
   Preset preset4 = {.pPresetID = "Preset4"};
   Preset preset5 = {.pPresetID = "Preset5"};
   Preset preset6 = {.pPresetID = "Preset6"};
   Preset preset7 = {.pPresetID = "Preset7"};
   Preset preset8 = {.pPresetID = "Preset8"};
   Preset preset9 = {.pPresetID = "Preset9"};
   Preset preset10 = {.pPresetID = "Preset10"};
   
   void capturePreset(Preset &preset);
   void retrievePreset(const char* presetID, Preset &preset);

   ArduinoJson::JsonDocument sendDoc;
   ArduinoJson::JsonDocument receivedJSON;

//*******************************************************************************
//BLE CONFIGURATION *************************************************************

BLEServer* pServer = NULL;
BLECharacteristic* pButtonCharacteristic = NULL;
BLECharacteristic* pCheckboxCharacteristic = NULL;
BLECharacteristic* pNumberCharacteristic = NULL;
//BLECharacteristic* pControlCharacteristic = NULL;

bool deviceConnected = false;
bool wasConnected = false;

#define SERVICE_UUID                  	"19b10000-e8f2-537e-4f6c-d104768a1214"
#define BUTTON_CHARACTERISTIC_UUID     "19b10001-e8f2-537e-4f6c-d104768a1214"
#define CHECKBOX_CHARACTERISTIC_UUID   "19b10002-e8f2-537e-4f6c-d104768a1214"
#define NUMBER_CHARACTERISTIC_UUID     "19b10003-e8f2-537e-4f6c-d104768a1214"
//#define CONTROL_CHARACTERISTIC_UUID    "19b10004-e8f2-537e-4f6c-d104768a1214"

BLEDescriptor pButtonDescriptor(BLEUUID((uint16_t)0x2902));
BLEDescriptor pCheckboxDescriptor(BLEUUID((uint16_t)0x2902));
BLEDescriptor pNumberDescriptor(BLEUUID((uint16_t)0x2902));
//BLEDescriptor pControlDescriptor(BLEUUID((uint16_t)0x2902));

//*******************************************************************************
// CONTROL FUNCTIONS ************************************************************

void startWaves() {
   gCurrentPaletteNumber = random(0,gGradientPaletteCount-1);
   CRGBPalette16 gCurrentPalette( gGradientPalettes[gCurrentPaletteNumber] );
   gTargetPaletteNumber = addmod8( gCurrentPaletteNumber, 1, gGradientPaletteCount);
   gTargetPalette = gGradientPalettes[ gCurrentPaletteNumber ];
}

// UI update functions ***********************************************

void sendReceiptButton(uint8_t receivedValue) {
   pButtonCharacteristic->setValue(String(receivedValue).c_str());
   pButtonCharacteristic->notify();
   if (debug) {
      Serial.print("Button value received: ");
      Serial.println(receivedValue);
   }
}

void sendReceiptCheckbox(String receivedID, bool receivedValue) {
  
   // Prepare the JSON document to send
   sendDoc.clear();
   sendDoc["id"] = receivedID;
   sendDoc["val"] = receivedValue;

   // Convert the JSON document to a string
   String jsonString;
   serializeJson(sendDoc, jsonString);

   // Set the value of the characteristic
   pCheckboxCharacteristic->setValue(jsonString);
   
   // Notify connected clients
   pCheckboxCharacteristic->notify();
   
   if (debug) {
      Serial.print("Sent receipt for ");
      Serial.print(receivedID);
      Serial.print(": ");
      Serial.println(receivedValue);
   }
}

void sendReceiptNumber(String receivedID, float receivedValue) {
   // Prepare the JSON document to send
   sendDoc.clear();
   sendDoc["id"] = receivedID;
   sendDoc["val"] = receivedValue;

   // Convert the JSON document to a string
   String jsonString;
   serializeJson(sendDoc, jsonString);

   // Set the value of the characteristic
   pNumberCharacteristic->setValue(jsonString);
   
   // Notify connected clients
   pNumberCharacteristic->notify();
   
   if (debug) {
      Serial.print("Sent receipt for ");
      Serial.print(receivedID);
      Serial.print(": ");
      Serial.println(receivedValue);
   }
}

//***********************************************************************

void updateUI() {

   pauseAnimation = true;
   sendReceiptButton(cFxIndex);  
   sendReceiptNumber("inBright",cBright);
   sendReceiptNumber("inColOrd",cColOrd);
   sendReceiptNumber("inSpeed",cSpeed);
   sendReceiptNumber("inZoom",cZoom);
   sendReceiptNumber("inScale",cScale);
   sendReceiptNumber("inAngle",cAngle);
   sendReceiptNumber("inTwist",cTwist);
   sendReceiptNumber("inRadius",cRadius);
   sendReceiptNumber("inEdge",cEdge);
   sendReceiptNumber("inZ",cZ);
   sendReceiptNumber("inRatBase",cRatBase);
   sendReceiptNumber("inRatDiff",cRatDiff);
   sendReceiptNumber("inOffBase",cOffBase);
   sendReceiptNumber("inOffDiff",cOffDiff);
   sendReceiptNumber("inRed",cRed);
   sendReceiptNumber("inGreen",cGreen);
   sendReceiptNumber("inBlue",cBlue);
   pauseAnimation = false;

}

void resetAll() {

   pauseAnimation = true;

   if (cSpeed != presetD.pSpeed){
      cSpeed = presetD.pSpeed;
      sendReceiptNumber("inSpeed",cSpeed);
   };   

   if (cZoom != presetD.pZoom){
      cZoom = presetD.pZoom;
      sendReceiptNumber("inZoom",cZoom);
   };   

   if (cScale != presetD.pScale){
      cScale = presetD.pScale;
      sendReceiptNumber("inScale",cScale);
   };   

   if (cAngle != presetD.pAngle){
      cAngle = presetD.pAngle;
      sendReceiptNumber("inAngle",cAngle);
   };   

   if (cTwist != presetD.pTwist){
      cTwist = presetD.pTwist;
      sendReceiptNumber("inTwist",cTwist);
   };   

   if (cRadius != presetD.pRadius){
      cRadius = presetD.pRadius;
      sendReceiptNumber("inRadius",cRadius);
      };   

   if (cEdge != presetD.pEdge){
      cEdge = presetD.pEdge;
      sendReceiptNumber("inEdge",cEdge);
      };   

   if (cZ != presetD.pZ){
      cZ = presetD.pZ;
      sendReceiptNumber("inZ",cZ);
      };   

   if (cRatBase != presetD.pRatBase){
      cRatBase = presetD.pRatBase;
      sendReceiptNumber("inRatBase",cRatBase);
      };   

   if (cRatDiff != presetD.pRatDiff){
      cRatDiff = presetD.pRatDiff;
      sendReceiptNumber("inRatDiff",cRatDiff);
      };   

   if (cOffBase != presetD.pOffBase){
      cOffBase = presetD.pOffBase;
      sendReceiptNumber("inOffBase",cOffBase);
   };   

   if (cOffDiff != presetD.pOffDiff){
      cOffDiff = presetD.pOffDiff;
      sendReceiptNumber("inOffDiff",cOffDiff);
   };   

   pauseAnimation = false;

}

// Handle UI request functions ***********************************************

void processButton(uint8_t receivedValue) {

   sendReceiptButton(receivedValue);
      
   if (receivedValue < 20) { // Program selection

      PROGRAM = receivedValue;
      MODE = 0;
      displayOn = true;
      
      if (PROGRAM==1) startWaves();
  
   }
   
   if (receivedValue >= 20 && receivedValue < 40) { // Mode selection
      MODE = receivedValue - 20;
      cFxIndex = MODE;
      displayOn = true;
   }

   if (receivedValue == 51) { capturePreset(preset1); }
   if (receivedValue == 52) { capturePreset(preset2); }
   if (receivedValue == 53) { capturePreset(preset3); }
   if (receivedValue == 54) { capturePreset(preset4); }
   if (receivedValue == 55) { capturePreset(preset5); }
   if (receivedValue == 56) { capturePreset(preset6); }
   if (receivedValue == 57) { capturePreset(preset7); }
   if (receivedValue == 58) { capturePreset(preset8); }
   if (receivedValue == 59) { capturePreset(preset9); }
   if (receivedValue == 60) { capturePreset(preset10); }
   if (receivedValue == 71) { retrievePreset("Preset1",preset1); }
   if (receivedValue == 72) { retrievePreset("Preset2",preset2); }
   if (receivedValue == 73) { retrievePreset("Preset3",preset3); }
   if (receivedValue == 74) { retrievePreset("Preset4",preset4); }
   if (receivedValue == 75) { retrievePreset("Preset5",preset5); }
   if (receivedValue == 76) { retrievePreset("Preset6",preset6); }
   if (receivedValue == 77) { retrievePreset("Preset7",preset7); }
   if (receivedValue == 78) { retrievePreset("Preset8",preset8); }
   if (receivedValue == 79) { retrievePreset("Preset9",preset9); }
   if (receivedValue == 80) { retrievePreset("Preset10",preset10); }

   if (receivedValue == 91) { updateUI(); }
   if (receivedValue == 94) { fancyTrigger = true; }
   if (receivedValue == 95) { resetAll(); }
   
   if (receivedValue == 98) { displayOn = true; }
   if (receivedValue == 99) { displayOn = false; }

}

void processNumber(String receivedID, float receivedValue ) {

   sendReceiptNumber(receivedID, receivedValue);

   if (receivedID == "inBright") {
      cBright = receivedValue;
      BRIGHTNESS = cBright;
      FastLED.setBrightness(BRIGHTNESS);
   };
   if (receivedID == "inColOrd") {cColOrd = receivedValue;};
   if (receivedID == "inSpeed") {cSpeed = receivedValue;};

   if (receivedID == "inPalNum") {
      uint8_t newPalNum = receivedValue;
      gTargetPalette = gGradientPalettes[ newPalNum ];
      if(debug) {
         Serial.print("newPalNum: ");
         Serial.println(newPalNum);
      }
   };
  
   if (receivedID == "inZoom") {cZoom = receivedValue;};    
   if (receivedID == "inScale") {cScale = receivedValue;};	
   if (receivedID == "inAngle") {cAngle = receivedValue;};	
   if (receivedID == "inTwist") {cTwist = receivedValue;};
   if (receivedID == "inRadius") {cRadius = receivedValue;};
   if (receivedID == "inEdge") {cEdge = receivedValue;};	
   if (receivedID == "inZ") {cZ = receivedValue;};	
   if (receivedID == "inRatBase") {cRatBase = receivedValue;};
   if (receivedID == "inRatDiff") {cRatDiff = receivedValue;};
   if (receivedID == "inOffBase") {cOffBase = receivedValue;};
   if (receivedID == "inOffDiff") {cOffDiff = receivedValue;};
   if (receivedID == "inRed") {cRed = receivedValue;};	
   if (receivedID == "inGreen") {cGreen = receivedValue;};	
   if (receivedID == "inBlue") {cBlue = receivedValue;};

}

void processCheckbox(String receivedID, bool receivedValue ) {
 
   sendReceiptCheckbox(receivedID, receivedValue);

   if (receivedID == "cx10") {rotateWaves = receivedValue;};
   if (receivedID == "cxLayer1") {Layer1 = receivedValue;};
   if (receivedID == "cxLayer2") {Layer2 = receivedValue;};
   if (receivedID == "cxLayer3") {Layer3 = receivedValue;};
   if (receivedID == "cxLayer4") {Layer4 = receivedValue;};
   if (receivedID == "cxLayer5") {Layer5 = receivedValue;};
    
}

//*******************************************************************************
// PRESETS **********************************************************************

void savePreset(const char* presetID, const Preset &preset) {
      
   String path = "/";
   path += presetID;
   path += ".txt"; 
   File file = LittleFS.open(path, "w");

   if (!file) {
      Serial.print("Failed to open file for writing: ");
      Serial.println(path);
   }

   file.printf("%s\n", preset.pPresetName);
   file.printf("%d\n", preset.pFxIndex);
   file.printf("%d\n", preset.pBright);
   file.printf("%d\n", preset.pColOrd);
   file.printf("%f\n", preset.pSpeed);
   file.printf("%f\n", preset.pZoom);
   file.printf("%f\n", preset.pScale);
   file.printf("%f\n", preset.pAngle);
   file.printf("%f\n", preset.pTwist);
   file.printf("%f\n", preset.pRadius);
   file.printf("%f\n", preset.pEdge);
   file.printf("%f\n", preset.pZ);
   file.printf("%f\n", preset.pRatBase);
   file.printf("%f\n", preset.pRatDiff);
   file.printf("%f\n", preset.pOffBase);
   file.printf("%f\n", preset.pOffDiff);
   file.printf("%f\n", preset.pRed);
   file.printf("%f\n", preset.pGreen);
   file.printf("%f\n", preset.pBlue);
   
   file.close();
   Serial.print("Preset saved to ");
   Serial.println(path);

}

//***************************************************************

void capturePreset(Preset &preset) {
  
   pauseAnimation = true;

   preset.pFxIndex = cFxIndex;
   preset.pBright = cBright;
   preset.pColOrd = cColOrd;
   preset.pSpeed = cSpeed;
   preset.pZoom = cZoom; 
   preset.pScale = cScale; 	
   preset.pAngle = cAngle; 
   preset.pTwist = cTwist; 
   preset.pRadius = cRadius; 
   preset.pEdge = cEdge; 	
   preset.pZ = cZ; 	
   preset.pRatBase = cRatBase; 
   preset.pRatDiff = cRatDiff; 
   preset.pOffBase = cOffBase; 
   preset.pOffDiff = cOffDiff; 
   preset.pRed = cRed; 
   preset.pGreen = cGreen; 	
   preset.pBlue = cBlue; 
   
   savePreset(preset.pPresetID.c_str(), preset);

   pauseAnimation = false;

}

//***************************************************************

void applyPreset(const Preset &preset) {
   
   pauseAnimation = true;

   if (cFxIndex != preset.pFxIndex){
      cFxIndex = preset.pFxIndex;
      sendReceiptButton(cFxIndex);  
   };   

   if (cBright != preset.pBright){
      cBright = preset.pBright;
      sendReceiptNumber("inBright",cBright);
   };   

   if (cColOrd != preset.pColOrd){
      cColOrd = preset.pColOrd;
      sendReceiptNumber("inColOrd",cColOrd);
   };   

   if (cSpeed != preset.pSpeed){
      cSpeed = preset.pSpeed;
      sendReceiptNumber("inSpeed",cSpeed);
   };   

   if (cZoom != preset.pZoom){
      cZoom = preset.pZoom;
      sendReceiptNumber("inZoom",cZoom);
   };   

   if (cScale != preset.pScale){
      cScale = preset.pScale;
      sendReceiptNumber("inScale",cScale);
   };   

   if (cAngle != preset.pAngle){
      cAngle = preset.pAngle;
      sendReceiptNumber("inAngle",cAngle);
   };   

   if (cTwist != preset.pTwist){
      cTwist = preset.pTwist;
      sendReceiptNumber("inTwist",cTwist);
   };   

   if (cRadius != preset.pRadius){
      cRadius = preset.pRadius;
      sendReceiptNumber("inRadius",cRadius);
      };   

   if (cEdge != preset.pEdge){
      cEdge = preset.pEdge;
      sendReceiptNumber("inEdge",cEdge);
      };   

   if (cZ != preset.pZ){
      cZ = preset.pZ;
      sendReceiptNumber("inZ",cZ);
      };   

   if (cRatBase != preset.pRatBase){
      cRatBase = preset.pRatBase;
      sendReceiptNumber("inRatBase",cRatBase);
      };   

   if (cRatDiff != preset.pRatDiff){
      cRatDiff = preset.pRatDiff;
      sendReceiptNumber("inRatDiff",cRatDiff);
      };   

   if (cOffBase != preset.pOffBase){
      cOffBase = preset.pOffBase;
      sendReceiptNumber("inOffBase",cOffBase);
   };   

   if (cOffDiff != preset.pOffDiff){
      cOffDiff = preset.pOffDiff;
      sendReceiptNumber("inOffDiff",cOffDiff);
   };   

   if (cRed != preset.pRed){
      cRed = preset.pRed;
      sendReceiptNumber("inRed",cRed);
   };   

   if (cGreen != preset.pGreen){
      cGreen = preset.pGreen;
      sendReceiptNumber("inGreen",cGreen);
   };   

   if (cBlue != preset.pBlue){
      cBlue = preset.pBlue;
      sendReceiptNumber("inBlue",cBlue);
   };   

   pauseAnimation = false;

}

//***************************************************************

void retrievePreset(const char* presetID, Preset &preset) {
   String path = "/";
   path += presetID;
   path += ".txt"; 
   File file = LittleFS.open(path, "r");

   if (!file) {
      Serial.print("Failed to open file for reading: ");
      Serial.println(path);
   }
  
   preset.pPresetName = file.readStringUntil('\n');
   preset.pFxIndex = file.readStringUntil('\n').toInt();
   preset.pBright = file.readStringUntil('\n').toInt();
   preset.pColOrd = file.readStringUntil('\n').toInt();
   preset.pSpeed = file.readStringUntil('\n').toFloat();
   preset.pZoom = file.readStringUntil('\n').toFloat();
   preset.pScale = file.readStringUntil('\n').toFloat();
   preset.pAngle = file.readStringUntil('\n').toFloat();
   preset.pTwist = file.readStringUntil('\n').toFloat();
   preset.pRadius = file.readStringUntil('\n').toFloat();
   preset.pEdge = file.readStringUntil('\n').toFloat();
   preset.pZ = file.readStringUntil('\n').toFloat();
   preset.pRatBase = file.readStringUntil('\n').toFloat();
   preset.pRatDiff = file.readStringUntil('\n').toFloat();
   preset.pOffBase = file.readStringUntil('\n').toFloat();
   preset.pOffDiff = file.readStringUntil('\n').toFloat();
   preset.pRed = file.readStringUntil('\n').toFloat();
   preset.pGreen = file.readStringUntil('\n').toFloat();
   preset.pBlue = file.readStringUntil('\n').toFloat();

   file.close();
   Serial.print("Preset loaded from: ");
   Serial.println(path);

   applyPreset(preset);

}

//*******************************************************************************
// CALLBACKS ********************************************************************

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

class ButtonCharacteristicCallbacks : public BLECharacteristicCallbacks {
   void onWrite(BLECharacteristic *characteristic) {

      String value = characteristic->getValue();
      if (value.length() > 0) {
         
         uint8_t receivedValue = value[0];
         
         if (debug) {
            Serial.print("Button value received: ");
            Serial.println(receivedValue);
         }
         
         processButton(receivedValue);
        
      }
   }
};

class CheckboxCharacteristicCallbacks : public BLECharacteristicCallbacks {
   void onWrite(BLECharacteristic *characteristic) {
  
      String receivedBuffer = characteristic->getValue();
  
      if (receivedBuffer.length() > 0) {
                  
         if (debug) {
            Serial.print("Received buffer: ");
            Serial.println(receivedBuffer);
         }
      
         ArduinoJson::deserializeJson(receivedJSON, receivedBuffer);
         String receivedID = receivedJSON["id"] ;
         bool receivedValue = receivedJSON["val"];
      
         if (debug) {
            Serial.print(receivedID);
            Serial.print(": ");
            Serial.println(receivedValue);
         }
      
         processCheckbox(receivedID, receivedValue);
      
      }
   }
};

class NumberCharacteristicCallbacks : public BLECharacteristicCallbacks {
   void onWrite(BLECharacteristic *characteristic) {
      
      String receivedBuffer = characteristic->getValue();
      
      if (receivedBuffer.length() > 0) {
      
         if (debug) {
            Serial.print("Received buffer: ");
            Serial.println(receivedBuffer);
         }
      
         ArduinoJson::deserializeJson(receivedJSON, receivedBuffer);
         String receivedID = receivedJSON["id"] ;
         float receivedValue = receivedJSON["val"];
      
         if (debug) {
            Serial.print(receivedID);
            Serial.print(": ");
            Serial.println(receivedValue);
         }
      
         processNumber(receivedID, receivedValue);
      }
   }
};

//*******************************************************************************
// BLE SETUP FUNCTION ***********************************************************

void bleSetup() {

   BLEDevice::init("Aurora Portal");

   pServer = BLEDevice::createServer();
   pServer->setCallbacks(new MyServerCallbacks());

   BLEService *pService = pServer->createService(SERVICE_UUID);

   pButtonCharacteristic = pService->createCharacteristic(
                     BUTTON_CHARACTERISTIC_UUID,
                     BLECharacteristic::PROPERTY_WRITE |
                     BLECharacteristic::PROPERTY_READ |
                     BLECharacteristic::PROPERTY_NOTIFY
                  );
   pButtonCharacteristic->setCallbacks(new ButtonCharacteristicCallbacks());
   pButtonCharacteristic->setValue(String(cFxIndex).c_str());
   pButtonCharacteristic->addDescriptor(new BLE2902());

   pCheckboxCharacteristic = pService->createCharacteristic(
                     CHECKBOX_CHARACTERISTIC_UUID,
                     BLECharacteristic::PROPERTY_WRITE |
                     BLECharacteristic::PROPERTY_READ |
                     BLECharacteristic::PROPERTY_NOTIFY
                  );
   pCheckboxCharacteristic->setCallbacks(new CheckboxCharacteristicCallbacks());
   pCheckboxCharacteristic->setValue(String(dummy).c_str());
   pCheckboxCharacteristic->addDescriptor(new BLE2902());
   
   pNumberCharacteristic = pService->createCharacteristic(
                     NUMBER_CHARACTERISTIC_UUID,
                     BLECharacteristic::PROPERTY_WRITE |
                     BLECharacteristic::PROPERTY_READ |
                     BLECharacteristic::PROPERTY_NOTIFY
                  );
   pNumberCharacteristic->setCallbacks(new NumberCharacteristicCallbacks());
   pNumberCharacteristic->setValue(String(dummy).c_str());
   pNumberCharacteristic->addDescriptor(new BLE2902());

	/*
   pControlCharacteristic = pService->createCharacteristic(
                     CONTROL_CHARACTERISTIC_UUID,
                     BLECharacteristic::PROPERTY_WRITE |
                     BLECharacteristic::PROPERTY_READ |
                     BLECharacteristic::PROPERTY_NOTIFY
                  );
   //pControlCharacteristic->setCallbacks(new ControlCharacteristicCallbacks());
   pControlCharacteristic->setValue(String(dummy).c_str());
   pControlCharacteristic->addDescriptor(new BLE2902());
   */
      
   //**********************************************************

   pService->start();

   BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
   pAdvertising->addServiceUUID(SERVICE_UUID);
   pAdvertising->setScanResponse(false);
   pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
   BLEDevice::startAdvertising();
   if (debug) {Serial.println("Waiting a client connection to notify...");}

}
