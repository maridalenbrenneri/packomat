#include <Servo.h>


// Config
#define TIME_CLOSE 1100
#define TIME_SEAL  2000
#define TIME_OPEN  1000

#define SERVO_EJECT_OPEN   0
#define SERVO_EJECT_CLOSED 0

// Pins
#define PIN_START_BUTTON 2
#define PIN_MOTOR        3
#define PIN_MOTOR_DIR    4
#define PIN_SERVO_EJECT  5

// States
#define STATE_IDLE     0
#define STATE_CLOSING  1
#define STATE_SEALING  2
#define STATE_OPENING  3
#define STATE_EJECT    4
#define STATE_CANCEL   5

Servo ejecter;

uint8_t state = STATE_IDLE;
char str[16];
volatile bool buttonPressed = false;

void buttonPress(void);

void setup() {

  Serial.begin(9600);
  ejecter.attach(PIN_SERVO_EJECT);
  ejecter.write(SERVO_EJECT_CLOSED);

  digitalWrite(PIN_MOTOR, LOW);
  pinMode(PIN_MOTOR, OUTPUT);

  pinMode(PIN_START_BUTTON, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_START_BUTTON), buttonPress, RISING);

  ejecter.detach();
}

void loop() {
  if {
    handleFilling();
  }
}

void handleFilling(void){

}

void buttonPress(void){
  buttonPressed = true;
}