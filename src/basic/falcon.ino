/*
* Simple laser handler code. This is not required for the laser to function but it is required
* for the laser sensors to activate and report their status correctly. 
* Tested with a 40W and 12W laser module
*/
#include <HardwareSerial.h>

#include "list.hpp";
#include "timer.hpp";
#include "laser_communicator.hpp";
#define BAUD_RATE 9600

HardwareSerial SerialPort(2);

LaserCommunicator laser(&SerialPort);

void setup() {
  // serial to usb
  Serial.begin(BAUD_RATE); 
  // serial to laser
  SerialPort.begin(BAUD_RATE); 

  laser.setup();

  Serial.println("launched..");
}

void loop() {
  laser.loop();
}
