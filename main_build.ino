/*
   Inkplate_SD_TXT example for e-radionica.com Inkplate 10
   For this example you will need only a micro USB cable, Inkplate 10 and a SD card
   loaded with text.txt file that can be found inside folder of this example.
   Select "Inkplate 10(ESP32)" from Tools -> Board menu.
   Don't have "Inkplate 10(ESP32)" option? Follow our tutorial and add it:
   https://e-radionica.com/en/blog/add-inkplate-6-to-arduino-ide/

   To work with SD card on Inkplate, you will need to add one extra library.
   Download and install it from here: https://github.com/e-radionicacom/Inkplate-6-SDFat-Arduino-Library

   You can open your own .txt file, but in order to this example works properly it should
   not have more than 200 chars and you should name it text.txt

   This example will show you how to open .txt files and display the content of that file on Inkplate epaper display.

   Want to learn more about Inkplate? Visit www.inkplate.io
   Looking to get support? Write on our forums: http://forum.e-radionica.com/en/
   11 February 2021 by e-radionica.com
*/

// Next 3 lines are a precaution, you can ignore those, and the example would also work without them
#ifndef ARDUINO_INKPLATE10
#error "Wrong board selection for this example, please select Inkplate 10 in the boards menu."
#endif

#include "Inkplate.h"            //Include Inkplate library to the sketch
#include "SdFat.h"               //Include library for SD card
Inkplate display(INKPLATE_1BIT); // Create an object on Inkplate library and also set library into 1 Bit mode (BW)
SdFile file;                     // Create SdFile object used for accessing tours csv file on SD card
SdFile labelIdentifier;          // Create SdFile object used for accessing label file on SD card
SdFile tagsFile;                 // Create SDfile object for accessing the tags file on the sd card 


//card reader
#include <Wire.h> //i2c base library for communicating with the nfc reader
#include "MFRC522_I2C.h" //library for the nfc reader
#define RST_PIN 1 //the rst pin is defined in other code and we do need to pass it but it's just pulled high so we just connect it to a 3.3 pin
MFRC522 mfrc522(0x28, RST_PIN);  // Create MFRC522 instance.
void ShowReaderDetails(); //dumps the nfc software version etc to serial 

//buzzer initial values
int freq = 2000;
int channel = 0;
int resolution = 8;

int defaultTourID;

//getting text from sd card 
char text[2350];            // Array where data from SD card is stored (max in square brackets chars here)
  int numberOfRows; //how many rows of that csv array to share between functions
  //arrays for the csv contents to go in
  char ** Labels;
  int16_t *TourID;
  char **MainTitle;
  char **SecondTitle;
  char **SubTitle;
  char **MainText;

char ** Tags;
  int16_t *TagsTourID;
 int numberOfTagRows; //how many rows of that csv array to share between functions

String noTag = "NoTag";  //global variable used to show there was no tag data

//each label has a location number for each tour
int labelNumber; //variable to store the label location variable  

int tourId;



#include <CSV_Parser.h> //for reading the csv files

//fonts need to be declared
//#include "Fonts/FreeMonoBoldOblique18pt7b.h"
//#include "Fonts/FreeMono18pt7b.h"
//#include "Fonts/FreeMonoBold18pt7b.h"
//#include "Fonts/FreeMonoOblique18pt7b.h"

#include "Fonts/FreeSansBoldOblique18pt7b.h"
#include "Fonts/FreeSans18pt7b.h"
#include "Fonts/FreeSansBold18pt7b.h"
#include "Fonts/FreeSansOblique18pt7b.h"

#include "Fonts/FreeSansBoldOblique24pt7b.h"
#include "Fonts/FreeSans24pt7b.h"
#include "Fonts/FreeSansBold24pt7b.h"
#include "Fonts/FreeSansOblique24pt7b.h"

//#include "Fonts/FreeSerifBoldItalic18pt7b.h"
//#include "Fonts/FreeSerif18pt7b.h"
//#include "Fonts/FreeSerifBold18pt7b.h"
//#include "Fonts/FreeSerifItalic18pt7b.h"

#include "Fonts/TruenoBd24pt7b.h"
#include "Fonts/TruenoBd28pt7b.h"
#include "Fonts/TruenoBd36pt7b.h"
#include "Fonts/TruenoLtIt24pt7b.h"
#include "Fonts/TruenoRg18pt7b.h"
#include "Fonts/TruenoRg14pt7b.h"

