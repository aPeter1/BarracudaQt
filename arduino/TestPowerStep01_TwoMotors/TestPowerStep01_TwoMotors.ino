// Program demonstrating how to control a powerSTEP01-based ST X-NUCLEO-IHM03A1 
// stepper motor driver shield on an Arduino Uno-compatible board

#include <powerSTEP01ArduinoLibrary.h>
#include <SPI.h>

// Pin definitions for the X-NUCLEO-IHM03A1 connected to an Uno-compatible board
#define nCS_PIN 7
#define STCK_PIN 9
#define nSTBY_nRESET_PIN 6
#define nBUSY_PIN 5



// Pin definitions for the X-NUCLEO-IHM03A1 connected to an Uno-compatible board
#define nCS_PIN_2 10
#define STCK_PIN_2 9
#define nSTBY_nRESET_PIN_2 8
#define nBUSY_PIN_2 4
#define flag_2 3
// powerSTEP library instance, parameters are distance from the end of a daisy-chain
// of drivers, !CS pin, !STBY/!Reset pin
powerSTEP driver(1, nCS_PIN, nSTBY_nRESET_PIN, nBUSY_PIN);
// powerSTEP library instance, parameters are distance from the end of a daisy-chain
// of drivers, !CS pin, !STBY/!Reset pin
powerSTEP driver_2(1, nCS_PIN_2, nSTBY_nRESET_PIN_2, nBUSY_PIN_2);

