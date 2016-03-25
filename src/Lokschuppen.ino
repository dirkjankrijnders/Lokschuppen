
#define DEBUG_OUTPUT
//#undef DEBUG_OUTPUT

#ifdef DEBUG_OUTPUT
#define USE_SERIAL
#define DEBUG(x) Serial.print(x)
#define DEBUGLN(x) Serial.println(x)
#else
#define DEBUG(x)
#define DEBUGLN(x)
#endif

#include <LocoNet.h>
#include <EEPROM.h>

#include "Bounce2.h"
#include "Door.h"
#include "Servo.h"

#include "decoder_conf.h"
#include "configuredpins.h"
#include "cvaccess.h"

#define annexLightPin 2
#define shedLightPin 3
#define servoEnablePin 15

#define sensorAddress 2001
#define doorAddress 2002
#define shedLightAddress 2003
#define annexLightAddress 2004

#define ARTNR 11001

decoder_conf_t EEMEM _CV = {
#include "default_conf.h"
};

#define DoorStateAddress (uint16_t*)250
/* 
    Door related stuff 
*/

Servo leftDoorServo;
Servo rightDoorServo;

Door leftDoor(&leftDoorServo, 170, 20, 1, 250);
Door rightDoor(&rightDoorServo, 20, 160, 1, 250);
DoubleDoor doors(&leftDoor, &rightDoor, 250, doorAddress);

bool annexLightsOn = false;
bool shedLightsOn = false;

void enableServos();
void disableServos();

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
boolean boucerPrevState;

#define LOCONET_TX_PIN 5

int sensor = 0;
bool pins_busy = false;
/*
	Switch related stuff
*/
#define MAX 2
ConfiguredPin* confpins[MAX];

void reportSwitch(uint16_t Address, uint16_t state){
#ifdef DS54
	byte AddrH = ( (--Address >> 7) & 0x0F ) | OPC_SW_REP_INPUTS  ;
	byte AddrL = ( Address) & 0x7F ;

	if ( state == 0 )
		AddrH |= OPC_SW_REP_SW  ;
	if ( state == 1 )
		AddrH |= OPC_SW_REP_HI  ;
	if ( state == 2 )
		AddrH |= OPC_SW_REP_HI | OPC_SW_REP_SW ;
	
	LocoNet.send(OPC_SW_REP, AddrL, AddrH );
//  LocoNet.reportSwitch(address);
#else
	if ( state == 0 ) {
		LocoNet.reportSensor(Address, 0);
		LocoNet.reportSensor(Address+10, 0);
	} else if( state == 1 ) {
		LocoNet.reportSensor(Address, 1);
		LocoNet.reportSensor(Address+10, 0);		
	} else if( state == 2 ) {
		LocoNet.reportSensor(Address, 0);
		LocoNet.reportSensor(Address+10, 1);
	}
#endif
}

void reportSensor(uint16_t address, bool state) {
  LocoNet.reportSensor(address, state);
}

void setup() {
  pinMode( servoEnablePin, OUTPUT);
  disableServos();
#ifdef USE_SERIAL
  while (!Serial)
    Serial.begin(57600);
#endif

  DEBUGLN("Lokschuppen v0.0");  
  LocoNet.init(LOCONET_TX_PIN);

  // put your setup code here, to run once:
  leftDoorServo.attach(9);
  rightDoorServo.attach(10);
  if (eeprom_read_word(DoorStateAddress) == 1) {
	  doors.open();
  } else {
	  doors.close();
  }
  
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

  uint8_t i = 0;
  uint8_t pin_config;
  uint16_t pin, address, pos1, pos2, speed;
  
  for (i = 0; i < 2; i++) {
    pin_config = eeprom_read_byte((uint8_t*)&(_CV.pin_conf[i]));
    pin   = eeprom_read_word((uint16_t*)&(_CV.conf[i].servo.arduinopin));
    address = eeprom_read_word((uint16_t*)&(_CV.conf[i].servo.address));
    switch (pin_config) {
      case 2: //Servo
        //ServoSwitch(i,0);
        pos1  = eeprom_read_word((uint16_t*)&(_CV.conf[i].servo.pos1));
        pos2  = eeprom_read_word((uint16_t*)&(_CV.conf[i].servo.pos2));
        speed = eeprom_read_word((uint16_t*)&(_CV.conf[i].servo.speed));
        confpins[i] = new ServoSwitch(i, pin, address, pos1, pos2, speed, servoEnablePin);
		confpins[i]->restore_state(eeprom_read_word((uint16_t*)&(_CV.conf[i].servo.state)));
        break;
      default:
        confpins[i] = new InputPin(i, pin, address);
        break;
    }  
    confpins[i]->print();
  }
  programmingMode = false;

}

