#include <Servo.h>


// Config
#define TIME_CLOSE 4300
#define TIME_CLOSE_HEAT_ON (TIME_CLOSE - 1000)
#define TIME_SEAL  2600
#define TIME_OPEN  4000

#define SERVO_EJECT_OPEN   10
#define SERVO_EJECT_CLOSED 90

// Pins
#define PIN_START_BUTTON 2
#define PIN_MOTOR_OUT    3
#define PIN_MOTOR_IN     4
#define PIN_HEATER       5
#define PIN_SERVO_EJECT  11

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
  Serial.println("Boot");
  ejecter.attach(PIN_SERVO_EJECT);
  ejecter.write(SERVO_EJECT_CLOSED);
  
  digitalWrite(PIN_MOTOR_IN, HIGH);
  digitalWrite(PIN_MOTOR_OUT, HIGH);
  digitalWrite(PIN_HEATER, LOW);

  pinMode(PIN_MOTOR_IN, OUTPUT);
  pinMode(PIN_MOTOR_OUT, OUTPUT);
  pinMode(PIN_HEATER, OUTPUT);

  pinMode(PIN_START_BUTTON, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_START_BUTTON), buttonPress, RISING);

  delay(1000);
  ejecter.detach();
}

void loop() {
  static long lastEvent;
  switch (state)
  {
  case STATE_IDLE:
    if (buttonPressed){
      Serial.println("Closing");
      state = STATE_CLOSING;
      lastEvent = millis();
      digitalWrite(PIN_MOTOR_IN, LOW);
    }
    break;

  case STATE_CLOSING:
    buttonPressed = false;
    if (millis() - lastEvent > TIME_CLOSE_HEAT_ON){
      digitalWrite(PIN_HEATER, HIGH);
    }
    if (millis() - lastEvent > TIME_CLOSE){
      Serial.println("Sealing");
      state = STATE_SEALING;
      digitalWrite(PIN_MOTOR_IN, HIGH);
      lastEvent = millis();
    }
    break;

  case STATE_SEALING:
    if (millis() - lastEvent > TIME_SEAL){
      Serial.println("Opening");
      state = STATE_OPENING;
      digitalWrite(PIN_MOTOR_OUT, LOW);
      digitalWrite(PIN_HEATER, LOW);
      lastEvent = millis();
    }
    break;
    
  case STATE_OPENING:
    if (millis() - lastEvent > TIME_OPEN){
      Serial.println("Eject");
      state = STATE_EJECT;
      digitalWrite(PIN_MOTOR_OUT, HIGH);
      lastEvent = millis();
    }
    break;

  case STATE_EJECT:
    ejecter.attach(PIN_SERVO_EJECT);
    ejecter.write(SERVO_EJECT_OPEN);
    delay(700);
    ejecter.write(SERVO_EJECT_CLOSED);
    delay(900);
    ejecter.detach();
    state = STATE_IDLE;
    break;
    
  }
}

void buttonPress(void){
  buttonPressed = true;
}