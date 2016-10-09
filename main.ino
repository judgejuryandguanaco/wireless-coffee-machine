/* main.ino
 *
 * Based on ESP8266_Shield_Demo.h by 
 * Jim Lindblom at SparkFun Electronics
 *
 * Controls Arduino, which in turn interfaces with ESP8266 shield
 */

#include <avr/pgmspace.h>

#include <SoftwareSerial.h>
#include <SparkFunESP8266WiFi.h>

/* Wifi network info */
const char mySSID[] = "i'm a tree";
const char myPSK[] = "ezekiel2517";

ESP8266Server server = ESP8266Server(80);

//const char destServer[] = "example.com";
const char htmlHeader1[] PROGMEM = "HTTP/1.1 200 OK\r\n";
const char htmlHeader2[] PROGMEM = "Content-Type: text/html\r\n";
const char htmlHeader3[] PROGMEM = "Connection: close\r\n\r\n";
const char htmlHeader4[] PROGMEM = "<!DOCTYPE HTML>\r\n";
const char htmlHeader5[] PROGMEM = "<html>\r\n";
const char htmlHeader6[] PROGMEM = "<body>\r\n";
const char* const htmlHeaderTable[] PROGMEM = { htmlHeader1, htmlHeader2, htmlHeader3, htmlHeader4, htmlHeader5, htmlHeader6 };
const char htmlHeaderTableSize PROGMEM = 5;

const char webpage1[] PROGMEM = "Temperature: ";
const char webpage2[] PROGMEM = "\r\n";
const char webpage3[] PROGMEM = "<form action=\"/?\" method=\"get\">\r\n";
const char webpage4[] PROGMEM = "    <input name=\"e\" type=\"submit\" value=\"0\">\r\n";
const char webpage5[] PROGMEM = "    <input name=\"e\" type=\"submit\" value=\"1\">\r\n";
const char webpage6[] PROGMEM = "</form>\r\n";
const char* const webpageTable[] PROGMEM = { webpage1, webpage2, webpage3, webpage4, webpage5, webpage6 };
const char webpageTableSize PROGMEM = 5;

const char htmlFooter1[] PROGMEM = "</body>\r\n";
const char htmlFooter2[] PROGMEM = "</html>\r\n";
const char* const htmlFooterTable[] PROGMEM = { htmlFooter1, htmlFooter2 };
const char htmlFooterTableSize PROGMEM = 1;

const unsigned int BUFSIZ = 300;
#define TEMP_PIN 0
#define LED_PIN 2
#define BAUD_RATE 115200

// All functions called from setup() are defined below the
// loop() function. They modularized to make it easier to
// copy/paste into sketches of your own.
void setup() 
{
  // Serial Monitor is used to control the demo and view
  // debug information.
  Serial.begin(115200);
  while(!Serial);  

  // initializeESP8266() verifies communication with the WiFi
  // shield, and sets it up.
  initializeESP8266();

  // connectESP8266() connects to the defined WiFi network.
  connectESP8266();

  // displayConnectInfo prints the Shield's local IP
  // and the network it's connected to.
  displayConnectInfo();

  //serialTrigger(F("Press any key to test server."));
  serverSetup();

  pinMode(LED_PIN, OUTPUT);
  pinMode(TEMP_PIN, INPUT);
}

void loop() 
{
  serverDemo();
}

void initializeESP8266()
{
  char ATVersion[100] = { '\0' };
  char SDKVersion[100] = { '\0' };
  char compileTime[100] = { '\0' };
  int test;
  
  test = esp8266.begin();
  
  if (test != 1)
  {
    Serial.println(F("Error talking to ESP8266."));
    errorLoop(test);
  }
  Serial.println(F("ESP8266 Shield Present"));

  esp8266.setBaud(115200);

  esp8266.getVersion(ATVersion, SDKVersion, compileTime);
  Serial.print("ATVersion: ");
  Serial.println(ATVersion);
  Serial.print("ADKVersion: ");
  Serial.println(SDKVersion);
  Serial.print("Compile time: ");
  Serial.println(compileTime);
}

void connectESP8266()
{
  // The ESP8266 can be set to one of three modes:
  //  1 - ESP8266_MODE_STA - Station only
  //  2 - ESP8266_MODE_AP - Access point only
  //  3 - ESP8266_MODE_STAAP - Station/AP combo
  // Use esp8266.getMode() to check which mode it's in:
  int retVal = esp8266.getMode();
  if (retVal != ESP8266_MODE_STA)
  { // If it's not in station mode.
    // Use esp8266.setMode([mode]) to set it to a specified
    // mode.
    retVal = esp8266.setMode(ESP8266_MODE_STA);
    if (retVal < 0)
    {
      Serial.println(F("Error setting mode."));
      errorLoop(retVal);
    }
  }
  Serial.println(F("Mode set to station"));
  // esp8266.status() indicates the ESP8266's WiFi connect
  // status.
  // A return value of 1 indicates the device is already
  // connected. 0 indicates disconnected. (Negative values
  // equate to communication errors.)
  retVal = esp8266.status();
  if (retVal <= 0)
  {
    Serial.print(F("Connecting to "));
    Serial.println(mySSID);
    // esp8266.connect([ssid], [psk]) connects the ESP8266
    // to a network.
    // On success the connect function returns a value >0
    // On fail, the function will either return:
    //  -1: TIMEOUT - The library has a set 30s timeout
    //  -3: FAIL - Couldn't connect to network.
    retVal = esp8266.connect(mySSID, myPSK);
    if (retVal < 0)
    {
      Serial.println(F("Error connecting"));
      errorLoop(retVal);
    }
  }
}

