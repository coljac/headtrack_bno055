#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

// #define TESTING 1

#define LED_PIN 13  
#define SERIAL_RATE 57600
// 19200
Adafruit_BNO055 bno = Adafruit_BNO055(55);

//float roll = 0;
//float pitch = 0;
float yaw = 0;
float offset_yaw, offset_pitch, offset_roll = 0;
sensors_event_t event; 

unsigned long current_time;
unsigned long last_time;
unsigned long elapsed_time;
uint8_t systems = 0;
uint8_t gyro, accel, mag;
uint8_t stopped = 0;
  
typedef struct  {
  int16_t  Begin  ;   // 2  Debut
  uint16_t Cpt ;      // 2  Compteur trame or Code info or error
  float    gyro[3];   // 12 [Y, P, R]    gyro
  float    acc[3];    // 12 [x, y, z]    Acc
  int16_t  End ;      // 2  Fin
} _hatire;

typedef struct  {
  int16_t  Begin  ;   // 2  Debut
  uint16_t Code ;     // 2  Code info
  char     Msg[24];   // 24 Message
  int16_t  End ;      // 2  Fin
} _msginfo;

_hatire hatire;
_msginfo msginfo;
char command;

void PrintCodeSerial(uint16_t code,char Msg[24],bool EOL ) {
  msginfo.Code=code;
  memset(msginfo.Msg,0x00,24);
  strcpy(msginfo.Msg,Msg);
  if (EOL) msginfo.Msg[23]=0x0A;
  // Send HATIRE message to  PC
  Serial.write((byte*)&msginfo,30);
}

void setup(void) 
{
  Serial.begin(SERIAL_RATE);
//  Serial.println("Orientation Sensor Test"); Serial.println("");
  
  /* Initialise the sensor */
  if(!bno.begin())
  {
    /* There was a problem detecting the BNO055 ... check your connections */
    #ifdef TESTING
    Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
    #else
    PrintCodeSerial(9007,"No BNO055 detected...",true);
    #endif 
    while(1);
  }
  
  delay(250);

  while (Serial.available() && Serial.read()); // empty buffer
  PrintCodeSerial(3004,"Initializing BNO...",true);

  bno.setExtCrystalUse(true);
  recalibrate(0);
  
  if(!system) {
    PrintCodeSerial(9007,"Failed to calibrate...",true);        
  }

  hatire.Begin=0xAAAA;
  hatire.Cpt=0;
  hatire.End=0x5555;
  
}

uint8_t recalibrate(int begin) {
  if(begin) {
      bno.begin();
  }
  systems = gyro = accel = mag = 0;
  PrintCodeSerial(3004,"Initializing BNO...",true);    
  while(systems != 3) {
      bno.getCalibration(&systems, &gyro, &accel, &mag);
      PrintCodeSerial(5001,"Calibrating...",true);
      delay(250);
  }
  zero();
  return systems;
}


void zero(void) {
  bno.getEvent(&event);
  offset_yaw = event.orientation.x;
  offset_pitch = event.orientation.y;
  offset_roll = event.orientation.z;
}


void serialEvent(){
  command = (char)Serial.read();
  switch (command) {
  case 'S':
    PrintCodeSerial(5001,"HAT START",true);
    stopped = 0;
    bno.enterNormalMode();
    pinMode(LED_PIN, 1);

    break;      

  case 's':
    PrintCodeSerial(5002,"HAT STOP",true);
    stopped = 1;
    bno.enterSuspendMode();
    pinMode(LED_PIN, 0);
    break;      

  case 'R':
    PrintCodeSerial(5003,"HAT RESET",true);
    recalibrate(1);
    break;      

  case 'Z':
    PrintCodeSerial(5003,"HAT ZERO",true);
    zero();
    break;      


  case 'C':
    break;      

  case 'V':
//    PrintCodeSerial(2000,Version,true);
    break;      


  case 'I':
    displayCalStatus();
    break;      


  default:
    break;
  } 
}

void displayCalStatus(void)
{
  /* Get the four calibration values (0..3) */
  /* Any sensor data reporting 0 should be ignored, */
  /* 3 means 'fully calibrated" */
  uint8_t gyro, accel, mag;
  systems = gyro = accel = mag = 0;
  bno.getCalibration(&systems, &gyro, &accel, &mag);

  /* The data should be ignored until the system calibration is > 0 */
  Serial.print("\t");
  if (!systems)
  {
    Serial.print("! ");
  }

  /* Display the individual values */
  Serial.print("Sys:");
  Serial.print(systems, DEC);
  Serial.print(" G:");
  Serial.print(gyro, DEC);
  Serial.print(" A:");
  Serial.print(accel, DEC);
  Serial.print(" M:");
  Serial.println(mag, DEC);
}

void loop(void) 
{
  
  if(Serial.available() > 0)  serialEvent();
  if(stopped) {
    delay(100);
    return;
  }
  /* Get a new sensor event */ 
  bno.getEvent(&event);
  yaw = event.orientation.x - offset_yaw;
  if(yaw>180.0) {
    yaw = yaw - 360;
  }
  hatire.gyro[0] = yaw;
  hatire.gyro[1] = event.orientation.y - offset_pitch;
  hatire.gyro[2] = event.orientation.z - offset_roll;
#ifdef TESTING
  Serial.print("Yaw:");
  Serial.print(yaw);
  Serial.print("   Pitch:");
  Serial.print(hatire.gyro[1]);
  Serial.print("   Roll: ");
  Serial.print(hatire.gyro[2]);
  Serial.print("\n");
#else
  Serial.write((byte*)&hatire,30);
#endif
  hatire.Cpt++;
  if (hatire.Cpt>999) {
    hatire.Cpt=0;
  }  
  delay(50); // Maybe not needed
}