// If your Inkplate doesn't have external (or second) MCP I/O expander, you should uncomment next line,
// otherwise your code could hang out when you send code to your Inkplate.
// You can easily check if your Inkplate has second MCP by turning it over and 
// if there is missing chip near place where "MCP23017-2" is written, but if there is
// chip soldered, you don't have to uncomment line and use external MCP I/O expander



void setup()
{
  Serial.begin(115200);
    display.begin();        // Init Inkplate library (you should call this function ONLY ONCE)
    display.clearDisplay(); // Clear frame buffer of display
    display.display();      // Put clear image on display
     Serial.println("READ Label System Start");
     
     Wire.begin(); // Initialize I2C
     mfrc522.PCD_Init();   // Init MFRC522
    ShowReaderDetails();  // Show details of PCD - MFRC522 Card Reader details
    

//read the csv from the sd card and parse into an array we can grab the labels into variables for the correct tour
//this uses the function readtourscsv to get the sd card (see bottom) 
CSV_Parser cp(readtourscsv(), /*format*/ "sdssss"); //start up the csv parser //format s = string, d = int

numberOfRows = cp.getRowsCount();

//get values from the CSV parser and put them in arrays based on the row number 
Labels = (char**)cp["Label"];
TourID = (int16_t*)cp["TourID"];
MainTitle = (char**)cp["MainTitle"];
SecondTitle = (char**)cp["SecondTitle"];
SubTitle = (char**)cp["SubTitle"];
MainText = (char**)cp["MainText"];


  
labelNumber = getLabelNumber(); //get the label number from the sd card (see the function below)


Serial.println("Label Number: ");
Serial.println(labelNumber);

//get the list of tags and which tour they link to

//read the csv from the sd card and parse into an array we can grab the tags into variables for the correct tour
//this uses the function ReadTagsCsv to get the sd card (see bottom) 
CSV_Parser cpTags(ReadTagsCsv(), /*format*/ "sd"); //start up the csv parser //format s = string, d = int

numberOfTagRows = cpTags.getRowsCount();

//get values from the CSV parser and put them in arrays based on the row number 
Tags = (char**)cpTags["UID"];
TagsTourID = (int16_t*)cpTags["TourID"];


ledcSetup(channel, freq, resolution); //esp32 doesn't do standard pwm so we use the led version instead (even though its a buzzer)
ledcAttachPin(14, channel); //setup the buzzer pin (GPIO 14 dont' use 12 or 13 else it messes with sd card reading)

defaultTourID = 1;
DisplayTheLabel(defaultTourID);

        }

void loop()

{

Serial.println("Starting main loop");
Serial.flush();
String uid = LookForTags(); //function below that looks for a NFC tag and returns the serial number (uid) when it does
  
if (!uid.compareTo(noTag)==0){ //if the tag is moved too quick it only gives a partial number so this function stops it freaking out
  Serial.println(uid);

//String frenchTag = "8a53fb39";
//
////temp proof of concept
//if (uid.compareTo(frenchTag)==0){
//  tourId = 2; //this is where the NFC card reader will come in
//}
//  else{
//   tourId = 1; //
//  }

for(int tagrow = 0; tagrow < numberOfTagRows; tagrow++) {
    Serial.println(Tags[tagrow]);
    
    if (uid.compareTo(Tags[tagrow])==0){ //
      tourId = TagsTourID[tagrow];//this will be the row relevent
      Serial.println(tourId);
      Serial.println(tagrow);
      
    }}
  

DisplayTheLabel(tourId); //function below that displays the label based on the tour ID
           
  delay(3000); // wait 3 seconds before looking again
  


  
}






}


