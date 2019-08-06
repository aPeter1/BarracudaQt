// Program demonstrating how to control a powerSTEP01-based ST X-NUCLEO-IHM03A1 
// stepper motor driver shield on an Arduino Uno-compatible board
// If you copy the powerSTEP01_Arduino_Library from git hub you must change the 
// contstuctors for the drivers so that it does not add _numboards for every new instance
// I did not daisy chain the nucleos so that will mess it up. 
// you can copy the library on this computer or change constructors to _numboards = 1


// AT the moment 10 mm travel on the microscope objective. 
// use the big nob to move the objective to center 
// Send move to zero command (0)
// Move objective to bottom using big nobs
// Move objective up by sending 1000. (6000 is close to stage)
// change INVERT_INPUT -1/+1 if motor goes backwards. 



#include <powerSTEP01ArduinoLibrary.h>
#include <SPI.h>
#include "QuickMedianLib.h"

// Pin definitions for the X-NUCLEO-IHM03A1 connected to an Uno-compatible board
#define nCS_PIN_2 10
#define STCK_PIN_2 9
#define nSTBY_nRESET_PIN_2 8
#define nBUSY_PIN_2 4
#define flag_2 3


// powerSTEP library instance, parameters are distance from the end of a daisy-chain
// of drivers, !CS pin, !STBY/!Reset pin

// powerSTEP library instance, parameters are distance from the end of a daisy-chain
// of drivers, !CS pin, !STBY/!Reset pin
powerSTEP driver_2(0, nCS_PIN_2, nSTBY_nRESET_PIN_2, nBUSY_PIN_2);


// Set up variables for motors
int outlet_div = 128;
// Set variables for Pressure
int SOLENOID1 = 23;
int SOLENOID2 = 29;

//SERIAL 
String inputString = "";   // a String to hold incoming data
int inputCount = 0;
bool stringComplete = false;  // whether the string is complete


void setup() 
{
  // Start serial
  Serial.begin(1000000);
  Serial.println("powerSTEP01 Arduino control initialising...");

  // Prepare pins
  pinMode(nSTBY_nRESET_PIN_2, OUTPUT);
  pinMode(nCS_PIN_2, OUTPUT);
  pinMode(MOSI, OUTPUT);
  pinMode(MISO, OUTPUT);
  pinMode(SCK, OUTPUT);



  // Reset powerSTEP and set CS
  digitalWrite(nSTBY_nRESET_PIN_2, HIGH);
  digitalWrite(nSTBY_nRESET_PIN_2, LOW);
  digitalWrite(nSTBY_nRESET_PIN_2, HIGH);
  digitalWrite(nCS_PIN_2, HIGH);

 // Turn pressure off
 digitalWrite(SOLENOID1, HIGH);
 digitalWrite(SOLENOID2,HIGH);

 


  // Start SPI
  SPI.begin();
  SPI.setDataMode(SPI_MODE3);

  // Configure powerSTEP

  driver_2.SPIPortConnect(&SPI); // give library the SPI port (only the one on an Uno)
  
  driver_2.configSyncPin(BUSY_PIN, 0); // use SYNC/nBUSY pin as nBUSY, 
                                     // thus syncSteps (2nd paramater) does nothing
                                     
  driver_2.configStepMode(STEP_FS_8); // 1/128 microstepping, full steps = STEP_FS,
                                // options: 1, 1/2, 1/4, 1/8, 1/16, 1/32, 1/64, 1/128
                                
  driver_2.setMaxSpeed(1200); // max speed in units of full steps/s 
  driver_2.setFullSpeed(2000); // full steps/s threshold for disabling microstepping
  driver_2.setAcc(2000); // full steps/s^2 acceleration
  driver_2.setDec(2000); // full steps/s^2 deceleration
  
  driver_2.setSlewRate(SR_980V_us); // faster may give more torque (but also EM noise),
                                  // options are: 114, 220, 400, 520, 790, 980(V/us)
                                  
  driver_2.setOCThreshold(8); // over-current threshold for the 2.8A NEMA23 motor
                            // used in testing. If your motor stops working for
                            // no apparent reason, it's probably this. Start low
                            // and increase until it doesn't trip, then maybe
                            // add one to avoid misfires. Can prevent catastrophic
                            // failures caused by shorts
  driver_2.setOCShutdown(OC_SD_ENABLE); // shutdown motor bridge on over-current event
                                      // to protect against permanant damage
  
  driver_2.setPWMFreq(PWM_DIV_1, PWM_MUL_0_75); // 16MHz*0.75/(512*1) = 23.4375kHz 
                            // power is supplied to stepper phases as a sin wave,  
                            // frequency is set by two PWM modulators,
                            // Fpwm = Fosc*m/(512*N), N and m are set by DIV and MUL,
                            // options: DIV: 1, 2, 3, 4, 5, 6, 7, 
                            // MUL: 0.625, 0.75, 0.875, 1, 1.25, 1.5, 1.75, 2
                            
  driver_2.setVoltageComp(VS_COMP_DISABLE); // no compensation for variation in Vs as
                                          // ADC voltage divider is not populated
                                          
  driver_2.setSwitchMode(SW_USER); // switch doesn't trigger stop, status can be read.
                                 // SW_HARD_STOP: TP1 causes hard stop on connection 
                                 // to GND, you get stuck on switch after homing
                                      
  driver_2.setOscMode(INT_16MHZ); // 16MHz internal oscillator as clock source

  // KVAL registers set the power to the motor by adjusting the PWM duty cycle,
  // use a value between 0-255 where 0 = no power, 255 = full power.
  // Start low and monitor the motor temperature until you find a safe balance
  // between power and temperature. Only use what you need
  driver_2.setRunKVAL(128);
  driver_2.setAccKVAL(128);
  driver_2.setDecKVAL(128);
  driver_2.setHoldKVAL(32);

  driver_2.setParam(ALARM_EN, 0x8F); // disable ADC UVLO (divider not populated),
                                   // disable stall detection (not configured),
                                   // disable switch (not using as hard stop)

  driver_2.getStatus(); // clears error flags

  Serial.println(F("Initialisation 2 complete"));
  
}


