#include <HX711.h>
#include <LiquidCrystal.h>
#include <Servo.h>


// Config
#define TARGET_WEIGHT 250
#define SLOW_WEIGHT   80  // The vibration slows down from this amount of grams remaining
#define PULSE_WEIGHT  10  // The vibration will start pulsing with this amount of grams remaining
#define PULSE_LENGTH  1   // How long is each pulse? 1 - 10 (10 = continuous)

#define SERVO_DISPENSER_OPEN   40
#define SERVO_DISPENSER_CLOSED 70


// Pins
#define SCALE_DOUT  9
#define SCALE_CLK   8

#define LCD_RS A2
#define LCD_E  A1
#define LCD_D7 7
#define LCD_D6 6
#define LCD_D5 5
#define LCD_D4 4

#define BUTTONS 0

#define PIN_MOTOR 3
#define PIN_DISPENSER 11

#define PIN_DISPENSE_BUTTON 2

// States
#define STATE_IDLE     0
#define STATE_FILLING  1
#define STATE_FULL     2
#define STATE_EMPTYING 3
#define STATE_MENU     4
#define STATE_MENU_2   5

#define MOTOR_STATE_OFF 0
#define MOTOR_STATE_ON  0


// Button IDs
#define BTN_NONE  0
#define BTN_RIGHT 1
#define BTN_LEFT  2
#define BTN_UP    3
#define BTN_DOWN  4
#define BTN_SEL   5

// Internal Config
#define WEIGHTS_SIZE 10
#define CHAR_PLAY  1
#define CHAR_PAUSE 2
#define CHAR_DOWN  3

#define MENU_SIZE    5
const char * menu[MENU_SIZE] = {
  "Fill 250 g",
  "Fill 500 g",
  "Fill 1.2 kg",
  "Tare",
  "Calibrate"
};

HX711 scale;
LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
Servo dispenser;

float calibration_weight = 250.0f;
float calibration_factor = -86.69f;
long target_weight = (TARGET_WEIGHT) * 10;
uint8_t state = STATE_MENU;
char str[16];
long weights[WEIGHTS_SIZE];
volatile bool dispensePressed = false;

void buttonPress(void);

byte smiley[8] = {
  B00000,
  B10001,
  B00000,
  B00100,
  B00000,
  B10001,
  B01110,
};

byte char_play[8] = {
  B00000,
  B10000,
  B11000,
  B11100,
  B11110,
  B11100,
  B11000,
  B10000,
};
byte char_pause[8] = {
  B00000,
  B00000,
  B11011,
  B11011,
  B11011,
  B11011,
  B11011,
  B11011,
};
byte char_down[8] = {
  B00000,
  B00000,
  B11111,
  B01110,
  B01110,
  B00100,
  B00100,
  B00000,
};

void setup() {

  Serial.begin(9600);
  scale.begin(SCALE_DOUT, SCALE_CLK);
  lcd.begin(16,2);
  dispenser.attach(PIN_DISPENSER);
  dispenser.write(SERVO_DISPENSER_CLOSED);

  digitalWrite(PIN_MOTOR, LOW);
  pinMode(PIN_MOTOR, OUTPUT);

  pinMode(PIN_DISPENSE_BUTTON, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_DISPENSE_BUTTON), buttonPress, RISING);

  lcd.createChar(0, smiley);
  lcd.createChar(CHAR_PLAY, char_play);
  lcd.createChar(CHAR_PAUSE, char_pause);
  lcd.createChar(CHAR_DOWN, char_down);
  lcd.setCursor(0,0);
  lcd.print("Taring...");
  lcd.setCursor(0,1);
  lcd.write(byte(0));

  // Need to wait a little for the taring to work well
  delay(1000);
  scale.set_scale(calibration_factor);
  Serial.println("Taring...");
  scale.tare(5);

  dispenser.detach();
}

void loop() {
  if (state == STATE_MENU || state == STATE_MENU_2){
    handleMenu();
  }
  else {
    handleFilling();
  }
}

