const int deadZoneMin = -15;
const int deadZoneMax = 15;
bool halfSpeed = false;  //child mode
bool halfSpeedBtn = false;
bool prevHalfSpeedBtn = false;
bool OTABtn = false;
bool prevOTABtn = false;
short buttons;
short prevButtons;

void processController(ControllerPtr ctl) {
  if (myController && myController->isConnected() && myController->hasData()) {
    // There are different ways to query whether a button is pressed.
    // By query each button individually:
    //  a(), b(), x(), y(), l1(), etc...
    OTABtn = ctl->miscHome();
    if ((OTABtn != prevOTABtn) && (OTABtn)) {
      OTA = !OTA;
      if (OTA) {
        ctl->playDualRumble(0 /*delayedStartMs*/, 1000 /*durationMs*/, 255 /*weakMagnitude*/, 255 /*strongMagnitude*/);
        enableOTA();
      } else {
        ctl->playDualRumble(0 /*delayedStartMs*/, 300 /*durationMs*/, 255 /*weakMagnitude*/, 255 /*strongMagnitude*/);
        disableOTA();
      }
    }

    halfSpeedBtn = ctl->miscCapture();  //middle misc button

    if ((halfSpeedBtn != prevHalfSpeedBtn) && (halfSpeedBtn)) {
      halfSpeed = !halfSpeed;  //toggle half speed
      if (halfSpeed) {
        ctl->playDualRumble(0 /*delayedStartMs*/, 300 /*durationMs*/, 255 /*weakMagnitude*/, 255 /*strongMagnitude*/);
      } else {
        ctl->playDualRumble(0 /*delayedStartMs*/, 100 /*durationMs*/, 255 /*weakMagnitude*/, 255 /*strongMagnitude*/);
      }
    }

    dpad = ctl->dpad();
    if (dpad != prevDpad) {
      if (dpad == 4) {  //right
        steeringOffset--;
        if (steeringOffset < -60) { steeringOffset = -60; }  //keep servo in working range
        ctl->playDualRumble(0 /*delayedStartMs*/, 100 /*durationMs*/, 255 /*weakMagnitude*/, 0 /*strongMagnitude*/);

      } else if (dpad == 8) {  //left
        steeringOffset++;
        if (steeringOffset > 60) { steeringOffset = 60; }  //keep servo in working range
        ctl->playDualRumble(0 /*delayedStartMs*/, 100 /*durationMs*/, 0 /*weakMagnitude*/, 255 /*strongMagnitude*/);

      } else if (dpad == 2) {  //down
        steeringOffset = 0;

      } else if (dpad == 1) {                     //up
        EEPROM.write(0, (steeringOffset + 127));  //convert to positive byte
        EEPROM.write(1, steeringRange);
        EEPROM.commit();
      }
    }
    buttons = ctl->buttons();
    if (buttons != prevButtons) {
      if (buttons == 8) {
        steeringRange++;
      } else if (buttons == 1) {
        steeringRange--;
      } else if (buttons == 4) {
        //RESET TO EEPROM SAVED VALUES
        steeringOffset = EEPROM.read(0) - 127;  //get saved steering offset
        steeringRange = EEPROM.read(1); //get saved steering range
      }
    }

    steering = ctl->axisX();
    //set joystick deadzone to prevent stick drift
    if (steering > deadZoneMin && steering < deadZoneMax) {
      steering = 90;
    } else {  //map to servo values
      if (steering > 0) {
        steering = map(steering, deadZoneMin + 1, 512, 91, (90+steeringRange));
      } else {
        steering = map(steering, deadZoneMax - 1, -511, 89, (90-steeringRange+1));
      }
    }
    steering += steeringOffset;  //add offset
    //Serial.println(halfSpeed);
    /*Serial.print("Throttle: ");
    Serial.print(throttle);
    Serial.print("---Reverse: ");
    Serial.print(reverse);
    Serial.print("Offset: ");
    Serial.print(steeringOffset);
    Serial.print("---Dpad: ");
    Serial.print(dpad);
    Serial.print("---Corrected steering: ");
    Serial.println(steering);*/

    servo.write(steering);
    throttle = ctl->throttle();
    reverse = ctl->brake();

    if (throttle > 0 && reverse == 0) {  //FORWARD
      if (halfSpeed) {
        throttle = map(throttle, 0, 1023, 1500, 1625);  //10 bit -> microseconds
      } else {
        throttle = map(throttle, 0, 1023, 1500, 2000);  //10 bit -> microseconds
      }
      motor.writeMicroseconds(throttle);

    } else if (reverse > 0 && throttle == 0) {  //REVERSE
      if (halfSpeed) {
        reverse = map(reverse, 0, 1023, 1500, 1375);  //10 bit -> microseconds
      } else {
        reverse = map(reverse, 0, 1023, 1500, 1000);  //10 bit -> microseconds
      }
      motor.writeMicroseconds(reverse);

    } else {  //NO GAS OR BRAKE
      motor.writeMicroseconds(1500);
      //stopped = true;
    }

    prevDpad = ctl->dpad();
    prevButtons = ctl->buttons();
    prevHalfSpeedBtn = ctl->miscCapture();
    prevOTABtn = ctl->miscHome();
  }
}


//Serial.print(ctl->miscSelect()); //left misc button
//Serial.println(ctl->miscStart()); //right misc button