//each label has a location indetifier number so that it knows what content to display
//it's stored in a plain text file on the SD card
//this function grabs it
int getLabelNumber()
{
  int labelNumber;

 // Init SD card. 
    if (display.sdCardInit())
    {
   // Try to load text file 
        if (!labelIdentifier.open("/label_identifier.txt", O_RDONLY))
        { // If it fails to open, send error message, otherwise read the file.
            Serial.print("file open error");
        }
        else
        {
            //reading text file from SD card code
           char text[100];            // Array where data from SD card is stored (max in square brackets chars here)
           int len = labelIdentifier.fileSize(); // Read how big is file that we are opening

            if (len > 100)
                len = 100;        // If it's more than x bytes (x chars), limit to max x bytes
            labelIdentifier.read(text, len); // Read data from file and save it in text array;'
              text[len] = 0;        // Put null terminating char at the and of data
            labelNumber = atoi(text); //convert char to int and stick it in our variable
        }
    } 

    return labelNumber;
    }

//routine to get the nfc reader details and output to serial
 void ShowReaderDetails() {
  // Get the MFRC522 software version
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.print(F("MFRC522 Software Version: 0x"));
  Serial.print(v, HEX);
  if (v == 0x91)
    Serial.print(F(" = v1.0"));
  else if (v == 0x92)
    Serial.print(F(" = v2.0"));
  else
    Serial.print(F(" (unknown)"));
  Serial.println("");
  // When 0x00 or 0xFF is returned, communication probably failed
  if ((v == 0x00) || (v == 0xFF)) {
    Serial.println(F("WARNING: Communication failure, is the MFRC522 properly connected?"));
  }
}


//read tours csv from sd card
char  * readtourscsv(){

 // char output;
 // Init SD card. Display if SD card is init propery or not.
    if (display.sdCardInit())
    {
        display.println("SD Card ok! Reading data...");
        //Serial.print("SD Card Initialised");
        display.partialUpdate();

        // Try to load csv file 
        if (!file.open("/tours.csv", O_RDONLY))
        { // If it fails to open, send error message to display, otherwise read the file.
            display.println("File open error");
            Serial.print("file open error");

            display.display();
        }
        else
        {
          // Serial.print("file open success");
            display.clearDisplay();    // Clear everything that is stored in frame buffer of epaper
            display.setCursor(0, 50);   // Set print position at the begining of the screen

     int len = file.fileSize(); // Read how big is file that we are opening

            if (len > 2350)
                len = 2350;        // If it's more than x bytes (x chars), limit to max x bytes


         file.read(text, len); // Read data from file and save it in text array;'
        text[len] = 0;        // Put null terminating char at the and of data
        }
    }
        
     else
    { // If card init was not successful, display error on screen and stop the program (using infinite loop)
        display.println("SD Card error!");
        display.partialUpdate();
        while (true)
            ;
    }
   
    
    return text;
}

//read tags csv from sd card
char  * ReadTagsCsv(){

 // char output;
 // Init SD card. Display if SD card is init propery or not.
    if (display.sdCardInit())
    {
        display.println("SD Card ok! Reading data...");
        //Serial.print("SD Card Initialised");
        display.partialUpdate();

        // Try to load csv file 
        if (!tagsFile.open("/tags.csv", O_RDONLY))
        { // If it fails to open, send error message to display, otherwise read the file.
            display.println("File open error");
            Serial.print("file open error");

            display.display();
        }
        else
        {
          // Serial.print("file open success");
            display.clearDisplay();    // Clear everything that is stored in frame buffer of epaper
            display.setCursor(0, 50);   // Set print position at the begining of the screen

     int len = tagsFile.fileSize(); // Read how big is file that we are opening

            if (len > 2350)
                len = 2350;        // If it's more than x bytes (x chars), limit to max x bytes


         tagsFile.read(text, len); // Read data from file and save it in text array;'
        text[len] = 0;        // Put null terminating char at the and of data
        }
    }
        
     else
    { // If card init was not successful, display error on screen and stop the program (using infinite loop)
        display.println("SD Card error!");
        display.partialUpdate();
        while (true)
            ;
    }
   
    
    return text;
}

