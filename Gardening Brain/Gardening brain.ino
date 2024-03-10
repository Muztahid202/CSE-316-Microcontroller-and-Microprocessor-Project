#include<SoftwareSerial.h>
// #include <Wire.h>
#include "DHT.h"
#include <LiquidCrystal_I2C.h>
#include <Stepper.h>

SoftwareSerial B(10,11);    //10-RX,  11-TX

#define DHTPIN 6            // pin 6 for DHT11
#define DHTTYPE DHT11      // DHT 11

bool showTemp = false;

String readString;
DHT dht(DHTPIN, DHTTYPE);
float prevTemp = 0.0;
float prevHumidity = 0.0;


//LCD for lcd A4 and A5 pin is used
LiquidCrystal_I2C lcd(0x27, 16, 2);
bool sprayMotorOn = false;

unsigned long prevTimeForPesticide = 0;
unsigned long prevTimeForFertilizer = 0;
const long intervalHighForPesticide = 120000;
const long intervalHighForFertilizer = 60000;

const long intervalLowForPesticide = 3000;
const long intervalLowForFertilizer = 3000;

bool pesticidePumpOn = false;
bool fertilizerPumpOn = false;
bool watermotorOn = false;
bool bluetoothShade = false;
const int moistureThreshold = 1024 / 2; 
const int moisturePinIn = A0;

bool showRain = false;
bool heavyRain = false;
#define STEPS 200 
int step_mode = 8;

Stepper stepper1(STEPS, 7, 9 , 8, 12);
Stepper stepper2(STEPS, 7, 9 , 8, 12);
bool shadeOn=false;


const int rainSensor = A1;
const int mildRainThreshold = 1024*2/3;
const int heavyRainThreshold = 1024/3;


void setup() 
{

pinMode(2, OUTPUT);//spray motor
pinMode(3, OUTPUT);//pesticide pump
pinMode(4, OUTPUT);//fertilizer pump
pinMode(5, OUTPUT); //moisture pump
B.begin(9600);
dht.begin();
Serial.begin(9600);

lcd.begin();
lcd.backlight();
}

void lcdshow(String str)
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(str);
  delay(2000);
  lcd.clear();
}