void loop() {
  /*** SERIAL INTERFACE ***/
#ifdef USESERIAL
  if (Serial.available() > 0) {
      // read the incoming byte:
      incomingByte = Serial.read();
      DEBUG(incomingByte);
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
        DEBUG("annex lights: " );
        DEBUGLN(annexLightsOn);
        digitalWrite(annexLightPin, annexLightsOn);
      } else if (incomingByte == 'l') {
        if (shedLightsOn) {
          shedLightsOn = false;
        } else {
          shedLightsOn = true;
        }
        DEBUG("shed lights: " );
        DEBUGLN(shedLightsOn);
        digitalWrite(shedLightPin, shedLightsOn);
      } else if (incomingByte == 's') {
         if (sensor)
           sensor = 0;
         else 
            sensor = 1;
		 DEBUGLN(sensor);
         DEBUGLN(LocoNet.reportSensor(1, sensor));
      }
  }
#endif
  pins_busy = false;
  for (uint8_t i =0 ; i < MAX ; i++) {
	  pins_busy |= confpins[i]->update();
  };
  if ((leftDoor.doorState() > 1) && (rightDoor.doorState() > 1) && !pins_busy)
	  disableServos();
  /*** PUSH BUTTON ***/
//  delay(1);
  doors.update();
  if (bouncer.update()) {
    if (bouncer.rose()) {
	  LocoNet.reportSensor(sensorAddress, 1);
	  DEBUGLN(1);
    } else if (bouncer.fell()) {
	  LocoNet.reportSensor(sensorAddress, 0);
	  DEBUGLN(0);	  
    }
  }
  /*** LOCONET ***/
  LnPacket = LocoNet.receive();
  if (LnPacket) {
    uint8_t packetConsumed(LocoNet.processSwitchSensorMessage(LnPacket));
    if (packetConsumed == 0) {
      DEBUG("Loop ");
      DEBUG((int)LnPacket);
      dumpPacket(LnPacket->ub);
      packetConsumed = lnCV.processLNCVMessage(LnPacket);
      DEBUG("End Loop\n");
    }
  }
};

void notifySwitchRequest( uint16_t Address, uint8_t Output, uint8_t Direction ) {
  DEBUG("Switch Request: ");
  DEBUG(Address);
  DEBUG(':');
  DEBUG(Direction ? "Closed" : "Thrown");
  DEBUG(" - ");
  DEBUGLN(Output ? "On" : "Off");
  if (Address == shedLightAddress) {
	  if (Direction) {
          digitalWrite(shedLightPin, HIGH);
	  } else {
		  digitalWrite(shedLightPin, LOW);
	  }
  } else if (Address == annexLightAddress) {
	  if (Direction) {
          digitalWrite(annexLightPin, HIGH);
	  } else {
		  digitalWrite(annexLightPin, LOW);
	  }
	  //LocoNet.reportSwitch(annexLightAddress);
  } else if (Address == doorAddress) {
	  if (Direction) {
		  doors.close();
		  eeprom_write_word(DoorStateAddress, 1);
	  } else {
		  doors.open();
		  eeprom_write_word(DoorStateAddress, 0);
	  }
  }
  for (uint8_t i =0 ; i < MAX ; i++) {
    if (confpins[i]->_address == Address){
      confpins[i]->set(Direction, Output);
      eeprom_write_word((uint16_t*)&(_CV.conf[i].servo.state), confpins[i]->get_state());
      DEBUG("Thrown switch: ");
      DEBUG(i);
      DEBUG(" to :");
      confpins[i]->print_state();
      DEBUG("\n");
    }
  }
  
}

void toggleDoors() {
  if (doors.state() == Door::DOOROPEN) {
    doors.close();
  } else if (doors.state() == Door::DOORCLOSED) {
    doors.open();
  }
}

void enableServos() {
	digitalWrite(servoEnablePin, HIGH);
}

void disableServos() {
	digitalWrite(servoEnablePin, LOW);
}

void dumpPacket(UhlenbrockMsg & ub) {
#ifdef DEBUG_OUTPUT
  Serial.print(" PKT: ");
  Serial.print(ub.command);
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
#endif
}

/**
   * Notifies the code on the reception of a read request.
   * Note that without application knowledge (i.e., art.-nr., module address
   * and "Programming Mode" state), it is not possible to distinguish
   * a read request from a programming start request message.
   */
