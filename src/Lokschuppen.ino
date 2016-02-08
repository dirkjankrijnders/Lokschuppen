#include <LocoNet.h>

#include "Bounce2.h"
#include "Door.h"

#include "Servo.h"

//#include <Delay.h>
#include <EEPROM.h>

#define annexLightPin 2
#define shedLightPin 3
#define servoEnablePin 15

/* 
    Door related stuff 
*/
Servo leftDoorServo;
Servo rightDoorServo;

Door leftDoor(&leftDoorServo, 170, 20, 1, 250);
Door rightDoor(&rightDoorServo, 20, 160, 1, 250);

bool annexLightsOn = false;
bool shedLightsOn = false;

void enableServos();
void disableServos();

void openDoors();
void closeDoors();
void toggleDoors(); 

/* 
    Serial related stuff
*/
int incomingByte = 0;

#define LNCV_COUNT 16

uint16_t lncv[LNCV_COUNT];

lnMsg *LnPacket;

LocoNetCVClass lnCV;

boolean programmingMode;

/* 
    Button related stuff
*/

Bounce bouncer = Bounce();
int inputPin = A0;

#define LOCONET_TX_PIN 6

#define LNCV_COUNT 16

#define ARTNR 5001
int sensor = 0;

void setup() {
  while (!Serial)
    Serial.begin(57600);

  Serial.println("Lokschuppen v0.0");  
 //LocoNet.init(LOCONET_TX_PIN);

  // put your setup code here, to run once:
  leftDoorServo.attach(9);
  rightDoorServo.attach(10);
  // Setup the button
  pinMode( inputPin ,INPUT);
  // Activate internal pull-up (optional) 
  digitalWrite( inputPin ,HIGH);
  // After setting up the button, setup the object
  bouncer .attach( inputPin );
  bouncer .interval(5);

  pinMode( annexLightPin ,OUTPUT);
  digitalWrite( annexLightPin, annexLightsOn);
  pinMode( shedLightPin ,OUTPUT);
  digitalWrite( shedLightPin, shedLightsOn);
  pinMode( servoEnablePin, OUTPUT);
  disableServos();
  lncv[0] = 1;
  for (int i(1); i < LNCV_COUNT; ++i) {
    lncv[i] = i;
  }
  //commitLNCVUpdate();
  programmingMode = false;

}


void commitLNCVUpdate() {
  Serial.print("Module Address is now: ");
  Serial.print(lncv[0]);
  Serial.print("\n");
}
void loop() {
  /*** SERIAL INTERFACE ***/
  if (Serial.available() > 0) {
      // read the incoming byte:
      incomingByte = Serial.read();
      Serial.print(incomingByte);
      if (incomingByte == 't') {
        toggleDoors();
      } else if (incomingByte == 'o') {
        openDoors();
      } else if (incomingByte == 'c') {
        closeDoors();
      } else if (incomingByte == 'a') {
        if (annexLightsOn) {
          annexLightsOn = false;
        } else {
          annexLightsOn = true;
        }
        Serial.print("annex lights: " );
        Serial.println(annexLightsOn);
        digitalWrite(annexLightPin, annexLightsOn);
      } else if (incomingByte == 'l') {
        if (shedLightsOn) {
          shedLightsOn = false;
        } else {
          shedLightsOn = true;
        }
        Serial.print("shed lights: " );
        Serial.println(shedLightsOn);
        digitalWrite(shedLightPin, shedLightsOn);
      } else if (incomingByte == 's') {
         if (sensor)
           sensor = 0;
         else 
            sensor = 1;
         LocoNet.reportSensor(1, sensor);
      }
  }
  if ((leftDoor.doorState() > 1) && (rightDoor.doorState() > 1))
	  disableServos();
  /*** PUSH BUTTON ***/
//  delay(1);
  leftDoor.update();
  rightDoor.update();
  if (bouncer.update()) {
    if (bouncer.read()) {
      toggleDoors();
      Serial.println(bouncer.read());
    }
  }
  /*** LOCONET ***/
/*  LnPacket = LocoNet.receive();
  if (LnPacket) {
    uint8_t packetConsumed(LocoNet.processSwitchSensorMessage(LnPacket));
    if (packetConsumed == 0) {
      Serial.print("Loop ");
      Serial.print((int)LnPacket);
      dumpPacket(LnPacket->ub);
      packetConsumed = lnCV.processLNCVMessage(LnPacket);
      Serial.print("End Loop\n");
    }
  }
*/
};

void toggleDoors() {
  if (leftDoor.doorState() == Door::DOOROPEN) {
    closeDoors();
  } else if (leftDoor.doorState() == Door::DOORCLOSED) {
    openDoors();
  }
}

void enableServos() {
	digitalWrite(servoEnablePin, HIGH);
}

void disableServos() {
	digitalWrite(servoEnablePin, LOW);
}

void closeDoors() {
	enableServos();
  rightDoor.setDelay(0);
  rightDoor.close();
  leftDoor.setDelay(250);
  leftDoor.close();
}

