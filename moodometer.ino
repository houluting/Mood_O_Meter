/*
      Mood-o-meter
      www.72u.org
      Author: Luting Hou (www.lutinghou.com)
      2015.12
      version: v0.3
 */
 
#include <Wire.h>
#include <Bridge.h>
#include <Adafruit_DotStar.h>
#include <SPI.h>
#include <FileIO.h>
#include <Console.h>
#include "RTClib.h"

// set the number of buttons: 
const int numButton = 6;
int buttonPin[numButton] = {4, 5, 6, 7, 8, 9}; 
double buttonCount[numButton] = {-1, -1, -1, -1, -1, -1}; //-1 to offset the initial count of each button
int buttonState[numButton]; //current legit button reading
int buttonReading[numButton]; //current pin reading
double buttonCountTotal = -6; //total number of times when buttons pushed; -6 to offset the initial count of each button

// for button debounce
int lastButtonState[numButton] = {LOW, LOW, LOW, LOW, LOW, LOW}; //last legit button reading
long lastDebounceTime[numButton] = {0,0,0,0,0,0};
long debounceDelay = 50;

// for LEDs
uint32_t  color = 0xFFFF00; //blue in DOTSTAR_RGB
const int numPixels = 42;
const int dataPins = 10;
const int clockPins = 11;
Adafruit_DotStar LEDStrips = Adafruit_DotStar( numPixels, dataPins, clockPins, DOTSTAR_RGB);
double hue = 0;
double hues[numButton]= {0.75, 0.5, 0.32, 0, 0.95, 0.67 };//each hue for a mood
uint32_t  colors[numButton] = {};
String mood[numButton]= {"Excited", "Inspired", "Calm", "Tired", "Anxious", "Angry"};

//for SD files
String filetoday = ""; //store the file name of the current day
String currentDate = "";
String prevDate = "";
String errorFile = "RecordsWithRTCIssues";

//for sending emails
Process p;

// for RTC
RTC_DS1307 RTC;

void setup() {

  Wire.begin();
  RTC.begin();
  // Initialize the Bridge and the Console
  Bridge.begin();
  Console.begin();
  FileSystem.begin();
  // wait for Console to connect. Only for testing.Program won't start until console/serial monitor is opened
  //while (!Console); 
  
  for (int i = 0; i < numButton; i++) {
    pinMode(buttonPin[i], INPUT);
    buttonState[i] = 0;
    lastButtonState[i] = LOW;
  }
  
  LEDStrips.begin();

  if (! RTC.isrunning()) {
    Console.println("RTC is NOT running!");
    currentDate = errorFile; 
    // following line sets the RTC to the date & time this sketch was compiled
    //RTC.adjust(DateTime(__DATE__, __TIME__));
  }

  //create the first daily file on SD card
  if (getDate() == "2165165165") {
    currentDate = errorFile;
  }
  else currentDate = getDate();
  filetoday += "/mnt/sd/";
  filetoday += currentDate;
  filetoday += ".txt";
   
  hue = 0.99;

  for (int k = 0; k < numPixels; k++ ) {
          color = hsvToRgbColor( hue, 1.0, 1.0 );        
          LEDStrips.setPixelColor( k, color );
   }
}


