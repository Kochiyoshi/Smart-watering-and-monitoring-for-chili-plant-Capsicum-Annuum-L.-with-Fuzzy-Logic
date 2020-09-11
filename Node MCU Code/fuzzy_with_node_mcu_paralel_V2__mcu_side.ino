//***********************************************************************
// Author : Arief Maulana                  
// NIM    : 1301144158                                             
// Email  : ariefmaulana@students.telkomuniversity.ac.id
//          arief.thehunter34@gmail.com                      
//***********************************************************************

#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include "ThingSpeak.h"
#include "secrets.h"
#include <ESP8266WiFi.h>

char ssid[] = SECRET_SSID;   // your network SSID (name) 
char pass[] = SECRET_PASS;   // your network password
int keyIndex = 0;            // your network key Index number (needed only for WEP)
WiFiClient  client;

unsigned long myChannelNumber = SECRET_CH_ID;
const char * myWriteAPIKey = SECRET_WRITE_APIKEY;
String myStatus = "";

SoftwareSerial s(D6,D5);
 
void setup() {
  // Initialize Serial port
  Serial.begin(115200);
  s.begin(115200);
  while (!Serial) continue;

  // Initialize ThingSpeak
  WiFi.mode(WIFI_STA); 
  ThingSpeak.begin(client);
}
 
void loop() {
  StaticJsonBuffer<1000> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(s);
 
  if (root == JsonObject::invalid())
  {
    return;
  }

  if(WiFi.status() != WL_CONNECTED){
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SECRET_SSID);
    while(WiFi.status() != WL_CONNECTED){
      WiFi.begin(ssid, pass);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      Serial.print(".");
      delay(5000);   
    } 
    Serial.println("\nConnected.");
  }
  
  //Print the data in the serial monitor
  Serial.println("JSON received and parsed");
  root.prettyPrintTo(Serial);
  Serial.println("");
    //Serial.print("Ph: ");
  float data1=root["ph"];
    //Serial.println(data1);
    //Serial.print("Moisture: ");
  int data2=root["mois"];
    //Serial.println(data2);
    //Serial.print("deg C: ");
  float data3=root["c"];
    //Serial.println(data3);
    //Serial.print("Fuzzy: ");
  float data4=root["f"];
    //Serial.println(data4);
    //Serial.println("");
  float data5=root["f_s"];
    //Serial.println(data5);
    //Serial.println("");
    //float data6=root["ph_s"];
    //Serial.println(data6);
    //Serial.println("");

  // set the fields with the values
  ThingSpeak.setField(1, data2);
  ThingSpeak.setField(2, data3);
  ThingSpeak.setField(3, data1);
  ThingSpeak.setField(4, data4);
  
  myStatus = "";
  // figure out the status message
  if(data5 == 1){
    myStatus = String("Plant Watered for 3 Seconds");
    ThingSpeak.setField(5, myStatus);
  }else if(data5 == 2){
    myStatus = String("Plant Watered for 10 Seconds");
    ThingSpeak.setField(5, myStatus);
  }else if(data5 == 3/* && data6 == 3*/){
    //myStatus = "No Action Required -Moisture at "+ String(data2) + "% and -temp at "+ String(data3) +"C";
    myStatus = String("No Action Required");
    ThingSpeak.setField(5, myStatus);
  }

  Serial.println(myStatus);

  // set the status
  ThingSpeak.setStatus(myStatus);
  // write to the ThingSpeak channel
  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if(x == 200){
    Serial.println("Channel update successful.");
  }
  else{
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }
  
  Serial.println("---------------------xxxxx--------------------");
 Serial.println("");
}
