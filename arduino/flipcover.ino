/*
What: FlipCap - PC controlled dust cover using the 
  Alnitak (Flip-Flat/Flat-Man) command set found here:
  https://www.optecinc.com/astronomy/catalog/alnitak/resources/GenericCommandsR3.pdf (updated 2024)

Who: 
 Implemented By: Gord Tulloch gord.tulloch@gmail.com 
     (based on Jared Wellman - jared@mainsequencesoftware.com)

When: 
  Last modified:  20240315


Typical usage on the command prompt (ii is device number):

Currently tested
Command     Send      Receive            Desc
Ping     :  >P000CR   *Pii000CR          Used to find device
Open     :  >O000CR   *Oii000CR          Open cover (FF only)
Close    :  >C000CR   *Cii000CR          Close cover(FF only) 
State    :  >S000CR   *SiiqrsCR          Get device status 
Version  :  >V000CR   *ViivvvCR          Get firmware version

Not currently tested:
Light on : >L000CR    *Lii000CR          Turn on light 
Light off: >D000CR    *Dii000CR          Turn off light 
Brightness: >BxxxCR   *BiixxxCR          Set brightness (xxx = 000-255) 
Brightness: >J000CR   *JiixxxCR          Get brightness from device 

Where: 
> is the leading character sent to the device 
CR is carriage return 
xxx is a three digit integer ranging from 0-255 
* is the leading character returned from the device 

ii is a two digit product id, 99 for Flip-Flat, 98 for Flip-Cover, 19 for Flat-Man, 10 for XL, 15 Flat-Man_L. 
qrs is device status where: 
s = 0 cover not open/closed 
s = 1 cover closed 
s = 2 cover open 
s = 3 timed out (open/closed not detected) 
r = 0 light off 
r = 1 light on 
q = 0 motor stopped 
q = 1 motor running 
vvv is firmware version
*/

#include <Servo.h>
Servo myservo;

volatile int ledPin   = 6;      // the pin that the LED is attached to, needs to be a PWM pin.
volatile int servoPin = 9;      // the pin that the servo signal is attached to, needs to be a PWM pin.
int brightness = 0;

enum devices
{
  FLAT_MAN_L = 10,
  FLAT_MAN_XL = 15,
  FLAT_MAN = 19,
  FLIP_FLAT = 99,
  FLIP_CVR = 98
};

enum motorStatuses
{
  STOPPED = 0,
  RUNNING
};

enum lightStatuses
{
  OFF = 0,
  ON
};

enum shutterStatuses
{
  UNKNOWN = 0, // ie not open or closed...could be moving
  CLOSED,
  OPEN
};

int deviceId = FLIP_CVR;
int motorStatus = STOPPED;
int lightStatus = OFF;
int coverStatus = UNKNOWN;
int DEBUG = 1;

void setup()
{
  // initialize the serial communication:
  Serial.begin(9600);
  // initialize the ledPin as an output:
  pinMode(ledPin, OUTPUT);
  analogWrite(ledPin, 0);
  myservo.attach(servoPin);
}

void loop() 
{
  handleSerial();
}


void handleSerial()
{
  if( Serial.available() >= 5 )  // all incoming communications are fixed length at 6 bytes including the \n
  {
    char* cmd;
  char* data;
    char temp[10];
    
    int len = 0;

    char str[20];
    memset(str, 0, 20);
    
  // I don't personally like using the \n as a command character for reading.  
  // but that's how the command set is.
    Serial.readBytesUntil('\n', str, 20);

  cmd = str + 1;
  data = str + 2;
  
  // useful for debugging to make sure your commands came through and are parsed correctly.
    if( DEBUG )
    {
      sprintf( temp, "cmd = >%s%s;", cmd, data);
      Serial.println(temp);
    } 
    
    switch( *cmd )
    {
    /*
    Ping device
      Request: >P000\n
      Return : *Pii000\n
        id = deviceId
    */
      case 'P':
      sprintf(temp, "*P%d000\n", deviceId);
      Serial.print(temp);
      break;

      /*
    Open shutter
      Request: >O000\n
      Return : *Oii000\n
        id = deviceId
    */
      case 'O':
      sprintf(temp, "*O%d000\n", deviceId);
      SetShutter(OPEN);
      Serial.print(temp);
      break;


      /*
    Close shutter
      Request: >C000\n
      Return : *Cii000\n
        id = deviceId

      */
      case 'C':
      sprintf(temp, "*C%d000\n", deviceId);
      SetShutter(CLOSED);
      Serial.print(temp);
      break;

    /*
    Turn light on
      Request: >L000\n
      Return : *Lii000\n
        id = deviceId
    */
      case 'L':
      sprintf(temp, "*L%d000\n", deviceId);
      Serial.print(temp);
      lightStatus = ON;
      analogWrite(ledPin, brightness);
      break;

    /*
    Turn light off
      Request: >D000\n
      Return : *Dii000\n
        id = deviceId
    */
      case 'D':
      sprintf(temp, "*D%d000\n", deviceId);
      Serial.print(temp);
      lightStatus = OFF;
      analogWrite(ledPin, 0);
      break;

    /*
    Set brightness
      Request: >Bxxx\n
        xxx = brightness value from 000-255
      Return : *Biiyyy\n
        id = deviceId
        yyy = value that brightness was set from 000-255
    */
      case 'B':
      brightness = atoi(data);    
      if( lightStatus == ON ) 
        analogWrite(ledPin, brightness);   
      sprintf( temp, "*B%d%03d\n", deviceId, brightness );
      Serial.print(temp);
        break;

    /*
    Get brightness
      Request: >J000\n
      Return : *Jiiyyy\n
        id = deviceId
        yyy = current brightness value from 000-255
    */
      case 'J':
        sprintf( temp, "*J%d%03d\n", deviceId, brightness);
        Serial.print(temp);
        break;
      
    /*
    Get device status:
      Request: >S000\n
      Return : *SidMLC\n
        id = deviceId
        M  = motor status( 0 stopped, 1 running)
        L  = light status( 0 off, 1 on)
        C  = Cover Status( 0 moving, 1 closed, 2 open)
    */
      case 'S': 
        sprintf( temp, "*S%d%d%d%d\n",deviceId, motorStatus, lightStatus, coverStatus);
        Serial.print(temp);
        break;

    /*
    Get firmware version
      Request: >V000\n
      Return : *Vii001\n
        id = deviceId
    */
      case 'V': // get firmware version
      sprintf(temp, "*V%d001\n", deviceId);
      Serial.print(temp);
      break;
    }    

  while( Serial.available() > 0 )
    Serial.read();

  }
}

void SetShutter(int val)
{
  if( val == OPEN && coverStatus != OPEN )
  {
    for (int angle = 0; angle <= 30; angle+=1) 
    {
      myservo.write (angle);
      delay (70);
    } 
    myservo.write (150);
    for (int angle = 150; angle <= 180; angle+=1) 
    {
      myservo.write (angle);
      delay (70);
    } 
    coverStatus = OPEN;
    // TODO: Implement code to OPEN the shutter.
  }
  else if( val == CLOSED && coverStatus != CLOSED )
  {
    for (int angle = 180; angle > 150; angle-=1) 
    {
      myservo.write (angle);
      delay (70);
    }   
    myservo.write (30);  
    for (int angle = 30; angle > 0; angle-=1) 
    {
      myservo.write (angle);
      delay (70);
    } 
    coverStatus = CLOSED;
    // TODO: Implement code to CLOSE the shutter
  }
  else
  {
    // TODO: Actually handle this case
    coverStatus = val;
  }
  
}