void serialCheck() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    // add it to the inputString:
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
      }else{
      inputString += inChar;
      }
  }
}

void moveMotor(){
  // Move the motor to specified position.
  driver_2.getStatus();
  driver_2.hardStop();
  int chnl = atoi(inputString[1]);
  String location = inputString.substring(3,13);
  float steps = location.toFloat();
  //#Serial.print("Move Motor ");
  //#Serial.print(inputString[1]);
  //#Serial.print(" To Location: ");
  //Serial.println(steps);
  steps = long(steps/8.*200.*outlet_div);
  Serial.println(steps);
  driver_2.goTo(steps);
}


void motorTalk(){
  // If the motor command (M) was issued determine the channel, and the command. 
  // M0L+000100 Move to step position 100 for motor channel 0. 
  // Commands: L+0000 Move to position
  // L? Request Postion
  // H Set home/origin position 
  // S+00000 set speed
  int chnl = atoi(inputString[1]);
  if (inputString[2]=='L'){
    if (inputString[3]=='?'){
      getMotorPos();
    } else {
    moveMotor();
    }
  }
  else if (inputString[2]=='H'){
    setHome();
  } else if (inputString[2]=='S'){
    setMotorSpeed();
  } else if (inputString[2]=='R'){
    resetDriver();
  }
}

void getMotorPos(){
  int chnl = atoi(inputString[1]);
  //Serial.print("Get Motor  Position for channel ");
  //Serial.print(chnl);
  //Serial.println(": ");
  long steps = driver_2.getPos();
  float return_steps = float(float(steps)/outlet_div/200.*8.);
  Serial.println(return_steps);
}

void setHome(){
  int chnl = atoi(inputString[1]);
  //Serial.print("Setting Home Position for channel: ");
  Serial.println(chnl);
  driver_2.resetPos();
}

void setMotorSpeed(){
  int chnl = atoi(inputString[1]);
  String location = inputString.substring(3,8);
  int stepspersec = location.toInt();
  //Serial.print(" Setting speed ");
  //Serial.print(stepspersec);
  //Serial.print(" for channnel: ");
  //Serial.println(chnl);
  driver_2.setMaxSpeed(stepspersec);
}

void pressureTalk(){
  int chnl = atoi(inputString[1]);
  //Serial.print("Setting Pressure ");
  //Serial.print(chnl);
  //Serial.println(": ");
  //Serial.println(inputString[2]);
  if (inputString[2]=='R'){
    Serial.println("ON");
    digitalWrite(SOLENOID2,LOW);  
    digitalWrite(SOLENOID1,HIGH);
  } else if(inputString[2]=='S'){
    digitalWrite(SOLENOID2,HIGH);
    digitalWrite(SOLENOID1,LOW);
    Serial.println("OFF");
  } else if(inputString[2]=='X'){
    digitalWrite(SOLENOID1,LOW);
    digitalWrite(SOLENOID2,LOW);
    Serial.println("All OPEN");
  } else if (inputString[2]=='C'){
    digitalWrite(SOLENOID1, HIGH);
    digitalWrite(SOLENOID2, HIGH);
    Serial.println("All CLOSED");
  }
}

void resetDriver(){
  digitalWrite(nSTBY_nRESET_PIN_2, HIGH);
  digitalWrite(nSTBY_nRESET_PIN_2, LOW);
  digitalWrite(nSTBY_nRESET_PIN_2, HIGH);
  digitalWrite(nSTBY_nRESET_PIN_2, LOW);
  digitalWrite(nCS_PIN_2, HIGH);
}

void loop() 
{ 
   // print the string when a newline arrives:

  serialCheck();
  if (stringComplete) {
    Serial.println(inputString);
    if (inputString[0]=='M'){
      motorTalk();
    }else if (inputString[0]=='P'){
      pressureTalk();
    }
    else if (inputString[0]=='S'){
      Serial.println("Run");
    }
    // clear the string:
    inputString = "";
    stringComplete = false;
  }
}