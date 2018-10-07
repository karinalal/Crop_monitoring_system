//Included libraries
#include "dht.h" 
#include <ESP8266WiFi.h>
#include "ThingSpeak.h"
#include<Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <SoftwareSerial.h>
//SoftwareSerial mySerial(3,1);  //(Rx,Tx)
//char msg;

// OLED display TWI address
#define OLED_ADDR   0x3C
Adafruit_SSD1306 display(-1);

#if (SSD1306_LCDHEIGHT != 64)  //128*64 resolution
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

//Wifi connection
const char* ssid     = "Anupriya";
const char* password = "anusingh";
unsigned long myChannelNumber = 520428;
const char * myWriteAPIKey = "VEHYSNWFM74B2F3S";
const char * myReadAPIKey = "1Y6NE3E4X9ZOWHHK";

//Pins for different sensors
#define AOUTpin A0 //the AOUT pin of the CO sensor goes into analog pin A0 of the arduino
#define smokeA0  A0 //0
int buzzer = 16; // D0 Negative terminal of buzzer on 16
int DHT11_PIN =D4;
int AirQualitypin =D5;   // 135 Digital
int SoilMoistPin= D3;   //D5  Digital
int trigPin= D7;  //D7
int echoPin= D8; //D6
int selectline= 10 ;  //S3
int motor = D6;  //S2

//Variables for storing the data read
int COlimit;
int COValue;
int smokeThresh = 400;
long duration;
int sensorValue;
int waterLevel;
int airQuality;
int soilMoist;
int smokeAmt;
String motor_status="initial";
int x = 10;  // x coordinate of OLED
int y = 0;   // y coordinate of OLED
String str = "NULL";

WiFiClient WiFiclient;    // Object of WiFiClient
dht DHT11;    //Object of dht

void temp();
void humid();
void air_quality();
void soil_moisture();
void water_level();
void carbon_monoxide();
void smoke();
void SendMessage();
void setup(){
  //WiFi setup
  Serial.begin(9600); 
  delay(10);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);  
  
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  ThingSpeak.begin(WiFiclient);
  delay(1000);

  //OLED setup
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.setCursor(x,y);
  display.display();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.print("Not in range:");
  display.display(); 
  
  //pinModes 
//  pinMode(AirQualitypin,INPUT); 
  pinMode(SoilMoistPin,INPUT);
  pinMode(trigPin,OUTPUT);
  pinMode(echoPin,INPUT);
  pinMode(AOUTpin,INPUT);
  //pinMode(DOUTpin,INPUT);//sets the pin as an input to the arduino
  pinMode(buzzer,OUTPUT);
  pinMode(smokeA0,INPUT);
  pinMode(motor, OUTPUT);
  digitalWrite(motor, LOW);
}
 
void loop()
{
  temp();
  delay(1000);
  humid();
  delay(1000);
  air_quality();
  delay(1000);
  soil_moisture();
  delay(1000);
  water_level();
  delay(1000);
  digitalWrite(selectline,HIGH);
  carbon_monoxide();
  digitalWrite(selectline,LOW);
  smoke();
  delay(1000);
  
  // Motor Status
  motor_status= ThingSpeak.readFloatField(myChannelNumber, 8, myReadAPIKey);
  Serial.println("Motor_status: "+motor_status);
  if(motor_status=="0.00")
  {
    digitalWrite(motor,LOW);
    Serial.println("Motor_status: "+motor_status);
  }
  else if(motor_status=="1.00")
  {
    digitalWrite(motor,HIGH);
    Serial.println("Motor_status"+motor_status);
  }
  ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
   
   if(str.length()!=0)
      SendMessage();
  //OLED display
  y=0;
  display.clearDisplay();
  display.setCursor(x,y);
  display.print("Not in range:");
  display.display();
  delay(5000);
}

//Air Temperature
void temp()
{ 
  int chk = DHT11.read11(DHT11_PIN);
  if(chk==0){
    Serial.print("Temp: ");
    Serial.print((String)DHT11.temperature);         // Printing the temperature on display.
    Serial.println("*C");     // Printing “ *C ”  on display.
    ThingSpeak.setField(1, (int)DHT11.temperature);
    if(DHT11.temperature < 15 || DHT11.temperature > 40)
    {
      Serial.println("Not the OPTIMUM TEMPERATURE.");
      y = y + 10;
      display.setCursor(x,y);
      display.print("Air Temp");
      display.display();
    }
  }
  else{
    Serial.println("Error");
  }  
}