void loop() 
{
  // delay(2000);

  // Read humidity
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Read moisture
  int moistureValue = analogRead(moisturePinIn);
  int moisturePercentage = map(moistureValue, 0, 970, 100, 0);
  
  //serial print 
  readString = B.read();

  //show the data
  if (isnan(h) || isnan(t)) {

    lcd.clear();
    delay(100);
    lcd.setCursor(0,0);
    lcd.print("Temp: ");
    lcd.print(prevTemp);
    lcd.print(" C");
    lcd.setCursor(0,1);
    lcd.print("Humi: ");
   
    lcd.print(prevHumidity);
    lcd.print(" %");
    Serial.println("Failed to read from DHT sensor!");

    // Attempt to reinitialize the DHT sensor
    dht.begin();
  } else {
    prevHumidity= h;
    prevTemp = t;

    lcd.clear();
    delay(100);
    lcd.setCursor(0,0);
    lcd.print("TEMP: ");
    lcd.print(t);
    lcd.print(" C");
    lcd.setCursor(0,1);
    lcd.print("HUMI: ");
    lcd.print(h);
    lcd.print(" %");
    delay(2000);
    lcdshow("Soil Moist: " + String(moisturePercentage) + " %");
    


  }
  //If user enters "I" command in bluetooth terminal then temperature.humidity info and soil moisutre will be sent to the user 
  if(readString== "73" && showTemp == false)
  {

    if (isnan(h) || isnan(t)) {
      B.print("Temparature: ");
      B.print(prevTemp);
      B.print(" C");
      B.print(";");

      B.print("Humidity: ");
      B.print(prevHumidity);
      B.print(" %");
      B.print(";");
    }
    else
    {
      B.print("Temparature: ");
      B.print(t);
      B.print(" C");
      B.print(";");

      B.print("Humidity: ");
      B.print(h);
      B.print(" %");
      B.print(";");
    }
    showTemp = true;
    B.print("Soil Moisture: ");
    B.print(moisturePercentage);
    B.print(";");

  }

  //If user enters "U" command then the user will get the status of all the pumps in present condition
  if(readString == "85")
  {
    //show motor status of all motors
    B.print("MOTOR STATUS: \n");
    if(sprayMotorOn == true)
    {
      B.print("SPRAYMOTOR ON");
      B.print("\n");
    }
    else
    {
      B.print("SPRAYMOTOR OFF");
      B.print("\n");
    }
    if(pesticidePumpOn == true)
    {
      B.print("PESTICIDE ON");
      B.print("\n");
    }
    else
    {
      B.print("PESTICIDE OFF");
      B.print("\n");
    }
    if(fertilizerPumpOn == true)
    {
      B.print("FERTILIZER ON");
      B.print("\n");
    }
    else
    {
      B.print("FERTILIZER OFF");
      B.print("\n");
    }
    if(watermotorOn == true)
    {
      B.print("SOILWATER ON");
      B.print(";");
    }
    else
    {
      B.print("SOILWATER OFF");
      B.print(";");
    }
  }

//If user enters "A" command then the water spray motor will be on 
  if(readString == "65" && sprayMotorOn == false)
  {
    sprayMotorOn = true;
    digitalWrite(2, HIGH);
    delay(1000);
    lcdshow("SPRAYMOTOR ON");
    B.print("SPRAYMOTOR ON");
    B.print(";");

  }

//If user enters "B" command then the water spray motor will be turned off
  if(readString == "66" && sprayMotorOn == true)
  {
    sprayMotorOn = false;
    digitalWrite(2, LOW);
    delay(1000);
    lcdshow("SPRAYMOTOR OFF");
    B.print("SPRAYMOTOR OFF");
    B.print(";");
  }


  // set the time for pesticide pump and fertilizer pump
  unsigned long currentTime = millis();

  if(pesticidePumpOn == false && currentTime - prevTimeForPesticide >= intervalHighForPesticide)
  {
    prevTimeForPesticide = currentTime;
    pesticidePumpOn = true;
    digitalWrite(3, HIGH);
    delay(1000);
    lcdshow("PESTICIDE ON");
    B.print("Routine Update: \nPESTICIDE ON");
    B.print(";");
  }
  else if (pesticidePumpOn == true && currentTime - prevTimeForPesticide >= intervalLowForPesticide)
  {
    prevTimeForPesticide = currentTime;
    pesticidePumpOn = false;
    digitalWrite(3, LOW);
    delay(1000);
    lcdshow("PESTICIDE OFF");
    B.print("PESTICIDE OFF");
    B.print(";");
    
  }

  if(fertilizerPumpOn == false && currentTime - prevTimeForFertilizer >= intervalHighForFertilizer)
  {
    prevTimeForFertilizer = currentTime;
    fertilizerPumpOn = true;
    B.print("Routine Update: \n FERTILIZER");
    B.print(" ON");
    B.print(";");
    digitalWrite(4, HIGH);
    delay(1000);
    lcdshow("FERTILIZER ON");
  }
  else if (fertilizerPumpOn == true && currentTime - prevTimeForFertilizer >= intervalLowForFertilizer)
  {
    prevTimeForFertilizer = currentTime;
    fertilizerPumpOn = false;
    B.print("FERTILIZER OFF");
    B.print(";");
    digitalWrite(4, LOW);
    delay(500);
    lcdshow("FERTILIZER OFF");
  }
  


  //Controlling the soil water motor by comparing the actual soil moisture value with the threshold value of optimum moisture and based on it soil water motor will be controlled
  if(moistureValue > moistureThreshold && watermotorOn == false)
  {
    watermotorOn = true;
    B.print(";");
    lcdshow("SOILWATER ON");
    B.print("SOILWATER ON");
    B.print(";");
    digitalWrite(5, HIGH);
    delay(100);
  }
  else if(moistureValue < moistureThreshold && watermotorOn == true)
  {
    watermotorOn = false;
    digitalWrite(5, LOW);
    delay(100);
    B.print("SOILWATER OFF");
    B.print(";");
    lcdshow("SOILWATER OFF");
    
  }

//If temp becomes greater than 30 then shade will be turned on and if temp becomes less than 30 then shade will be turned off
   if(t>30 && shadeOn == false)//t beshi, bondho tai samne agabe
  {
    lcdshow("Temp High");
    lcdshow("SHADE LOADING...");
    B.print("Temp High! Not safe for your plant");

    stepper1.setSpeed(180); // 1 rpm
    stepper1.step(28500);
    Serial.println("if");
    lcdshow("SHADE ON");
    shadeOn=true;
  } 
  
  else if(t<30 && shadeOn==true)// t kom, khola tai back krbe
  {

    //show values
    lcdshow("Temp OK now");
    lcdshow("SHADE Closing...");
    B.print("Temp has been decreased");
    B.print("SHADE Closing...");
    B.print(";");
    stepper2.setSpeed(180); 
    stepper2.step(-28500);
    lcdshow("SHADE OFF");
    shadeOn=false;
  }


//rain condition is being monitored and if heavy rain starts then a precautionary message will be sent to the user to turn on the shade and if rain stops then a message will be sent to the user that rain has stopped
  int rainValue = analogRead(rainSensor);
    if(rainValue > mildRainThreshold && showRain == false && heavyRain == true)
    {
      showRain = true;
      lcd.print("No Rain");
      delay(200);
      B.print("Weather Update: Rain stopped;");
      heavyRain = false;
    }
    else if(rainValue < mildRainThreshold && rainValue > heavyRainThreshold)
    {
      lcd.print("Moderate Rain");
      delay(200);
      if(heavyRain == true)
      {
        heavyRain = false;
        B.print("Heavy Rain Stopped Rain is now Moderate");
       B.print("Weather Update: Moderate Rain. Weather is good for your plant! ;");
      }
      // 
    }
    else if( rainValue< heavyRainThreshold && heavyRain == false)
    {
      lcdshow("Heavy Rain");
      delay(200);
      B.print("Heavy Rain is detected. \n You should turn shade on by pressing S");
      B.print(";");
      heavyRain = true;
    }

//If User enters "S" command then the shade will be turned on and if the shade is already on then it will be turned off
  if(readString == "83" )
  {
    if(bluetoothShade == false)
    {

      B.print("Shade turning ON");
      B.print(";");
      stepper1.setSpeed(180); // 1 rpm
      stepper1.step(28500);
      Serial.println("if");
      lcdshow("SHADE ON");
      B.print("Shade is now ON!;");
      bluetoothShade = true;
    }
    else
    {
      B.print("shade turning OFF;");
      stepper1.setSpeed(180); // 1 rpm
      stepper1.step(-28500);
      Serial.println("if");
      lcdshow("SHADE OFF");
      B.print("Shade is now OFF;");
      bluetoothShade = false;
    }
  }

//IF user enters "R" command then status of present rain condition is being sent to the user
  if(readString == "82" )
  {
    
    if(rainValue > mildRainThreshold)
    {
      B.print("Weather Update: No Rain Detected;");
    }
    else if(rainValue < mildRainThreshold && rainValue > heavyRainThreshold)
    {
     
      B.print("Weather Update: Moderate Rain. Weather is good for your plant! ;");
    }
    else
    {
      B.print("Weather Update: Heavy Rain is detected;");
    }
  }

  delay(300); 
}
