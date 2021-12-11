/*********
  Darrell Good
  Carbon Oven Team
  Engi 200, Rice University
*********/

//---------------------------------------------------------------------------------------
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is connected to the Arduino digital pin 5
#define ONE_WIRE_BUS 5

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);
//---------------------------------------------------------------------------------------


//---------------------------------------------------------------------------------------
//LCD 
#include <LiquidCrystal_I2C.h> // Library for LCD

LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C address 0x27, 16 column and 2 rows
//---------------------------------------------------------------------------------------


//---------------------------------------------------------------------------------------
#define STARTSTOP_PIN 2
#define HEATWIRE_1_PIN 3
//#define HEATWIRE_2_PIN 6
#define MOTOR_PIN 4

#define R_LED 9
#define G_LED 10
#define B_LED 11

#define TIMER_PIN A1
#define TEMP_PIN A2

//---------------------------------------------------------------------------------------
#define UPDATE_TIME 500
#define DATA_OUT_TIME 60000
#define POT_THRESHOLD 10

#define RELAY_DELAY 500

//---------------------------------------------------------------------------------------
// Error is 0
#define STANDBY 1
#define HEATING 2
#define COOLING 3
int state = 1;

#define COOLING_TEMP 80
//---------------------------------------------------------------------------------------


long settings_changed = 0;
int prev_temp = analogRead(TEMP_PIN);
int max_temp = 0;
long prev_cure_length = 0;
long timer = 0;



bool pressed = 0;
unsigned long last_button_time = 0;
bool last_interrupt_state = HIGH;

int prev_chamber_temp = 0;
int chamber_temp = 0;
float temps[] = {0,0,0};

long eta = 0;
unsigned long started_at = 0;

unsigned long last_read = 0;
unsigned long last_data_out = 0;

void setup() {  
  heatersOFF();
  motorOFF();
  
  // Start serial communication for debugging purposes
  Serial.begin(9600);
  // Start up the library
  sensors.begin();

  pinMode(STARTSTOP_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(STARTSTOP_PIN), start_stop_handler, CHANGE);

  pinMode(HEATWIRE_1_PIN, OUTPUT);
  //pinMode(HEATWIRE_2_PIN, OUTPUT);
  pinMode(MOTOR_PIN, OUTPUT);
  pinMode(R_LED, OUTPUT);
  pinMode(G_LED, OUTPUT);
  pinMode(B_LED, OUTPUT);

  LEDOFF();

  lcd.init(); // initialize the lcd
  lcd.backlight();

  lcd.setCursor(0, 0);         // move cursor to   (0, 0)
  lcd.print("Boot-up!");        // print message at (0, 0)
  lcd.setCursor(2, 1);         // move cursor to   (2, 1)

  state = 1;
  pressed = 0;

  heatersOFF();
  motorOFF();
}


void loop() {
  // put your main code here, to run repeatedly:
  delay(5);

  // Reset the button
  if (pressed && millis() - last_button_time > 50 && last_interrupt_state) {
    switch (state) {
      case STANDBY:
        state = HEATING;
        started_at = millis();
        break;
      case HEATING:
        state = STANDBY;
        break;
      case COOLING:
        state = STANDBY;
        break;
    }
    pressed = 0;
    lcd.clear();
  }

  updateLED();
  logData();
  
  //--------------------------------------------------------------------------
  if (state == STANDBY){
    readSettings();
    displaySettings();
    heatersOFF();
    motorOFF();
  }
  //--------------------------------------------------------------------------
  if (state == HEATING){
    unsigned long now = millis();
    eta = (started_at + timer) - now;
    displayCurrentState();
    updateChamberTemp();
    if (eta <= 0) {
      state = COOLING;
    }
    updateHeaters();
    delay(RELAY_DELAY);
    motorON();
  }
  //--------------------------------------------------------------------------
  if (state == COOLING){
    heatersOFF();
    updateChamberTemp();
    displayCurrentState();
    if (chamber_temp < COOLING_TEMP) {
      state = STANDBY;
    }
  }
}

void logData() {
  if (millis() - last_data_out > DATA_OUT_TIME) {
    getChamberTemps();
    Serial.print(temps[0]);
    Serial.print("\t");
    Serial.print(temps[1]);
    Serial.print("\t");
    Serial.println(temps[2]);
    last_data_out = millis();
  }
}

void updateChamberTemp() {
  if (millis() - last_read > UPDATE_TIME) {
    prev_chamber_temp = chamber_temp;
    chamber_temp = estimateTempAtMandrel();
    last_read = millis();
  }
}

void displaySettings(){
  lcd.setCursor(0, 0);         // move cursor to   (0, 0)
  lcd.print("Timer: ");        // print message at (0, 0)
  int hours = timer / ((long)3600000);
  if (hours < 10) lcd.print("0");
  lcd.print(hours);
  lcd.print(":");
  int mins = (timer % ((long)3600000))/60000;
  if (mins < 10) lcd.print("0");
  lcd.print(mins);
  lcd.print(" h:m");
  lcd.setCursor(0, 1); // move cursor to   (2, 1)
  lcd.print(" Temp: ");
  if (max_temp < 100) lcd.print(" ");
  lcd.print(max_temp);
  lcd.print("\xDF"); // Degrees symbol [°]
  lcd.print("F");
}


