#include <Arduino.h>
#include <SoftwareSerial.h>
#include <dht.h>


#define water_level_detector A0

#define tv 2
#define light_outside 3
#define light_inside 4
#define gate 5
#define temperature_inside 12
#define temperature_outside 13


// Global variables
unsigned long delayUntilMillis = millis();
boolean newMessage = false;
float min_temp_inside_value = 20;
float max_temp_inside_value = 24;
float current_temperature_inside = 0;
float current_temperature_outside = 0;
bool current_light_outside = false;
bool current_light_inside = false;
bool current_tv = false;
bool current_gate = false;
int max_water_level_value = 300;

// Working with memory
const byte numChars = 20;
char message[numChars];
char tempChars[numChars];


// Commands
const char command_light_outside[14] = "LIGHT OUTSIDE";
const char command_light_inside[13] = "LIGHT INSIDE";
const char command_min_temperature_inside[9] = "MIN TEMP";
const char command_max_temperature_inside[9] = "MAX TEMP";
const char command_tv[3] = "TV";
const char command_gate[7] = "GATE";


// Functions
void GsmConnected();
void ReadMessage();
void SendMessage(const char text[]);
float GetNumberFromMessage();
void UpdateSIM900Serial();
void UpdateSerial();
void DelAllSMS();

void SetCurrentTemperature();
void ValidateTemperatureInside();
bool IfSetTemperatureInsideMessage();

bool IfOnOffMessage(const char device[], const byte &device_pin, bool &pointer_to_current_value);
bool StrContains(char *str, const char *sfind);

SoftwareSerial SIM900(7, 8);
dht temperature_sensor;

void setup()
{
  Serial.begin(9600);
  SIM900.begin(9600);

  pinMode(light_inside, OUTPUT);
  pinMode(light_outside, OUTPUT);
  pinMode(tv, OUTPUT);
  pinMode(gate, OUTPUT);
  
  GsmConnected();
  delay(1000);
}

void loop()
{
  if (analogRead(water_level_detector) > max_water_level_value)
    SendMessage("Water detected");


  SetCurrentTemperature();
  ReadMessage();
  Serial.println(message);

  if (!newMessage)
    ;
  else if (IfSetTemperatureInsideMessage())
    ;
  else if (IfOnOffMessage(command_light_outside, light_outside, current_light_outside))
    ;
  else if (IfOnOffMessage(command_light_inside, light_inside, current_light_inside))
    ;
  else if (IfOnOffMessage(command_tv, tv, current_tv))
    ;
  else if (IfOnOffMessage(command_gate, gate, current_gate))
    ;
  else
    SendMessage("Unknown command");

  newMessage = false;

  ValidateTemperatureInside();

  Serial.println("Working");
  UpdateSerial();
  delay(1000);
}


//----------------------------------------------
// Base commands
//----------------------------------------------

void SetCurrentTemperature()
{
  temperature_sensor.read11(temperature_inside);
  current_temperature_inside = temperature_sensor.temperature;

  temperature_sensor.read11(temperature_outside);
  current_temperature_outside = temperature_sensor.temperature;
}

void ValidateTemperatureInside()
{
  unsigned long currentMillis = millis();

  if (currentMillis < delayUntilMillis)
    return;

  if (current_temperature_inside > max_temp_inside_value)
    digitalWrite(actuator, LOW);

  else if (current_temperature_inside < min_temp_inside_value)
    digitalWrite(actuator, HIGH);

  delayUntilMillis = currentMillis + 5000;
}

bool IfSetTemperatureInsideMessage()
{
  if (StrContains(message, command_min_temperature_inside))
    min_temp_inside_value = GetNumberFromMessage();
  else if (StrContains(message, command_max_temperature_inside))
    max_temp_inside_value = GetNumberFromMessage();
  else
    return false;

  SendMessage(message);
  return true;
}

//----------------------------------------------
// Analyzing message
//----------------------------------------------

float GetNumberFromMessage()
{
  strcpy(tempChars, message);
  char *token = strtok(tempChars, " ");

  while (token != NULL)
  {
    if (isdigit(token[0]))
      return atof(token);

    token = strtok(NULL, " ");
  }

  return 0;
}

bool IfOnOffMessage(const char device[], const byte &device_pin, bool &pointer_to_current_value)
{
  if (!StrContains(message, device)) // not found
    return false;

  if (StrContains(message, "ON"))
  {
    digitalWrite(device_pin, HIGH);
    SendMessage(message);
    pointer_to_current_value = true;
    return true;
  }
  else if (StrContains(message, "OFF"))
  {
    digitalWrite(device_pin, LOW);
    SendMessage(message);
    pointer_to_current_value = false;
    return true;
  }
  else
  {
    SendMessage("Message is not recognized!");
    return true;
  }
}


//----------------------------------------------
// Utils functions
//----------------------------------------------

bool StrContains(char *str, const char *sfind)
{
  unsigned int found = 0;
  unsigned int index = 0;

  while (index < strlen(str))
  {
    if (str[index] == sfind[found])
    {
      found++;
      if (strlen(sfind) == found)
        return true;
    }
    else
      found = 0;
    index++;
  }

  return false;
}


//----------------------------------------------
// Gsm commands
//----------------------------------------------

void GsmConnected()
{
  SIM900.println("AT+CMGF=1\r"); // Configuring TEXT mode
  delay(1000);
  SIM900.println("AT+CNMI=2,2,0,0,0\r"); // Decides how newly arrived SMS messages should be handled
  delay(1000);
  UpdateSIM900Serial();
  DelAllSMS();
}

// Read message to global buffer and set bool if new message exist
void ReadMessage()
{
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char startMarker = '#';
  char endMarker = '\n';
  char rc;

  while (SIM900.available() > 0 && newMessage == false)
  {
    rc = SIM900.read();

    if (recvInProgress == true)
    {
      if (rc != endMarker)
      {
        message[ndx] = rc;
        ndx++;
        if (ndx >= numChars)
        {
          Serial.println("Message to long!");
          ndx = numChars - 1;
        }
      }
      else
      {
        message[ndx] = '\0'; // terminate the string
        recvInProgress = false;
        ndx = 0;
        newMessage = true;
      }
    }
    else if (rc == startMarker)
      recvInProgress = true;
  }

  DelAllSMS();
}

void SendMessage(const char text[])
{
  // Change to your number
  SIM900.println("AT+CMGS=\"+37060000000\"\r");
  delay(1000);
  SIM900.print(text);
  delay(100);
  SIM900.write(26);
  delay(1000);
  UpdateSIM900Serial();
}

void DelAllSMS()
{
  // Serial.println("AT+CMGD=1,4");  //Delete all SMS in box
  SIM900.print("AT+CMGDA=\"");
  SIM900.println("DEL ALL\"");
  delay(500);
  Serial.println("All Messages Deleted");
}


//----------------------------------------------
// Memory management
//----------------------------------------------

void UpdateSIM900Serial()
{
  delay(1000);
  while (SIM900.available())
    SIM900.read();
  delay(1000);
}

void UpdateSerial()
{
  delay(1000);
  while (Serial.available())
    Serial.read();
  delay(1000);
}