String LookForTags(){
    
     
        // Look for new cards
  while ( ! mfrc522.PICC_IsNewCardPresent()) {
    //code stops here until a tag appears
    //Serial.println("Now looking for tags"); 
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) { 
   //fail safe if tag if moved too soon or partially read
    return noTag;
  }
  
String uid; //variable for the UID
char hexbyte[2]; //variable for the hex converion
char hexstring[50]; //variable for string conversion
strcpy (hexstring,""); //adds the null terminator at the end of the string
//work through the UID register on the card and add it to our variable until its done
for (byte i = 0; i < mfrc522.uid.size; i++) { 
  sprintf(hexbyte, "%02x", mfrc522.uid.uidByte[i]); //this was used to stop leading zeros being lost
  strcat (hexstring, hexbyte); //combine the hexbytes to the hexstring    
  // uncomment to Dump debug info about the card; PICC_HaltA() is automatically called
 // mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
}

uid = String(hexstring); //convert to a String for sending back and comparison
 Serial.println("uid:");
 Serial.println(uid);

 
//make the buzzer beep
ledcWriteTone(channel, 3000);
ledcWrite(channel, 125);
ledcWriteTone(channel, 1100);
delay(75);
ledcWriteTone(channel, 1400);
delay(75);
ledcWriteTone(channel, 0);   

//battery experiments
 float voltage = display.readBattery(); 
Serial.println(voltage);

//send back the UID
return uid;
}

void DisplayTheLabel(int tourId) {
Serial.flush();
Serial.println("Starting Display the Label Function");

  
  int labelRow = 0; //a variable to store the correct row when we found it 

Serial.println("Looking through csv to match label and tour ID");

//go through the csv array and look for the row that matches both the label and the tour ID
for(int row = 0; row < numberOfRows; row++) {
    if ((atoi(Labels[row]) == labelNumber) && (TourID[row] == tourId)){ //atoi converts the char to int to we can compare
      labelRow = row; //this will be the row relevent
    }
    
  }


Serial.println("Putting contents in variables");

//using the row number we now have put the contents in the relevent variables
//Serial.println("main title");
//char* mainTitle = MainTitle[labelRow];
//Serial.println("second row");
//char* secondTitle = SecondTitle[labelRow];
//Serial.println("subtitle");
//char* subTitle = SubTitle[labelRow];
//Serial.println("main text");
char* mainText = MainText[labelRow];

//clear the screen
display.clearDisplay(); // Clear frame buffer of display
display.display();
display.setCursor(0,50); //put the cursor back at the top
//now put them up on the screen
            //Title/Artist
            Serial.println("main title");
            display.setFont(&TruenoBd36pt7b);
            display.println(MainTitle[labelRow]);
            
            display.partialUpdate();
            //Item
            Serial.println("second row");
            display.setFont(&TruenoBd24pt7b);
            display.println(SecondTitle[labelRow]);
            display.partialUpdate();
            //subtitle
            Serial.println("subtitle");
            display.setFont(&TruenoLtIt24pt7b);
            display.println(SubTitle[labelRow]);
            display.println(" ");
            display.partialUpdate();
            //text
            display.setFont(&TruenoRg14pt7b);
           // display.println(mainText);

            //test
            Serial.println("main text");
            Serial.println(MainText[labelRow]);
            //display.println(MainText[labelRow]);
            displayWrappedText(mainText);
            
            Serial.println("end of display function");
             //Serial.println(MainText[labelRow]);
             
            
            //display.partialUpdate();
            //display.display();    // Do a full refresh of display
}

void displayWrappedText(char* text){
  int lineTotal = 0;
//the standard println function doesn't wrap by spaces in word so this function takes care of that for longer text
//It doesn't dynamically look at the font size and work it out so those varaibles have to be figured out mannually beforehand
Serial.println("declare line variable");
char line[80];
//strtok breaks up the long text into words based on the space delimiter
Serial.println("strtok");
char* word = strtok(text, " ");
  
  //for each word then
  Serial.println("while loop");
  while (word != NULL) {
    // Calculate the width of the current word
    uint16_t wordWidth = strlen(word);

    //Serial.println(word);
   
   //keep a running total of how long the line is getting
   lineTotal = wordWidth + lineTotal;

   
   //add the word to the line with a space
   strncat(line,word,sizeof(line) - strlen(line) - 1);
   strncat(line," ",sizeof(line) - strlen(line) - 1);
// Serial.println(lineTotal);
   //if the line is now as long as it will fit on the screen
    if (lineTotal > 60){
     //display the line
     Serial.println("print line");
     display.println(line);
     display.partialUpdate();
     //reset the line variables     
     strncpy(line,"", sizeof(line) - strlen(line) - 1); 
      lineTotal = 0;
    }
     
 word = strtok(NULL, " ");
}
//display.println(line);
//display.partialUpdate();
Serial.println("end of function");
}