int8_t notifyLNCVread(uint16_t ArtNr, uint16_t lncvAddress, uint16_t,
    uint16_t & lncvValue) {

  DEBUG("Enter notifyLNCVread(");
  DEBUG(ArtNr);
  DEBUG(", ");
  DEBUG(lncvAddress);
  DEBUG(", ");
  DEBUG(", ");
  DEBUG(lncvValue);
  DEBUG(")\n");
  
  // Step 1: Can this be addressed to me?
  // All ReadRequests contain the ARTNR. For starting programming, we do not accept the broadcast address.
  if (programmingMode) {
    if (ArtNr == ARTNR) {
      if (lncvAddress < 255) {
        lncvValue = read_cv(&_CV, lncvAddress);
        
        DEBUG("\nEeprom address: ");
        DEBUG(((uint16_t)&(_CV.address)+cv2address(lncvAddress)));
        DEBUG(" LNCV Value: ");
        DEBUG(lncvValue);
        DEBUG("\n");
        
        return LNCV_LACK_OK;
      } else {
        // Invalid LNCV address, request a NAXK
        return LNCV_LACK_ERROR_UNSUPPORTED;
      }
    } else {
      
      DEBUG("ArtNr invalid.\n");
      
      return -1;
    }
  } else {
    
    DEBUG("Ignoring Request.\n");
    
    return -1;
  }
}

int8_t notifyLNCVprogrammingStart(uint16_t & ArtNr, uint16_t & ModuleAddress) {
  // Enter programming mode. If we already are in programming mode,
  // we simply send a response and nothing else happens.
  uint16_t MyModuleAddress = eeprom_read_byte(&_CV.address);

  DEBUG(ArtNr);
  DEBUG(ModuleAddress);
  DEBUG(MyModuleAddress);


  if (ArtNr == ARTNR) {
    if (ModuleAddress == MyModuleAddress) {
      programmingMode = true;

      DEBUG("Programming started");

      return LNCV_LACK_OK;
    } else if (ModuleAddress == 0xFFFF) {
      ModuleAddress = eeprom_read_byte(&_CV.address);
      return LNCV_LACK_OK;
    }
  }
  return -1;
}

  /**
   * Notifies the code on the reception of a write request
   */
int8_t notifyLNCVwrite(uint16_t ArtNr, uint16_t lncvAddress,
    uint16_t lncvValue) {
  //  dumpPacket(ub);
  if (!programmingMode) {
    return -1;
  }

  DEBUG("Enter notifyLNCVwrite(");
  DEBUG(ArtNr);
  DEBUG(", ");
  DEBUG(lncvAddress);
  DEBUG(", ");
  DEBUG(", ");
  DEBUG(lncvValue);
  DEBUG(")\n");

  if (ArtNr == ARTNR) {

    if (lncvAddress < 255) {
      DEBUG(cv2address(lncvAddress));
      DEBUG(bytesizeCV(lncvAddress));
      DEBUG((uint8_t)lncvValue);
      write_cv(&_CV, lncvAddress, lncvValue);      
      DEBUG(read_cv(&_CV, lncvAddress));
      //lncv[lncvAddress] = lncvValue;
      return LNCV_LACK_OK;
    }
    else {
      return LNCV_LACK_ERROR_UNSUPPORTED;
    }

  }
  else {
    
    DEBUG("Artnr Invalid.\n");
    
    return -1;
  }
}

void commitLNCVUpdate() {
	 // Reset the decoder to reread the configuration
	asm volatile ("  jmp 0");
}
  /**
   * Notifies the code on the reception of a request to end programming mode
   */
void notifyLNCVprogrammingStop(uint16_t ArtNr, uint16_t ModuleAddress) {
  
  DEBUG("notifyLNCVprogrammingStop ");
      
  if (programmingMode) {
    if (ArtNr == ARTNR && ModuleAddress == eeprom_read_byte(&_CV.address)) {
      programmingMode = false;
      
      DEBUG("End Programing Mode.\n");
          

      commitLNCVUpdate();
    }
    else {
      if (ArtNr != ARTNR) {
        
        DEBUG("Wrong Artnr.\n");
            

        return;
      }
      if (ModuleAddress != eeprom_read_byte(&_CV.address)) {
        
        DEBUG("Wrong Module Address.\n");
            

        return;
      }
    }
  }
  else {
    
    DEBUG("Ignoring Request.\n");
        

  }
}

int8_t notifyLNCVdiscover( uint16_t & ArtNr, uint16_t & ModuleAddress ) {
  DEBUG("Discover: ");
  DEBUG(ArtNr);
  DEBUG(ARTNR);
//    if (ArtNr == ARTNR ) {
      uint16_t MyModuleAddress = eeprom_read_byte(&_CV.address);
      ModuleAddress = MyModuleAddress;
    DEBUG(ModuleAddress);
      return LNCV_LACK_OK;
 //   }
    

}