void readSettings(){
  int temp = analogRead(TEMP_PIN);
  if (abs(temp - prev_temp) > POT_THRESHOLD){
    prev_temp = temp;
    max_temp = map(temp, 0, 1023, 64, 120);
    if (max_temp < 70) max_temp = 70;
    if (max_temp > 115) max_temp = 115;
    settings_changed = millis();
  }

  int cure_length = analogRead(TIMER_PIN);
  if ( abs(cure_length - prev_cure_length) > POT_THRESHOLD){
    prev_cure_length = cure_length;
    timer = map(cure_length, 90, 990, 0, 3600);
    if (timer < 0) timer = 0;
    if (timer > 3600L) timer = 3600L;
    timer *= 20000L;
    settings_changed = millis();
  }
}

void updateLED() {
  if (state == 1) writeLED(0,0,150);
  else if (state == 2) writeLED(120, 80, 0);
  else if (state == 3) writeLED(0, 150, 0);
  else writeLED(150, 0, 0);
}

void start_stop_handler() {
  pressed = 1;
  last_button_time = millis();
  last_interrupt_state = digitalRead(STARTSTOP_PIN);
}

void writeLED(int r, int g, int b)
{
  analogWrite(R_LED, 255-r);
  analogWrite(G_LED, 255-g);
  analogWrite(B_LED, 255-b);
}

void LEDOFF() {
  writeLED(0,0,0);
}

void displayCurrentState() {
  lcd.setCursor(0, 1);         // move cursor to   (0, 0)
  lcd.print("Tmp/Max: ");
  lcd.print(chamber_temp);
  lcd.print("/");
  lcd.print(max_temp);

  lcd.setCursor(0, 0); // move cursor to   (2, 1)
  lcd.print("Timer: ");
  int eta_hours = eta / ((long)3600000);
  if (eta_hours < 10) lcd.print("0");
  lcd.print(eta_hours);
  lcd.print(":");
  int eta_mins = (eta % ((long)3600000))/60000;
  if (eta_mins < 10) lcd.print("0");
  lcd.print(eta_mins);
  lcd.print(":");
  int eta_sec = (eta % 60000)/1000;
  if (eta_sec < 10) lcd.print("0");
  lcd.print(eta_sec);
}

void getChamberTemps() {
  // Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
  sensors.requestTemperatures(); 
    
  // Why "byIndex"? You can have more than one IC on the same bus. 0 refers to the first IC on the wire
  temps[0] = sensors.getTempFByIndex(0);
  temps[1] = sensors.getTempFByIndex(1);
  temps[2] = sensors.getTempFByIndex(2);

}
float getMaxChamberTemp(){
  getChamberTemps();
  
  float sum = 0;
  float avg = sum/(sizeof(temps)/ sizeof(temps[0]));
  float avg_rounded = ((int) (avg * 10.0 + 0.5) / 10.0);

  float maxt = 0;
  for (int i = 0; i < sizeof(temps) / sizeof(temps[0]); ++i){
      sum += temps[i];
      if (temps[i] > maxt){
        maxt = temps[i];
      }
      else if (temps[i] < 0){
        //Sensor malfunction??
        // Handle this case gracefully by just using other sensors?
        //Serial.print("Sensor ");
        //Serial.print(i);
        //Serial.println(" reads negative temperatures and is likely malfunctioning.");
        //lcd.setCursor(0, 0);         // move cursor to   (0, 0)
        //lcd.print("Temp Sensor Error on ");        // print message at (0, 0)
        //lcd.print(i);
      }
  }
  return maxt;
}

int estimateTempAtMandrel() {
  // Since we can't measure the temperature right at the mandrel
  // estimate what it should be based on empirical testing
  
  float maxRead = getMaxChamberTemp();
  
  #define ROOM_TEMP  75
  #define MEASURED_MAX  121   // 
  #define ACTUAL_MAX  134     // Measured with external tooling

  // The offset is approximately linear, but varies with cycle time:
  float guessTemp = (maxRead - ROOM_TEMP) * (ACTUAL_MAX - ROOM_TEMP)/(MEASURED_MAX - ROOM_TEMP) + ROOM_TEMP;
  return round(guessTemp);
}

void updateHeaters() {
  #define OVERSHOOT_OFFSET 6 //
  if (prev_chamber_temp < chamber_temp) {
    // Temp rising:
    if (chamber_temp > max_temp - OVERSHOOT_OFFSET) heatersOFF();
  } else {
    // Temp falling:
    if (chamber_temp < max_temp ) heatersON();
  }

  if (chamber_temp > max_temp) {
    heatersOFF();
    Serial.println("Max temp overshot!!");
  }
}

void heatersON() {
  digitalWrite(HEATWIRE_1_PIN, HIGH);
  //delay(RELAY_DELAY);
  //digitalWrite(HEATWIRE_2_PIN, LOW);
  //delay(RELAY_DELAY);
}

void heatersOFF() {
  digitalWrite(HEATWIRE_1_PIN, LOW);
  //delay(RELAY_DELAY);
  //digitalWrite(HEATWIRE_2_PIN, HIGH);
  //delay(RELAY_DELAY);
}

void motorON() {
  digitalWrite(MOTOR_PIN, LOW);
}

void motorOFF() {
  digitalWrite(MOTOR_PIN, HIGH);
}