void loop() {
  
  // The FileSystem card is mounted at the following "/mnt/FileSystema1" 
  //create a new daily file on SD card when it's a new day
  prevDate = currentDate;
  
  //currentDate = getDate();
  if (getDate() == "2165165165")
    currentDate = errorFile;
  else 
    currentDate = getDate();

  if (prevDate != currentDate) {
    //email data file of previous day
    p.runShellCommand("python /mnt/sda1/sendmood.py " + filetoday);
    filetoday += "/mnt/sd/";
    filetoday += currentDate;
    filetoday += ".txt";
    if (prevDate != "2165165165" && currentDate != "2165165165") {
          for (int i = 0; i < numButton; i++) {
            buttonCount[i] = 0;
           }
          buttonCountTotal = 0;
     }
  }

  File dataFile = FileSystem.open(filetoday.c_str(), FILE_APPEND);

  //reset the counters every day at 00:00:00 and enter the first line
  if (prevDate != currentDate) {       
    if (dataFile) {     
      String dataString = "This file is created at: ";
      dataString += getTimeStamp();      
      dataFile.println(dataString);//to enter the first line of the new file
      Console.println(dataString);
    }
    // if the file isn't open, pop up an error:
    else {
      Console.println("FIlL ERROR HERE!!!");
    }
  }
  

  // read the state of the pushbutton value:
  for (int i = 0; i < numButton; i++) {
    buttonReading[i] = digitalRead(buttonPin[i]);
    
    if (buttonReading[i] != lastButtonState[i]) {
        lastDebounceTime[i] = millis();
    }
  }


  // check if any button is pressed, if so change the color and enter a line to the current file
  for (int i = 0; i < numButton; i++) {
    if ((millis() - lastDebounceTime[i]) > debounceDelay) {
      if (buttonReading[i] != buttonState[i]) {
          buttonState[i] = buttonReading[i];
        if (buttonState[i] == HIGH) {
          updateData(i); //update buttonCountTotal and buttonCount[]
          updateColor(i); //set LED color to %

          if (buttonCountTotal > 0) {
              //save data into the file on SD card 
              String dataString;
              dataString += getTimeStamp();               
              dataString += " = Total Entry: ";
              dataString += int(buttonCountTotal);         
            
              for (int i = 0; i < numButton; i++) {
                dataString += ", ";
                dataString += mood[i];
                dataString += ": ";
                dataString += int(buttonCount[i]);
              }
           
              dataFile.println(dataString); 
              Console.println(dataString);
          }
        } 
      }
    }
    lastButtonState[i] = buttonReading[i];
  }

  LEDStrips.show();
  dataFile.close(); 
  delay(50);   
  
}

uint32_t hsvToRgbColor(double h, double s, double v) {
    double r, g, b;

    int i = (int)(h * 6);
    double f = h * 6 - i;
    double p = v * (1 - s);
    double q = v * (1 - f * s);
    double t = v * (1 - (1 - f) * s);

    switch(i % 6){
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
    }

    uint32_t color = ( ((uint32_t)(255*r) & 0xff ) << 16 ) + ( ((uint32_t)(255*g) & 0xff) << 8 ) + ( (uint32_t)(255*b) & 0xff );
    return color;
}

void updateData (int i) {
    buttonCount[i] ++;
    buttonCountTotal ++;
}

void updateColor(int i){
    int headLED = 0;
    int tailLED = 0;
    //COLOR FEEDBACK: set all LED to the color of the recent entry
    for (int k = 0; k < 6; ++k){
      for (int j = 0; j < numPixels; ++j ) { // Loop over all strips
            hue = hues[i];                    
            color = hsvToRgbColor( hue, 1.0, 1.0 );        
            j % 3 == 0 ? LEDStrips.setPixelColor( j, color ) : LEDStrips.setPixelColor( j, 0 );
                     
        }
       LEDStrips.show(); 
       delay(100);
       for (int j = 0; j < numPixels; ++j ) { // Loop over all strips
            hue = hues[i];                    
            color = hsvToRgbColor( hue, 1.0, 1.0 );        
            j % 3 == 1 ? LEDStrips.setPixelColor( j, color ) : LEDStrips.setPixelColor( j, 0 );
                     
        }
       LEDStrips.show(); 
       delay(100); 
       for (int j = 0; j < numPixels; ++j ) { // Loop over all strips
            hue = hues[i];                    
            color = hsvToRgbColor( hue, 1.0, 1.0 );        
            j % 3 == 2 ? LEDStrips.setPixelColor( j, color ) : LEDStrips.setPixelColor( j, 0 );
                     
        }
       LEDStrips.show(); 
       delay(100);        
     }      

     
    //COLOR UPDATE: reflect the % of the times each button is pressed        
    for (int j = 0; j < numButton; j++){  
        //calculate % of this color    
        headLED =  tailLED + buttonCount[j] / buttonCountTotal * numPixels;
        //set LEDs to this color
        for (int k = tailLED; k < headLED; k++ ) {
          color = hsvToRgbColor( hues[j], 1.0, 1.0 );        
          LEDStrips.setPixelColor( k, color );
         }

        //move to the next color
        tailLED = headLED;  

    }
    LEDStrips.show(); 
        
}

String getTimeStamp() {
  DateTime now = RTC.now();
  String result;
  result = now.year();
  result += now.month();
  result += now.day();
  result += "-";
  result += now.hour();
  result += ":";
  result += now.minute();
  result += ":";
  result += now.second();

  return result;
}

String getDate() {
  DateTime now = RTC.now();
  String result;
  result = now.year();
  result += now.month();
  result += now.day();
  return result;
}
