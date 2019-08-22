#include <HX711.h>
#include <LiquidCrystal.h>
#include <Servo.h>


// Pins
#define DOUT  3
#define CLK   2

#define LCD_RS 8
#define LCD_E  9
#define LCD_D7 7
#define LCD_D6 6
#define LCD_D5 5
#define LCD_D4 4

#define BUTTONS 0

#define PIN_MOTOR 10
#define PIN_DISPENSER 11



// Config
#define WEIGHTS_SIZE 10
#define SLOW_WEIGHT 50
#define PROPORTIONAL_FACTOR 5

#define SERVO_DISPENSER_OPEN   60
#define SERVO_DISPENSER_CLOSED 90

// States
#define STATE_IDLE 0
#define STATE_FILLING 1
#define STATE_FULL 2
#define STATE_EMPTYING 3

#define MOTOR_STATE_OFF 0
#define MOTOR_STATE_ON  0


// Button IDs
#define BTN_NONE  0
#define BTN_RIGHT 1
#define BTN_LEFT  2
#define BTN_UP    3
#define BTN_DOWN  4
#define BTN_SEL   5

HX711 scale;
LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
Servo dispenser;

float calibration_weigth = 250.0;
float calibration_factor = -86.42;
long target_weight = 2500;
static char str[16];
static long weights[WEIGHTS_SIZE];

byte smiley[8] = {
  B00000,
  B10001,
  B00000,
  B00100,
  B00000,
  B10001,
  B01110,
};

void setup() {

  Serial.begin(9600);
  scale.begin(DOUT, CLK);
  lcd.begin(16,2);
  dispenser.attach(PIN_DISPENSER);
  dispenser.write(SERVO_DISPENSER_CLOSED);

  digitalWrite(PIN_MOTOR, LOW);
  pinMode(PIN_MOTOR, OUTPUT);

  lcd.createChar(0, smiley);
  lcd.setCursor(0,0);
  lcd.print("Taring...");
  lcd.setCursor(0,1);
  lcd.write(byte(0));

  // Need to wait a little for the taring to work well
  delay(1000);
  scale.set_scale(calibration_factor);
  Serial.println("Taring...");
  scale.tare(5);
}

void loop() {

  static long weightSum = 0;
  static uint8_t weightsIndex = 0;
  static uint8_t state = STATE_IDLE;
  static uint8_t motorState = MOTOR_STATE_OFF;
  static unsigned long dispenserOpenTime = 0;
  long weight;
  long avgWeight;
  long oldWeight;
  long rate;

  weight = scale.get_units(1);
  oldWeight = weights[weightsIndex];
  weightSum += weight;
  weightSum -= oldWeight;
  weights[weightsIndex] = weight;
  rate = (weight - oldWeight) / WEIGHTS_SIZE;

  ++weightsIndex;
  if (weightsIndex >= WEIGHTS_SIZE){
    weightsIndex = 0;
  }
 
  avgWeight = weightSum / WEIGHTS_SIZE;

  if (state == STATE_FILLING){

    int diff = target_weight - weight;

    if (diff <= 0) {

      if (avgWeight > target_weight){
        state = STATE_FULL;
      }
      digitalWrite(PIN_MOTOR, LOW);
    }
    else if (diff < SLOW_WEIGHT * 10){
      // Serial.print("Slow: ");
      // Serial.print(diff);
      // Serial.print(" ");
      // Serial.print(weightsIndex);
      // Serial.print(" ");
      // Serial.println(max(diff / 50, 1));
      if (weightsIndex < max(diff / 50, 1)){
        digitalWrite(PIN_MOTOR, HIGH);
      }
      else {
        digitalWrite(PIN_MOTOR, LOW);
      }
    }
    else {
      digitalWrite(PIN_MOTOR, HIGH);
    }
  }
  else {
    digitalWrite(PIN_MOTOR, LOW);
  }

  if (millis() - dispenserOpenTime > 1500UL){
    if (dispenser.read() == SERVO_DISPENSER_OPEN){
      dispenser.write(SERVO_DISPENSER_CLOSED);
      delay(100);
      state = STATE_FILLING;
    }
  }

  bool negative = (avgWeight < 0);
  if (negative){
    avgWeight = -avgWeight;
  }
  long dec = avgWeight % 10;
  avgWeight /= 10;
  if (weightsIndex == 0){
    if(negative){
      Serial.print("-");
    }
    Serial.print(avgWeight, 10);
    Serial.print(".");
    Serial.println(dec, 10);
  }

  // dtostrf(weight, 6, 2, str);
  if ((weightsIndex % 2) == 0){
    lcd.setCursor(0, 0);
    if(negative){
      lcd.write('-');
    }
    else {
      lcd.write(' ');
    }
    sprintf(str, "%04d.", avgWeight);
    lcd.print(str);
    sprintf(str, "%1d g", dec);
    lcd.print(str);

    lcd.setCursor(0, 1);
    lcd.print(rate / 10.0f);

    lcd.setCursor(15, 1);
    lcd.print(state);
  }

  char btn = getButtonRelease();
  if (btn == BTN_RIGHT){
    tareCalibrate(btn == BTN_SEL);
  }
  else if (btn == BTN_SEL){
    digitalWrite(PIN_MOTOR, HIGH);
  }
  else if (btn == BTN_LEFT){
    if (state == STATE_IDLE){
      state = STATE_FILLING;
    }
    else {
      state = STATE_IDLE;
    }
  }
  else if (btn == BTN_DOWN){
    Serial.println("Dispenser Open");
    dispenser.write(SERVO_DISPENSER_OPEN);
    dispenserOpenTime = millis();

    Serial.println(millis());
  }
}


bool tareCalibrate(bool calibrate){

  lcd.setCursor(0,0);
  lcd.print("Tare...   ");
  lcd.blink();

  scale.tare(20);
  
  lcd.noBlink();
  lcd.clear();
  
  Serial.print("Tare offset: ");
  Serial.println(scale.get_offset());

  if (calibrate){
    lcd.setCursor(0,0);
    lcd.print("Add CAL bag!");
    lcd.setCursor(0,1);
    lcd.print("And press SEL...");
    while (getButton() == BTN_SEL);
    while (getButton() != BTN_SEL);
    lcd.clear();

    float m = scale.read_average(20);
    Serial.print("Meas: ");
    Serial.println(m, 2);
    calibration_factor = (m - scale.get_offset()) / calibration_weigth / 10;
    scale.set_scale(calibration_factor);
    Serial.print("Calibration: ");
    Serial.println(calibration_factor, 10);
  }
}

char getButtonRelease(){
  char btn = getButton();
  while (btn != BTN_NONE && btn == getButton());

  return btn;
}

char getButton(){
  int x;
  x = analogRead(BUTTONS);
  if (x < 100) {
    return BTN_RIGHT; 
  }
  else if (x < 200) {
    return BTN_UP;
  }
  else if (x < 400){
    return BTN_DOWN; 
  }
  else if (x < 600){
    return BTN_LEFT;
  }
  else if (x < 800){
    return BTN_SEL;
  }
  else {
    return BTN_NONE;
  }
}