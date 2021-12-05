/*********
  Darrell Good
  Carbon Oven Team
  Engi 200, Rice University
*********/

#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is conntec to the Arduino digital pin 4
#define ONE_WIRE_BUS 4

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

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
}

void loop(void) {
  // here is where you'd put code that needs to be running all the time.

  // check to see if it's time to blink the LED; that is, if the difference
  // between the current time and last time you blinked the LED is bigger than
  // the interval at which you want to blink the LED.
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {

    previousMillis = currentMillis;
    // Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
    sensors.requestTemperatures(); 
    
    // Why "byIndex"? You can have more than one IC on the same bus. 0 refers to the first IC on the wire


    float t1 = sensors.getTempFByIndex(0);
    float t2 = sensors.getTempFByIndex(1);
    float t3 = sensors.getTempFByIndex(2);
    float t4 = sensors.getTempFByIndex(3);
    //float avg = (t1 + t2 + t3 + t4)/4;

    float avg = t1; // Only using 1 for now

    Serial.print(t1);
    Serial.print("\t");
    
    Serial.print(t2);
    Serial.print("\t");
   
    Serial.print(t3);
    Serial.print("\t");
    
    Serial.print(t4);
    Serial.print("\t");
    Serial.println(avg);

    

    if (avg > 110){
      digitalWrite(heatpin, HIGH);
    } else if (avg < 105){
      digitalWrite(heatpin, LOW); 
    }
  }
}
