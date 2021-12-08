/*********
  Darrell Good
  Carbon Oven Team
  Engi 200, Rice University
*********/

#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is conntec to the Arduino digital pin 4
#define ONE_WIRE_BUS 5

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

//LCD 
#include <LiquidCrystal_I2C.h> // Library for LCD

LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C address 0x27, 16 column and 2 rows

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;        // will store last time LED was updated


// constants won't change. Used here to set a pin number:
const int heatpin = 3;

const long interval = 10000;           // interval at which to blink (milliseconds)


void setup(void)
{
  // Start serial communication for debugging purposes
  Serial.begin(9600);
  // Start up the library
  sensors.begin();

  pinMode(heatpin, OUTPUT);

  Serial.print("Interval: ");
  Serial.print(interval/1000);
  Serial.println(" sec");

  lcd.init(); // initialize the lcd
  lcd.backlight();

  lcd.setCursor(0, 0);         // move cursor to   (0, 0)
  lcd.print("Ignition!");        // print message at (0, 0)
  lcd.setCursor(2, 1);         // move cursor to   (2, 1)
}

void loop(void) {
  // here is where you'd put code that needs to be running all the time.

  // check to see if it's time to read out the temp
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {

    previousMillis = currentMillis;
    // Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
    sensors.requestTemperatures(); 
    
    // Why "byIndex"? You can have more than one IC on the same bus. 0 refers to the first IC on the wire

    float temps[] = {
      sensors.getTempFByIndex(0),
      sensors.getTempFByIndex(1),
      sensors.getTempFByIndex(2)
    };

    float maxt = 0;
    float sum = 0;
    for (int i = 0; i < sizeof(temps) / sizeof(temps[0]); ++i){
        sum += temps[i];
        if (temps[i] > maxt){
          maxt = temps[i];
        }
        else if (temps[i] < 0){
          //Sensor malfunction??
          // Handle this case gracefully by just using other sensors?
          Serial.print("Sensor ");
          Serial.print(i);
          Serial.println(" reads negative temperatures and is likely malfunctioning.");

        }
    }

    float avg = sum/(sizeof(temps)/ sizeof(temps[0]));
    float avg_rounded = ((int) (avg * 10.0 + 0.5) / 10.0);

    Serial.print(temps[0]);
    Serial.print("\t");
    
    Serial.print(temps[1]);
    Serial.print("\t");
   
    Serial.print(temps[2]);
    Serial.print("\t");
    Serial.println(avg);


    lcd.setCursor(0, 0);         // move cursor to   (0, 0)
  
    lcd.print("Max Temp: ");
    lcd.print(maxt);
  
    lcd.setCursor(0, 1);
    
    lcd.print("Avg Temp: ");
    lcd.print(avg_rounded);

    

    if (maxt > 115){
      // Writing HIGH turns the relay off
      digitalWrite(heatpin, HIGH);
    } else if (avg < 105){
      // While writing low turns the relay on
      digitalWrite(heatpin, LOW); 
    }
  }
}
