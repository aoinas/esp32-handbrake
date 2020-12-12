#include <Arduino.h>
#include <BleGamepad.h>

#define POTENTIOMETER_PIN A5
#define CALIBRATE_BUTTON 0
#define ANALOG_READ_MAX_VAL 4095

const static double LEVER_POS_TOLERANCE = (double)ANALOG_READ_MAX_VAL * 0.02; // 1% tolerance

enum RunMode {  SETUP, // Setup mode is used to reprogram esp32 OR calibrating range
                           RUN };

double _minPos = 0;
double _maxPos = 0;
RunMode _currRunMode = RunMode::RUN;

BleGamepad bleGamepad("ESP32 Handbrake", "Softhare Solutions Inc.", 100);

double getMomentaryPos() {
      return (double)analogRead(POTENTIOMETER_PIN);
}

void resetCalibration() {
    // Lets presume, that since lever is springloaded, it will be on its zero position on start
  _minPos = getMomentaryPos(); // Initialise with resting position
  _maxPos = _minPos + 100.0; // Initialize with slightly higher value
} 

void setup() {

  pinMode(POTENTIOMETER_PIN, INPUT);
  pinMode(CALIBRATE_BUTTON, INPUT);
  Serial.begin(115200);

  resetCalibration();

  bleGamepad.begin();
}

int _loopCounter = 0;
double _currLeverPosition = 0;
double _output = 0.0;
double _rawPosition = 0.0;
signed char _outputChar = 0;

void loop() {
  
  try {

      const RunMode newRunMode = digitalRead(CALIBRATE_BUTTON) == HIGH ? RUN : SETUP;

      // Has mode been changed
      if ( newRunMode != _currRunMode)  {
        if ( newRunMode == SETUP ) {
          resetCalibration();
        }
        _currRunMode = newRunMode;
      }

      const double momentaryPos = getMomentaryPos();
      
      // If lever position change exceeds tolerance
      if ( abs(_currLeverPosition - momentaryPos) > LEVER_POS_TOLERANCE) { 
        _currLeverPosition = momentaryPos;
        _maxPos = std::max(_maxPos, _currLeverPosition); // Recalibrata max val if needed
        
        // Continue only if we have sane min and max
        if ( _maxPos > _minPos) {
          _output = ( _currLeverPosition - _minPos ) / (_maxPos-_minPos);
          double validatedOutput = std::max(_output, 0.0);
          validatedOutput = std::min(validatedOutput, 1.0); // Do not exceed 100%

          // Send new value via bluetooth
          if(bleGamepad.isConnected()) {
            signed char outputInt = (signed char)(127 * validatedOutput);
            _outputChar = outputInt;
            bleGamepad.setAxes(0, outputInt);
          }
        }
        
      }
    } 

  catch (const std::exception& e)
  {
    Serial.println("Exception occurred: ");
    Serial.println(e.what());
  }
  
  if ( _loopCounter % 2000 == 0)
  {
    Serial.println( bleGamepad.isConnected() ? "BT connected" : "BT not connected");
    Serial.print("About to send value for X-axis:") ;
    Serial.println(_outputChar);

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

  }
  
  delay(10);
  _loopCounter++;

}