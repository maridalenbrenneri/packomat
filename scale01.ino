#include <HX711.h>
#include <LiquidCrystal.h>
#include <Servo.h>

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

HX711 scale;
LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

float calibration_weigth = 250.0;
float calibration_factor = -86.42;
long target_weight = 250;
static char str[16];

#define BTN_NONE  0
#define BTN_RIGHT 1
#define BTN_LEFT  2
#define BTN_UP    3
#define BTN_DOWN  4
#define BTN_SEL   5

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

  digitalWrite(PIN_MOTOR, LOW);
  pinMode(PIN_MOTOR, OUTPUT);

  lcd.createChar(0, smiley);
  lcd.setCursor(0,0);
  lcd.print("Taring...");
  lcd.setCursor(0,1);
  lcd.write(byte(0));

  // Need to wait a little for the taring to work well
  delay(500);
  scale.set_scale(calibration_factor);
  Serial.println("Taring...");
  scale.tare(5);
}

#define WEIGHTS_SIZE 10
#define STOP_ITERATIONS 8
#define STOP_WEIGHT 75

#define STATE_IDLE 0
#define STATE_FILLING 1
#define STATE_FULL 2
#define STATE_EMPTYING 3

void loop() {

  static long weights[WEIGHTS_SIZE];
  static long weightSum = 0;
  static uint8_t weightsIndex = 0;
  static uint8_t state = STATE_IDLE;
  long weight;
  long avgWeight;
  long oldWeight;
  long rate;

  if (state == STATE_FILLING){
    digitalWrite(PIN_MOTOR, HIGH);
  }
  else {
    digitalWrite(PIN_MOTOR, LOW);
  }

  weight = scale.get_units(1);
  oldWeight = weights[weightsIndex];
  weightSum += weight;
  weightSum -= oldWeight;
  weights[weightsIndex] = weight;
  rate = (weight - oldWeight) / WEIGHTS_SIZE;

  if (state == STATE_FILLING){
    // if ((weight + rate * STOP_ITERATIONS) > (target_weight * 10)){
    if (weight + STOP_WEIGHT * 10  > target_weight * 10){
      state = STATE_FULL;
    }
  }

  ++weightsIndex;
  if (weightsIndex >= WEIGHTS_SIZE){
    weightsIndex = 0;
  }

  // Serial.println(weight);
  // Serial.print(" ");
  // Serial.println(rate / 10.0f);
  
  weight = weightSum / WEIGHTS_SIZE;


  bool negative = (weight < 0);
  if (negative){
    weight = -weight;
  }
  long dec = weight % 10;
  weight /= 10;
  if (weightsIndex == 0){
    if(negative){
      Serial.print("-");
    }
    Serial.print(weight, 10);
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
    sprintf(str, "%04d.", weight);
    lcd.print(str);
    sprintf(str, "%1d g", dec);
    lcd.print(str);

    lcd.setCursor(0, 1);
    lcd.print(rate / 10.0f);

    lcd.setCursor(15, 1);
    lcd.print(state);
  }
  // lcd.print(" g");
  // Serial.println(str);  
  // delay(1000);

/* 
  Serial.print("Reading: ");
  Serial.print(weight, 2);
  Serial.print(" kg"); //Change this to kg and re-adjust the calibration factor if you follow SI units like a sane person
  Serial.print(" calibration_factor: ");
  Serial.print(calibration_factor);
  Serial.println();
*/

  // delay(1000);
//  lcd.noDisplay();
  // delay(1000);
//  lcd.display();
/* 
  if(Serial.available())
  {
    char temp = Serial.read();
    if(temp == '+' || temp == 'a')
      calibration_factor += 1000;
    else if(temp == '-' || temp == 'z')
      calibration_factor -= 1000;
    scale.set_scale(calibration_factor); //Adjust to this calibration factor
  }
*/

  char btn = getButtonRelease();
  if (btn == BTN_SEL || btn == BTN_RIGHT){
    tareCalibrate(btn == BTN_SEL);
  }
  else if (btn == BTN_DOWN){
    if (state == STATE_IDLE){
      state = STATE_FILLING;
    }
    else {
      state = STATE_IDLE;
    }
  }
  
}


bool tareCalibrate(bool calibrate){

    lcd.setCursor(0,0);
    lcd.print("Tare...   ");
    lcd.blink();
    // scale.set_scale();
    scale.tare(20);
    lcd.noBlink();
    Serial.print("Tare offset: ");
    Serial.println(scale.get_offset());
    lcd.clear();

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
  x = analogRead(BUTTONS);                  // Read the analog value for buttons
  if (x < 100) {                       // Right button is pressed
    return BTN_RIGHT; 
  }
  else if (x < 200) {                  // Up button is pressed
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