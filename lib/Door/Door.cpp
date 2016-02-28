/* 
   Door.cpp - Library for a servo controlled door
   Created by Dirkjan KRijnders, April 11th, 2015
   Released into the public domain
*/

#include "Arduino.h"
#include "Servo.h"
#include "EEPROM.h"

#include "Door.h"

extern void reportSwitch(uint16_t address, uint16_t state);
extern void enableServos();

Door::Door(Servo* servo, unsigned int degreesOpen, unsigned degreesClosed, unsigned int speed, unsigned int slowDown) {
  _servo = servo;
  _degreesOpen  = degreesOpen;
  _degreesClosed = degreesClosed;
  _speed = speed;
  _slowDown = slowDown;
	
  _currentDegrees = EEPROM.read(1000);
  _targetDegrees = _currentDegrees;

  _servo->write(_currentDegrees);
	_slowCounter = 0;
	
	_delayCounter = 0;
}

void Door::set(unsigned int degrees) {
  _targetDegrees = degrees;
  EEPROM.write(1000, degrees);
}

void Door::open() {
  _targetDegrees = _degreesOpen;
  EEPROM.write(1000, _degreesOpen);
}
void Door::close() {
  _targetDegrees = _degreesClosed;
  EEPROM.write(1000, _degreesClosed);
}

void Door::setDelay(unsigned int delay) {
	_delayCounter = delay;
}

unsigned int Door::getDelay() {
	return _delayCounter;
}

void Door::update() {
	if (_delayCounter > 0){
		if (_delayCounter2 > 0) {
			_delayCounter2--;
		} else {
			_delayCounter--;
			_delayCounter2 = 255;
		};
//		delay(10);
		return;
	};
	if (_slowCounter > _slowDown)
	 {
		if (_targetDegrees < _currentDegrees) {
   	 	_currentDegrees -= _speed;
 	 	} else if (_targetDegrees > _currentDegrees) {
      _currentDegrees += _speed;
    }
		_servo->write(_currentDegrees);
		_slowCounter = 0;
	} else {
		_slowCounter++;
	}
}

unsigned int Door::doorState() {
	if (_currentDegrees == _targetDegrees) {
		if (_currentDegrees == _degreesOpen) {
			return DOOROPEN;
		} else {
			return DOORCLOSED;
		}
	} else {
		return DOORMOVING;
	}
	return DOORERROR;
}

DoubleDoor::DoubleDoor(Door* door1, Door* door2, int delay, uint16_t address) {
	_doors[0] = door1;
	_doors[1] = door2;
	_delay = delay;
	_address = address;
	
	_prev_state = Door::DOORERROR;
}
	
void DoubleDoor::open(){
    reportSwitch(_address, 0);
    enableServos();
	
    _doors[0]->setDelay(0);
    _doors[0]->open();
    _doors[1]->setDelay(_delay);
    _doors[1]->open();
}
void DoubleDoor::close(){
	reportSwitch(_address, 0);
	enableServos();
	_doors[1]->setDelay(0);
	_doors[1]->close();
	_doors[0]->setDelay(_delay);
	_doors[0]->close();
}

void DoubleDoor::update(){
	_doors[0]->update();
	_doors[1]->update();
	int _state = state();
//	Serial.println(_state);
	if (_state != _prev_state){
		Serial.print("state change");
		Serial.println(_prev_state);
		_prev_state = _state;
		if (_state == Door::DOOROPEN){
			reportSwitch(_address, 1);
		} else if (_state == Door::DOORCLOSED) {
			reportSwitch(_address, 2);
		}
	}
}

int DoubleDoor::state() {
	if (_doors[0]->doorState() == _doors[1]->doorState()) {
		return _doors[0]->doorState();
	} else {
		return Door::DOORMOVING;
	}
}