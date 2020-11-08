#include <Arduino.h>
#include <SoftwareSerial.h>
#include <dht.h>

#define light_outside 3
#define light_inside 4


// Global variables
boolean newMessage = false;
float min_temp_inside_value = 20;
float max_temp_inside_value = 24;
float current_temperature_inside = 0;
float current_temperature_outside = 0;


// Working with memory
const byte numChars = 20;
char message[numChars];
char tempChars[numChars];

const char command_min_temperature_inside[9] = "MIN TEMP";
const char command_max_temperature_inside[9] = "MAX TEMP";


// Commands
const char command_info[5] = "INFO";


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


SoftwareSerial SIM900(7, 8);
dht temperature_sensor;

void setup()
{
  Serial.begin(9600);
  SIM900.begin(9600);

  GsmConnected();
  delay(1000);
}

void loop()
{
  SetCurrentTemperature();
  ReadMessage();
  Serial.println(message);

  if (!newMessage)
    ;
  else if (IfSetTemperatureInsideMessage())
    ;
  else
    SendMessage("Unknown command");

  newMessage = false;

  ValidateTemperatureInside();

  Serial.println("working");
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
          Serial.println("Warning: message to long!");
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
