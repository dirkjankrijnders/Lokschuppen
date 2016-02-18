Lokschuppen
===========
Master: [![Build Status](https://travis-ci.org/dirkjankrijnders/Lokschuppen.svg?branch=master)](https://travis-ci.org/dirkjankrijnders/Lokschuppen) Develop: [![Build Status](https://travis-ci.org/dirkjankrijnders/Lokschuppen.svg?branch=develop)](https://travis-ci.org/dirkjankrijnders/Lokschuppen)

The locomotive shed will have the following sensors:

- IR gate (Sharp IS471F), for checking whether the doors can be closed.
- Distance sensor (Vishay VCNL4000), the gauge the remaining distance the loco can still move into the shed

and the following actuators:

- Light in the shed
- Light in the extension
- Left door
- Right door

The building and decoder are connected using a 10 pin (2x5) IDC Connector at 2.54mm pitch.

- GND
- +5V
- Vo door
- Servo left
- Servo right
- LED shed
- LED office
- SDA distance sensor
- SCL distance
