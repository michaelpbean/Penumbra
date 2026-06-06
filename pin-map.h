
// Enable if using Penumbra board
#define PENUMBRA_BOARD

#ifdef PENUMBRA_BOARD

//#define USE_USB

#define DIN1_PIN                34
#define DIN2_PIN                35

#define DOUT1_PIN               14
#define DOUT2_PIN               13

#define RXD1_PIN                33 // Labeled as 2 on PCB
#define TXD1_PIN                25
#define RXD2_PIN                16 // Labeled as 3 on PCB
#define TXD2_PIN                17
#define RXD3_PIN                32 // Labeled as 4 on PCB
#define TXD3_PIN                4

#define OUTPUT_ENABLE_PIN       27
#define RS485_RTS_PIN           26

///////////////////////

// ------------------------------
// Assign pins for Penumbra Board

#define SERIAL1_RX_PIN RXD1_PIN    // Set to your board's TX pin for Serial1.
#define SERIAL1_TX_PIN TXD1_PIN    // Set to your board's RX pin for Serial1.

#define SERIAL2_RX_PIN RXD3_PIN   // Set to your board's TX pin for Serial2. 
#define SERIAL2_TX_PIN TXD3_PIN   // Set to your board's RX pin for Serial2.

// Set the following only if you use PWM for your drive system.
// You may ignore this if you do not use PWM for your drive system.
#define LEFT_MOTOR_PWM      TXD1_PIN  // Set to a PWM pin for left foot motor.
#define RIGHT_MOTOR_PWM     TXD2_PIN  // Set to a PWM pin for right foot motor.
// MPB - Using Neo motors, no throttle PWM pin.
//#define THROTTLE_MOTOR_PWM  SDA  // Optional Roboteq pin used for MicroBasic scripts running on the Roboteq controller to change the throttle.
                                    // If the Microbasic script is not runnig this PWM signal will have no effect.

// Set the following only if you use PWM for your dome system.
// You may ignore this if you do not use PWM for your dome system.
// MPB - Using Syren for dome drive so no PWM pin is needed.
//#define DOME_MOTOR_PWM      DOUT1_PIN  // Set to a PWM pin used to control the dome motor

#define SOUND_SERIAL_TX         DOUT1_PIN
#define SOUND_SERIAL_RX         DIN2_PIN
#define SOUND_SERIAL            soundSerial
#define SOUND_SERIAL_INIT(baud) { SOUND_SERIAL.begin(baud, SWSERIAL_8N1, SOUND_SERIAL_RX, SOUND_SERIAL_TX, false); delay(1500); }
#define SOUND_SERIAL_BAUD       9600

#include "SoftwareSerial.h"

#define TANK_DRIVE_ID       128
#define DOME_DRIVE_ID       129

// ------------------------------------------------------------------------------------------

#else /* !PENUMBRA_BOARD */

// ------------------------------
// Assign pins for Generic ESP32

#define SERIAL1_RX_PIN 4    // Set to your board's TX pin for Serial1.
#define SERIAL1_TX_PIN 2    // Set to your board's RX pin for Serial1.

#define SERIAL2_RX_PIN 18   // Set to your board's TX pin for Serial2. 
#define SERIAL2_TX_PIN 19   // Set to your board's RX pin for Serial2.

// Set the following only if you use PWM for your drive system.
// You may ignore this if you do not use PWM for your drive system.

#define LEFT_MOTOR_PWM      23  // Set to a PWM pin for left foot motor.
#define RIGHT_MOTOR_PWM     22  // Set to a PWM pin for right foot motor.
#define THROTTLE_MOTOR_PWM  21  // Optional Roboteq pin used for MicroBasic scripts running on the Roboteq controller to change the throttle.
                                // If the Microbasic script is not runnig this PWM signal will have no effect.

// Set the following only if you use PWM for your dome system.
// You may ignore this if you do not use PWM for your dome system.

#define DOME_MOTOR_PWM      27  // Set to a PWM pin used to control the dome motor

// Uncomment the following to enable use of Software Serial for Marcduino

//#include "SoftwareSerial.h"
//#define SERIAL_MARCDUINO_TX_PIN 5 // Set to the pin you use for Software Serial

#define SOUND_SERIAL_TX         5
#define SOUND_SERIAL_RX         18
#define SOUND_SERIAL            soundSerial
#define SOUND_SERIAL_INIT(baud) { SOUND_SERIAL.begin(baud, SWSERIAL_8N1, SOUND_SERIAL_RX, SOUND_SERIAL_TX, false); delay(1500); }
#define SOUND_SERIAL_BAUD       9600

#endif