void setup() 
{
  // Start serial
  Serial.begin(9600);
  Serial.println("powerSTEP01 Arduino control initialising...");

  // Prepare pins
  pinMode(nSTBY_nRESET_PIN, OUTPUT);
  pinMode(nCS_PIN, OUTPUT);
  pinMode(nSTBY_nRESET_PIN_2, OUTPUT);
  pinMode(nCS_PIN_2, OUTPUT);
  pinMode(MOSI, OUTPUT);
  pinMode(MISO, OUTPUT);
  pinMode(SCK, OUTPUT);


  // Reset powerSTEP and set CS
  digitalWrite(nSTBY_nRESET_PIN, HIGH);
  digitalWrite(nSTBY_nRESET_PIN, LOW);
  digitalWrite(nSTBY_nRESET_PIN, HIGH);
  digitalWrite(nCS_PIN, HIGH);

  digitalWrite(nSTBY_nRESET_PIN_2, HIGH);
  digitalWrite(nSTBY_nRESET_PIN_2, LOW);
  digitalWrite(nSTBY_nRESET_PIN_2, HIGH);
  digitalWrite(nCS_PIN_2, HIGH);
 


  // Start SPI
  SPI.begin();
  SPI.setDataMode(SPI_MODE3);

  // Configure powerSTEP
  driver.SPIPortConnect(&SPI); // give library the SPI port (only the one on an Uno)
  
  driver.configSyncPin(BUSY_PIN, 0); // use SYNC/nBUSY pin as nBUSY, 
                                     // thus syncSteps (2nd paramater) does nothing
                                     
  driver.configStepMode(STEP_FS_128); // 1/128 microstepping, full steps = STEP_FS,
                                // options: 1, 1/2, 1/4, 1/8, 1/16, 1/32, 1/64, 1/128
                                
  driver.setMaxSpeed(1000); // max speed in units of full steps/s 
  driver.setFullSpeed(2000); // full steps/s threshold for disabling microstepping
  driver.setAcc(2000); // full steps/s^2 acceleration
  driver.setDec(2000); // full steps/s^2 deceleration
  
  driver.setSlewRate(SR_520V_us); // faster may give more torque (but also EM noise),
                                  // options are: 114, 220, 400, 520, 790, 980(V/us)
                                  
  driver.setOCThreshold(8); // over-current threshold for the 2.8A NEMA23 motor
                            // used in testing. If your motor stops working for
                            // no apparent reason, it's probably this. Start low
                            // and increase until it doesn't trip, then maybe
                            // add one to avoid misfires. Can prevent catastrophic
                            // failures caused by shorts
  driver.setOCShutdown(OC_SD_ENABLE); // shutdown motor bridge on over-current event
                                      // to protect against permanant damage
  
  driver.setPWMFreq(PWM_DIV_1, PWM_MUL_0_75); // 16MHz*0.75/(512*1) = 23.4375kHz 
                            // power is supplied to stepper phases as a sin wave,  
                            // frequency is set by two PWM modulators,
                            // Fpwm = Fosc*m/(512*N), N and m are set by DIV and MUL,
                            // options: DIV: 1, 2, 3, 4, 5, 6, 7, 
                            // MUL: 0.625, 0.75, 0.875, 1, 1.25, 1.5, 1.75, 2
                            
  driver.setVoltageComp(VS_COMP_DISABLE); // no compensation for variation in Vs as
                                          // ADC voltage divider is not populated
                                          
  driver.setSwitchMode(SW_USER); // switch doesn't trigger stop, status can be read.
                                 // SW_HARD_STOP: TP1 causes hard stop on connection 
                                 // to GND, you get stuck on switch after homing
                                      
  driver.setOscMode(INT_16MHZ); // 16MHz internal oscillator as clock source

  // KVAL registers set the power to the motor by adjusting the PWM duty cycle,
  // use a value between 0-255 where 0 = no power, 255 = full power.
  // Start low and monitor the motor temperature until you find a safe balance
  // between power and temperature. Only use what you need
  driver.setRunKVAL(64);
  driver.setAccKVAL(64);
  driver.setDecKVAL(64);
  driver.setHoldKVAL(32);

  driver.setParam(ALARM_EN, 0x8F); // disable ADC UVLO (divider not populated),
                                   // disable stall detection (not configured),
                                   // disable switch (not using as hard stop)

  driver.getStatus(); // clears error flags

  Serial.println(F("Initialisation complete"));

  driver_2.SPIPortConnect(&SPI); // give library the SPI port (only the one on an Uno)
  
  driver_2.configSyncPin(BUSY_PIN, 0); // use SYNC/nBUSY pin as nBUSY, 
                                     // thus syncSteps (2nd paramater) does nothing
                                     
  driver_2.configStepMode(STEP_FS_128); // 1/128 microstepping, full steps = STEP_FS,
                                // options: 1, 1/2, 1/4, 1/8, 1/16, 1/32, 1/64, 1/128
                                
  driver_2.setMaxSpeed(1000); // max speed in units of full steps/s 
  driver_2.setFullSpeed(2000); // full steps/s threshold for disabling microstepping
  driver_2.setAcc(2000); // full steps/s^2 acceleration
  driver_2.setDec(2000); // full steps/s^2 deceleration
  
  driver_2.setSlewRate(SR_520V_us); // faster may give more torque (but also EM noise),
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
  driver_2.setRunKVAL(64);
  driver_2.setAccKVAL(64);
  driver_2.setDecKVAL(64);
  driver_2.setHoldKVAL(32);

  driver_2.setParam(ALARM_EN, 0x8F); // disable ADC UVLO (divider not populated),
                                   // disable stall detection (not configured),
                                   // disable switch (not using as hard stop)

  driver_2.getStatus(); // clears error flags

  Serial.println(F("Initialisation 2 complete"));
  
}

void loop() 
{ 
  Serial.println((int)driver.getStatus(), HEX); // print STATUS register
  
  driver.move(FWD, 20000); // move forward 2000 microsteps
  driver_2.move(FWD, 20000); // move forward 2000 microsteps
  while(driver.busyCheck()); // wait fo the move to finish
  driver.softStop(); // soft stops prevent errors in the next operation
  while(driver.busyCheck());
  
  driver.move(REV, 20000); // reverse back
  driver_2.softStop(); // soft stops prevent errors in the next operation
  driver_2.move(REV, 20000); // reverse back
  while(driver.busyCheck());
  driver.softStop();
  while(driver.busyCheck());


  while(driver_2.busyCheck()); // wait fo the move to finish

  while(driver_2.busyCheck());
  driver_2.softStop();
  while(driver_2.busyCheck());
  
  
  Serial.println((int)driver_2.getStatus(), HEX);
  
  while(1); // do nothing for all time
}
