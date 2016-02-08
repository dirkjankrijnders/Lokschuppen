/* 
   Door.cpp - Library for a servo controlled door
   Created by Dirkjan KRijnders, April 11th, 2015
   Released into the public domain
*/

#include "Arduino.h"
#include "Servo.h"
#include "EEPROM.h"

#include "Door.h"

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