void openDoors() {
	enableServos();
  leftDoor.setDelay(0);
  leftDoor.open();
  rightDoor.setDelay(250);
  Serial.println(leftDoor.getDelay());
  Serial.println(rightDoor.getDelay());
  rightDoor.open();
}


void dumpPacket(UhlenbrockMsg & ub) {
  Serial.print(" PKT: ");
  Serial.print(ub.command, HEX);
  Serial.print(" ");
  Serial.print(ub.mesg_size, HEX);
  Serial.print(" ");
  Serial.print(ub.SRC, HEX);
  Serial.print(" ");
  Serial.print(ub.DSTL, HEX);
  Serial.print(" ");
  Serial.print(ub.DSTH, HEX);
  Serial.print(" ");
  Serial.print(ub.ReqId, HEX);
  Serial.print(" ");
  Serial.print(ub.PXCT1, HEX);
  Serial.print(" ");
  for (int i(0); i < 8; ++i) {
    Serial.print(ub.payload.D[i], HEX);
    Serial.print(" ");
  }
  Serial.write("\n");
}

  /**
   * Notifies the code on the reception of a read request.
   * Note that without application knowledge (i.e., art.-nr., module address
   * and "Programming Mode" state), it is not possible to distinguish
   * a read request from a programming start request message.
   */
int8_t notifyLNCVread(uint16_t ArtNr, uint16_t lncvAddress, uint16_t,
    uint16_t & lncvValue) {
  Serial.print("Enter notifyLNCVread(");
  Serial.print(ArtNr, HEX);
  Serial.print(", ");
  Serial.print(lncvAddress, HEX);
  Serial.print(", ");
  Serial.print(", ");
  Serial.print(lncvValue, HEX);
  Serial.print(")");
  // Step 1: Can this be addressed to me?
  // All ReadRequests contain the ARTNR. For starting programming, we do not accept the broadcast address.
  if (programmingMode) {
    if (ArtNr == ARTNR) {
      if (lncvAddress < 16) {
        lncvValue = lncv[lncvAddress];
        Serial.print(" LNCV Value: ");
        Serial.print(lncvValue);
        Serial.print("\n");
        return LNCV_LACK_OK;
      } else {
        // Invalid LNCV address, request a NAXK
        return LNCV_LACK_ERROR_UNSUPPORTED;
      }
    } else {
      Serial.print("ArtNr invalid.\n");
      return -1;
    }
  } else {
    Serial.print("Ignoring Request.\n");
    return -1;
  }
}

int8_t notifyLNCVprogrammingStart(uint16_t & ArtNr, uint16_t & ModuleAddress) {
  // Enter programming mode. If we already are in programming mode,
  // we simply send a response and nothing else happens.
  Serial.print("notifyLNCVProgrammingStart ");
  if (ArtNr == ARTNR) {
    Serial.print("artnrOK ");
    if (ModuleAddress == lncv[0]) {
      Serial.print("moduleUNI ENTERING PROGRAMMING MODE\n");
      programmingMode = true;
      return LNCV_LACK_OK;
    } else if (ModuleAddress == 0xFFFF) {
      Serial.print("moduleBC ENTERING PROGRAMMING MODE\n");
      ModuleAddress = lncv[0];
      return LNCV_LACK_OK;
    }
  }
  Serial.print("Ignoring Request.\n");
  return -1;
}

  /**
   * Notifies the code on the reception of a write request
   */
int8_t notifyLNCVwrite(uint16_t ArtNr, uint16_t lncvAddress,
    uint16_t lncvValue) {
  Serial.print("notifyLNCVwrite, ");
  //  dumpPacket(ub);
  if (!programmingMode) {
    Serial.print("not in Programming Mode.\n");
    return -1;
  }

  if (ArtNr == ARTNR) {
    Serial.print("Artnr OK, ");

    if (lncvAddress < 16) {
      lncv[lncvAddress] = lncvValue;
      return LNCV_LACK_OK;
    }
    else {
      return LNCV_LACK_ERROR_UNSUPPORTED;
    }

  }
  else {
    Serial.print("Artnr Invalid.\n");
    return -1;
  }
}

  /**
   * Notifies the code on the reception of a request to end programming mode
   */
void notifyLNCVprogrammingStop(uint16_t ArtNr, uint16_t ModuleAddress) {
  Serial.print("notifyLNCVprogrammingStop ");
  if (programmingMode) {
    if (ArtNr == ARTNR && ModuleAddress == lncv[0]) {
      programmingMode = false;
      Serial.print("End Programing Mode.\n");
      commitLNCVUpdate();
    }
    else {
      if (ArtNr != ARTNR) {
        Serial.print("Wrong Artnr.\n");
        return;
      }
      if (ModuleAddress != lncv[0]) {
        Serial.print("Wrong Module Address.\n");
        return;
      }
    }
  }
  else {
    Serial.print("Ignoring Request.\n");
  }
}

