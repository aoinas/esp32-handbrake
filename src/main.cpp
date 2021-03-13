#include <Arduino.h>
#include <sstream>
#include <BleGamepad.h>

#define POTENTIOMETER_PIN A5
#define CALIBRATE_BUTTON 0
#define ANALOG_READ_MAX_VAL 4095

#define MIN_INT16_VALUE -32767
#define MAX_INT16_VALUE 32767

const static double LEVER_POS_TOLERANCE = (double)ANALOG_READ_MAX_VAL * 0.02; // 1% tolerance

enum RunMode {  SETUP, // Setup mode is used to reprogram esp32 OR calibrating range
                           RUN };

double _minPos = 0;
double _maxPos = 0;
RunMode _currRunMode = RunMode::RUN;

int _loopCounter = 0;
double _currLeverPosition = 0;
double _output = 0.0;
double _rawPosition = 0.0;
int16_t _actualOutput = 0;

BleGamepad bleGamepad("ESP32 Handbrake", "Softhare Solutions Inc.", 100);

double getMomentaryPos() {
      return (double)analogRead(POTENTIOMETER_PIN);
}

void resetCalibration() {
    // Lets presume, that since lever is springloaded, it will be on its zero position on start
  _minPos = getMomentaryPos(); // Initialise with resting position
  _maxPos = _minPos + 50.0; // Initialize with slightly higher value
} 

void setup() {

  pinMode(POTENTIOMETER_PIN, INPUT);
  pinMode(CALIBRATE_BUTTON, INPUT);
  Serial.begin(115200);

  resetCalibration();
  _currLeverPosition = _minPos;

  bleGamepad.begin(true);

  // Wait 5s for BT connection
  delay(5000);
  if (bleGamepad.isConnected())
    bleGamepad.setX(MIN_INT16_VALUE);
}

void loop() {
  
  try {

      const RunMode newRunMode = digitalRead(CALIBRATE_BUTTON) == HIGH ? RUN : SETUP;

      // Has mode been changed
      if ( newRunMode != _currRunMode)  {
        _currRunMode = newRunMode;
      }

      // Do not read values in setup mode
      if (_currRunMode == RunMode::SETUP)
      {
        delay(1000);
        return;
      }

      const double momentaryPos = getMomentaryPos();
      
      // If lever position change exceeds tolerance
      if ( abs(_currLeverPosition - momentaryPos) > LEVER_POS_TOLERANCE) { 
        _currLeverPosition = momentaryPos;
        _maxPos = std::max(_maxPos, _currLeverPosition); // Recalibrata max val if needed
        
        // Continue only if we have sane min and max
        if ( _maxPos > _minPos) {
          double tempOutput = ( _currLeverPosition - _minPos ) / (_maxPos-_minPos);
          double _output = std::min(std::max(tempOutput, 0.0), 1.0); // Do not exceed 100%

          // To avoid "dragging brake, do not report values below 10%"
          if ( _output < 0.1)
            _output = 0.0;

          // Send new value via bluetooth
          if(bleGamepad.isConnected()) {
            int16_t _actualOutput = (int16_t)(MIN_INT16_VALUE +  (MAX_INT16_VALUE-MIN_INT16_VALUE) * _output);
            bleGamepad.setY(_actualOutput);
            std::stringstream ss;
            ss << "Sent output via BT:" << _output << "% Actual output value: " << _actualOutput;
            Serial.println(ss.str().c_str());
          }
        }
        
      }
    } 

  catch (const std::exception& e)
  {
    Serial.println("Exception occurred: ");
    Serial.println(e.what());
  }
  
 /*  if ( _loopCounter % 1000 == 0)
  {
    Serial.println( bleGamepad.isConnected() ? "BT connected" : "BT not connected");
    Serial.print("About to send value for X-axis:") ;
    Serial.println(_actualOutput);

    Serial.print("Current value: ");
    Serial.print(_currLeverPosition);

    Serial.print("  (Range: ");
    Serial.print(_minPos);
    Serial.print(" - ");
    Serial.print(_maxPos);
    Serial.print("  Raw: ");
    Serial.println(_rawPosition);

    Serial.print("Output: ");
    Serial.print(_output * 100.0);
    Serial.println("%");

    Serial.print("Current calibrate button pos: ");
    Serial.println(_currRunMode == SETUP ? "SETUP" : "RUN");

    Serial.println("");

    int chars = (int)_output / 10;

    for (int i=1; i <= 10; i++) {
      Serial.print( i < chars ? "#" : "-");
    }
    
    Serial.println("");

  } */
  
  delay(10);
  _loopCounter++;

}