void handleMenu(void){

  static uint8_t menuIdx;

  if (state == STATE_MENU){
    menuIdx = 0;
    drawMenu(menuIdx);
    state = STATE_MENU_2;
  }
  
  char btn = getButtonRelease();
  if (btn == BTN_DOWN){
    menuIdx++;
    menuIdx %= MENU_SIZE;
    drawMenu(menuIdx);
  }
  else if (btn == BTN_UP){
    if (menuIdx == 0){
      menuIdx = MENU_SIZE;
    }
    menuIdx--;
    drawMenu(menuIdx);
  }
  else if (btn == BTN_SEL){
    switch (menuIdx)
    {
    case 0:
      target_weight = 2500;
      state = STATE_IDLE;
      break;
    case 1:
      target_weight = 5000;
      state = STATE_IDLE;
      break;
    case 2:
      target_weight = 12000;
      state = STATE_IDLE;
      break;
    case 3:
      tareCalibrate(false);
      state = STATE_MENU;
      break;
    case 4:
      tareCalibrate(true);
      state = STATE_MENU;
      break;
    default:
      break;
    }
    lcd.clear();
  }
}

void handleFilling(void){

  static long weightSum = 0;
  static uint8_t weightsIndex = 0;
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
    else if (diff < PULSE_WEIGHT * 10){
      // Pulse motor
      if (weightsIndex < (PULSE_LENGTH)){
        digitalWrite(PIN_MOTOR, HIGH);
      }
      else {
        digitalWrite(PIN_MOTOR, LOW);
      }
    }
    else if (diff < SLOW_WEIGHT * 10){
      int power = map(diff, PULSE_WEIGHT * 10, SLOW_WEIGHT * 10, 160, 255);
      // Serial.print(diff);
      // Serial.print(" -> ");
      // Serial.println(power);
      analogWrite(PIN_MOTOR, power);
    }
    else {
      digitalWrite(PIN_MOTOR, HIGH);
    }
  }
  else {
    digitalWrite(PIN_MOTOR, LOW);
  }

  // Open 5 ms per gram
  if (millis() - dispenserOpenTime > (target_weight * 5 / 10)){
    if (dispenser.attached()){
      dispenser.write(SERVO_DISPENSER_CLOSED);
      delay(100);
      dispenser.detach();
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
    sprintf(str, "\x7E%04d.0 g", target_weight/10);
    lcd.print(str);

    lcd.setCursor(14, 1);
    if (dispensePressed){
      lcd.write(byte(CHAR_DOWN));
    }
    else {
      lcd.print(" ");
    }
    if (state == STATE_FILLING){
      lcd.write(byte(CHAR_PLAY));
    }
    else {
      lcd.write(byte(CHAR_PAUSE));
    }
    // lcd.print(state);
  }

  char btn = getButtonRelease();
  if (btn == BTN_SEL){
    dispensePressed = false;
    if (state == STATE_IDLE){
      state = STATE_FILLING;
    }
    else {
      state = STATE_IDLE;
    }
  }
  else if (btn == BTN_RIGHT){
    digitalWrite(PIN_MOTOR, LOW);
    state = STATE_MENU;
  }

  if (state == STATE_FULL && dispensePressed){
    dispensePressed = false;
    Serial.println("Dispenser Open");
    dispenser.attach(PIN_DISPENSER);
    dispenser.write(SERVO_DISPENSER_OPEN);
    dispenserOpenTime = millis();

    Serial.println(millis());
  }
}

void drawMenu(uint8_t menuIdx){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("\x7E");
  lcd.print(menu[menuIdx]);
  lcd.setCursor(1, 1);
  lcd.print(menu[(menuIdx + 1) % MENU_SIZE]);
}

bool tareCalibrate(bool calibrate){
  
  lcd.clear();
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
    calibration_factor = (m - scale.get_offset()) / calibration_weight / 10;
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

void buttonPress(void){
  dispensePressed = true;
}