void displayConnectInfo()
{
  char connectedSSID[24];
  memset(connectedSSID, 0, 24);
  // esp8266.getAP() can be used to check which AP the
  // ESP8266 is connected to. It returns an error code.
  // The connected AP is returned by reference as a parameter.
  int retVal = esp8266.getAP(connectedSSID);
  if (retVal > 0)
  {
    Serial.print(F("Connected to: "));
    Serial.println(connectedSSID);
  }

  // esp8266.localIP returns an IPAddress variable with the
  // ESP8266's current local IP address.
  IPAddress myIP = esp8266.localIP();
  Serial.print(F("My IP: ")); Serial.println(myIP);
}

void serverSetup()
{
  server.begin();
  Serial.print(F("Server started! Go to "));
  Serial.println(esp8266.localIP());
  Serial.println();
}

void serverDemo()
{
  // available() is an ESP8266Server function which will
  // return an ESP8266Client object for printing and reading.
  // available() has one parameter -- a timeout value. This
  // is the number of milliseconds the function waits,
  // checking for a connection.
  ESP8266Client client = server.available(10);
  char clientline[BUFSIZ] = { '\0' };
  int index = 0;

  if (client)
  {
    Serial.println(F("Client Connected!"));
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    client.flush();

    while (client.connected()) 
    {
      Serial.print(esp8266.status());
      if (client.available()) 
      {
        char c = client.read();

        if( index < BUFSIZ ) {
          clientline[index++] = c;
        } // end of if

        if (c == '\n' && currentLineIsBlank) 
        {
          clientPrintProgmemArray(htmlHeaderTable, htmlHeaderTableSize, client);
          clientPrintProgmemArray(webpageTable, webpageTableSize, client, 1, 2, getTemp());
          clientPrintProgmemArray(htmlFooterTable, htmlFooterTableSize, client);
          Serial.println(F("Page sent"));
          break;
        } // end of if

        if (c == '\n') 
        {
          currentLineIsBlank = true;
        } // end of if
        
        else if (c != '\r') 
        {
          currentLineIsBlank = false;
        } // end of if
      } // end of if( client.available() )
    } // end of while ( client.connected() )
    delay(1);
    client.stop();
    Serial.println(F("Client disconnected"));
    Serial.print(F("Index: "));
    Serial.println(index);
    Serial.print(F("Clientline: \""));
    Serial.print(clientline);
    Serial.print(F("\"\r\n"));
    if( strstr(clientline, "e=1") != 0) {
      digitalWrite(LED_PIN, digitalRead(LED_PIN) ^ 1);
    }
  }
}

// errorLoop prints an error code, then loops forever.
void errorLoop(int error)
{
  Serial.print(F("Error: ")); Serial.println(error);
  Serial.println(F("Looping forever."));
  for (;;);
}

int clientPrintProgmemArray(const char* const table[], char tableSize, ESP8266Client client)
{
  char stringBuffer[255];
  char i;
  
  for (i = 0; i < tableSize; i++)
  {
    strcpy_P(stringBuffer, (char*)pgm_read_word(&table[i]));
    client.print(stringBuffer);
  }
  return 0;
}

int clientPrintProgmemArray(const char* const table[], char tableSize, ESP8266Client client, int addTemp, int indexPlace, int temp)
{
  char stringBuffer[255];
  char i;
  
  for (i = 0; i < tableSize; i++)
  {
    if( addTemp == 1 && i == indexPlace ) { 
      client.print(temp); 
    }
    strcpy_P(stringBuffer, (char*)pgm_read_word(&table[i]));
    client.print(stringBuffer);
  }
  return 0;
}

/* Read value of analogue pin the temperature sensor is
 * connected to. 
 */
int getTemp(void) {
  int analog;
  float temp;

  analog = analogRead(TEMP_PIN);
  temp = ( analog * 5  ) / 1024.0;
  temp = (temp - 0.5) * 100;
  
  return (int)temp;
}

void debugString(char* string1, char* string2)
{
  Serial.write('\r\n');
  Serial.write(string1);
  Serial.write(" is: ");
  Serial.write(string2);
  Serial.write('\r\n');
}