//Humidity
void humid()
{
  int chk = DHT11.read11(DHT11_PIN);
  if(chk==0)
  {  
    Serial.print("Relative Humidity: ");
    Serial.print((String)DHT11.humidity);     // Printing the humidity on display
    Serial.println("%");     // Printing “%” on display
    ThingSpeak.setField(2,(int)DHT11.humidity);
    if(DHT11.humidity < 10 || DHT11.humidity > 70)
    {
       Serial.println("Not the OPTIMUM HUMIDITY.");
       y = y + 10;
       display.setCursor(x,y);
       display.print("Humidity");
       display.display();
    }
  }  
  else{
    Serial.println("Error");
  }  
}

//MQ135
void air_quality()
{
  airQuality = analogRead(AirQualitypin);//prior it was analog
  Serial.print("Air Quality = ");
  Serial.print(airQuality);
  Serial.println("PPM");
  ThingSpeak.setField(3, airQuality);

  if(airQuality < 100)
  {
    Serial.println("Suitable for crop growth.");
  }
  else if(airQuality>101 && airQuality < 200)
  {
    Serial.println("Unhealthy for crop growth.");
    y = y + 10;
    display.setCursor(x,y);
    display.print("Air Quality");
    display.display();
  }
  else
  {
    Serial.println("Hazardous for crop growth.");
    y = y + 10;
    display.setCursor(x,y);
    display.print("Air Quality");
    display.display();
  }
}

//Soil moisture 
void soil_moisture()
{
  soilMoist = analogRead(SoilMoistPin);
  Serial.print("Soil Moisture: ");
  Serial.print(soilMoist);
  Serial.println("%");
  ThingSpeak.setField(4,soilMoist);

  if(soilMoist < 15)
  {
    Serial.println("Soil moisture too low");
    digitalWrite(motor,HIGH);
    y = y + 10;
    display.setCursor(x,y);
    display.print("Soil Moisture");
    display.display();
  }
  else if(soilMoist>16 && soilMoist < 60)
  {
    Serial.println("Soil moisture in range");  
    digitalWrite(motor,LOW);
  }
  else
  {
    Serial.println("Soil moisture too high");
    y = y + 10;
    display.setCursor(x,y);
    display.print("Soil Moisture");
    display.display();
  }
}

//Ultrasonic sensor
void water_level()
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); 
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  sensorValue = (duration * 0.034)/2;
  Serial.println(sensorValue);
  waterLevel = 10 - sensorValue;
  
  Serial.print("Water level: ");
  Serial.print(waterLevel);
  Serial.println("cm");
  ThingSpeak.setField(5, waterLevel);
  
  if(waterLevel > 3)
  {
    Serial.println("High Water Level");
    digitalWrite(motor,LOW);
    y = y + 10;
    display.setCursor(x,y);
    display.print("Water Level");
    display.display();
  }
  else if(waterLevel >=0 && waterLevel <=3)
  {
    Serial.println("Water level in range");
    digitalWrite(motor,HIGH);
  }
}

//MQ7
void carbon_monoxide()
{
  COValue = analogRead(AOUTpin);//reads the analaog value from the CO sensor's AOUT pin
//  COlimit = digitalRead(DOUTpin);//reads the digital value from the CO sensor's DOUT pin
  Serial.print("CO value: ");
  Serial.println(COValue);//prints the CO value
  //Serial.print("Limit: ");
  //Serial.println(COlimit);//prints the limit reached as either LOW or HIGH (above or underneath)
  ThingSpeak.setField(6, COValue);
  ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
   
  if(COValue>300)
  {
    digitalWrite(buzzer,LOW);
    Serial.println("CO level high");
    y = y + 10;
    display.setCursor(x,y);
    display.print("CO");
    display.display();
  }
  else
  {
    digitalWrite(buzzer,HIGH);
    Serial.println("CO level OK");
  }
}

//MQ2
void smoke()
{
  smokeAmt = analogRead(smokeA0);
  Serial.print("Smoke: ");
  Serial.println(smokeAmt);
  ThingSpeak.setField(7, smokeAmt);
  ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

  if (smokeAmt > smokeThresh)
  {
    Serial.println("Smoke level high");
    digitalWrite(buzzer,HIGH);
    y = y + 10;
    display.setCursor(x,y);
    display.print("Smoke");
    display.display();
  }
  else
  {
    digitalWrite(buzzer,LOW);
  }
}
void SendMessage()
{
  
  Serial.println("send msg");
  Serial.println("AT+CMGF=1");    //Sets the GSM Module in Text Mode
  delay(1000);  // Delay of 1000 milli seconds or 1 second
  Serial.println("AT+CMGS=\"+917838433503\"\r"); // Replace x with mobile number
  delay(1000);
  Serial.println("ALERT:  "+str);// The SMS text you want to send
  delay(100);
   Serial.println((char)26);// ASCII code of CTRL+Z
  delay(1000);
  Serial.println("msg sent");
  
}

