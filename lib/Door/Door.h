/* 
   Door.h - Library for a servo controlled door
   Created by Dirkjan KRijnders, April 11th, 2015
   Released into the public domain
*/

#ifndef Door_h
#define Door_h

#include "Arduino.h"

#include "Servo.h"

class Door {
  private:
    Servo* _servo;
    unsigned int _degreesOpen;
    unsigned int _degreesClosed;
    int _speed;
		unsigned int _slowCounter;
		unsigned int _slowDown;
		
    unsigned int _currentDegrees;
    unsigned int _targetDegrees;
		
		unsigned int _delayCounter;
		unsigned int _delayCounter2;
  public:
    Door(Servo* servo, unsigned int degreesOpen, unsigned degreesClosed, unsigned int speed = 1, unsigned int slowDown = 0);
    void open();
    void close();
    void set(unsigned int degrees);
    
    void update();
		void setDelay(unsigned int delay);
		unsigned int getDelay();
		unsigned int doorState();
		
		enum state {
			DOORERROR = 0,
			DOORMOVING,
			DOOROPEN,
			DOORCLOSED
		};
};

#endif
