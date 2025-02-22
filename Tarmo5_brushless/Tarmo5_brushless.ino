#include <ESP32Servo.h>
#include <Bluepad32.h>
#include <EEPROM.h>

//ARDUINO OTA LIBRARIES
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// define the number of bytes you want to access
#define EEPROM_SIZE 2
//  SERVO  //
#define motorPin 10
#define servoPin 8
Servo servo = Servo();
Servo motor = Servo();  //MOTOR ESC

//  BLUEPAD  //
ControllerPtr myController;

short throttle = 0;
short reverse = 0;
byte prevDpad;
byte dpad;
short steeringOffset = 0;
short steering = 0;
bool OTA = false;
byte steeringRange = 40;


// This callback gets called any time a new gamepad is connected.
// Up to 4 gamepads can be connected at the same time.
void onConnectedController(ControllerPtr ctl) {
  if (myController == nullptr) {
    Serial.print("CALLBACK: Controller is connected");
    // Additionally, you can get certain gamepad properties like:
    // Model, VID, PID, BTAddr, flags, etc.
    ControllerProperties properties = ctl->getProperties();
    Serial.printf("Controller model: %s, VID=0x%04x, PID=0x%04x\n", ctl->getModelName().c_str(), properties.vendor_id,
                  properties.product_id);
    myController = ctl;
  } else {
    Serial.println("CALLBACK: Controller connected, but could not found empty slot");
  }
}

void onDisconnectedController(ControllerPtr ctl) {
  motor.writeMicroseconds(1500);  //EMERGENCY STOP
  //stopped = true;
  throttle = reverse = 0;

  if (myController == ctl) {
    Serial.print("CALLBACK: Controller disconnected");
    myController = nullptr;
  } else {
    Serial.println("CALLBACK: Controller disconnected, but not found in myController");
  }
}


//void rumbleCheck(ControllerPtr ctl) {
//if (throttle == 2000) {
//if ((millis() - prevMillis) > 2){
//ctl->playDualRumble(0 /*delayedStartMs*/, 2 /*durationMs*/, 255 /*weakMagnitude*/, 255 /*strongMagnitude*/);
//prevMillis = millis();
//rumble 0-255
//}
//}

// Arduino setup function. Runs in CPU 1
void setup() {
  Serial.begin(115200);

  EEPROM.begin(EEPROM_SIZE);
  steeringOffset = EEPROM.read(0) - 127;  //get saved steering offset
  steeringRange = EEPROM.read(1); //get saved steering range

  Serial.printf("Firmware: %s\n", BP32.firmwareVersion());
  const uint8_t* addr = BP32.localBdAddress();
  Serial.printf("BD Addr: %2X:%2X:%2X:%2X:%2X:%2X\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

  // Setup the Bluepad32 callbacks
  BP32.setup(&onConnectedController, &onDisconnectedController);

  motor.attach(motorPin);
  servo.attach(servoPin);
  //servo.write(90 + steeringOffset); //centre servo
}

// Arduino loop function. Runs in CPU 1.
void loop() {
  if (OTA) {
    ArduinoOTA.handle();  // Handle OTA if enabled
  }
  // This call fetches all the controllers' data.
  // Call this function in your main loop.
  bool dataUpdated = BP32.update();
  if (dataUpdated) {
    processController(myController);
  }

  //rumbleCheck(myController);
}
