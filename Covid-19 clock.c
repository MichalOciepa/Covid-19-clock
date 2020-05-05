
/*
  Wiring on Wemos D1 R2
  5v -> 5v
  Gnd -> Gnd
  SCA -> SCA
  SCL -> SCL
*/

//Defining time parameters
unsigned long curMillis;
unsigned long prevMillis = 0;
const unsigned long displayDuration = 10000UL;    //screen display time
const unsigned long displayBlankDuration = 500UL; //screen switching time
unsigned long prevFetchMillis = 0;
const unsigned long fetchDelay = 300000UL; //When to download data from the API (the API on which it works is refreshed every 5 minutes)
int displayState;                          //status of displayed data

enum
{
    INFO,
    INFECTED,
    DECEASED,
    REGION,
    INFECTED_REGION,
    DECEASED_REGION,
    LAST_STATE
};

//Defining variables
int infected;
int deceased;
int regionInfectedCount;
int regionDeceasedCount;
int oldInfected;

//Buzzer parameters
const int buzzer = 14; //Number of buzzer Pin
unsigned long lastBuzzer;
const unsigned long buzzerDelay = 10000UL;

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>
#include <String.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <SSD1306.h>
#include <SSD1306Wire.h>
#include <NTPClient.h>
#include <Time.h>
#include <TimeLib.h>
#include <Timezone.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2); //or try 0x3f

// Define NTP properties
#define NTP_OFFSET 60 * 60         // In seconds
#define NTP_INTERVAL 60 * 1000     // In miliseconds
#define NTP_ADDRESS "pool.ntp.org" // change this to whatever pool is closest (see ntp.org)

// Set up the NTP UDP client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);

const char *ssid = "";     // insert your own ssid such as BTHub4-9X99
const char *password = ""; // and your wifi password

String date; //create the string for the date which will be printed on the lcd screen below
String t;    // create the string for the time

//You can replace the shortcuts month, e.g. "mar", for the readability of the displayed information I chose this entry
const char *months[] = {"01", "02", "03", "04", "05", "06", "07", "08", "09", "10", "11", "12"};

void clearLCDLine(int line) //The "cleaning" function of the display line, enter the line number in brackets, remember that the first line is 0
{
    lcd.setCursor(0, line);
    for (int n = 0; n < 16; n++) //For 2x16 LCD write - 16
    {
        lcd.print(" ");
    }
}

void setup()
{
    Serial.begin(115200); // Included for serial monitor debugging
    timeClient.begin();   // Start the NTP UDP client

    prevFetchMillis = millis() - fetchDelay; // so we fetch the first time through loop()
    prevMillis = millis() - displayDuration;

    //  connect the esp8266 to your wifi //
    int cursorPosition = 0;
    lcd.begin();
    lcd.backlight();
    Wire.begin();
    lcd.print("Polacz");

    lcd.setCursor(0, 0);
    lcd.print("Polacz");
    lcd.setCursor(0, 1);
    lcd.print(ssid);

    // Connect to wifi
    Serial.println(""); //checking the correct operation in the console
    Serial.print("Laczenie z ");
    Serial.print(ssid);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Polaczono z WiFi ");
    Serial.print(WiFi.localIP());
    Serial.println("");

    delay(1000);
    // end wifi connection set up //
}

void loop()
{
    // The first part of the loop is for the internet clock //

    curMillis = millis();

    if (WiFi.status() == WL_CONNECTED) //Check WiFi connection status
    {
        refreshTime(); // function for refreshing the clock data

        // Display the date and time

        //    Serial.println("");
        //    Serial.print("Local date: ");
        //    Serial.print(date);
        //    Serial.println("");
        //    Serial.print("Local time: ");
        //    Serial.print(t);                                  // uncomment if you want to check the clock operation in the console

        //lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(t);
        lcd.setCursor(6, 0);
        lcd.print(date); // settings for the clock display location

        // check if it is time to download information
        if (curMillis - prevFetchMillis >= fetchDelay)
        {
            fetchData(); // downloading data from the API

            Serial.println("");
            Serial.print("Nowe zachorowania: ");
            Serial.print(infected);

            Serial.println("");
            Serial.print("Stare zachorowania: ");
            Serial.print(oldInfected);

            if (oldInfected != infected) // comparing new cases with old ones
            {
                // tone(buzzer, 1000, 500);
                // delay(500);
                // tone(buzzer, 500, 500);
                // delay(500);
                // tone(buzzer, 1000, 500);
                // delay(500);
                // tone(buzzer, 500, 500);
                // delay(500);
                // tone(buzzer, 1000, 500);
                // delay(500);
                // tone(buzzer, 500, 500);
                // delay(500);
                // tone(buzzer, 1000, 500);
                // delay(500);
                // tone(buzzer, 500, 500);
                // delay(500);
                // noTone(buzzer);   I don't know how to add buzzer without delay

                Serial.println("");
                Serial.print("there are new cases"); //check in console
            }
            else
            {
                noTone(buzzer);
                Serial.println("");
                Serial.print("there are no new cases"); //check in console
            }
            oldInfected = infected;
            prevFetchMillis = curMillis;
        }

        // check if it is time to blank the line to prepare for next state
        if ((curMillis - prevMillis) >= displayDuration - displayBlankDuration)
        {
            clearLCDLine(1);
        }

        // check if it is time to display the next bit of information
        if (curMillis - prevMillis >= displayDuration)
        {
            // move to next display state
            prevMillis = curMillis;
            displayState++;
            if (displayState == LAST_STATE)
                displayState = 0;
            lcd.setCursor(0, 1);
            switch (displayState)
            {
            case INFO:
                lcd.print("Polska COVID-19");
                break;

            case INFECTED:
                lcd.print("Chorych:");
                lcd.setCursor(9, 1);
                lcd.print(infected, DEC);
                break;

            case DECEASED:
                lcd.print("Zgonow:");
                lcd.setCursor(8, 1);
                lcd.print(deceased, DEC);
                break;

            case REGION:
                lcd.print("WIELKOPOLSKIE");
                break;

            case INFECTED_REGION:
                lcd.print("Chorych:");
                lcd.setCursor(9, 1);
                lcd.print(regionInfectedCount, DEC);
                break;

            case DECEASED_REGION:
                lcd.print("Zgonow:");
                lcd.setCursor(8, 1);
                lcd.print(regionDeceasedCount, DEC);
                break;
            }
        }
    }

    else // this part is a step to attempt to connect to wifi again if disconnected
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Polacz");

        //display.display();
        WiFi.begin(ssid, password);
        //display.drawString(0, 24, "Connected.");

        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("Polacz.");

        delay(1000);
    }

    delay(1000); //Send a request to update every 1 sec
} //end void loop

void refreshTime()
{
    date = ""; // clear the variables
    t = "";

    // update the NTP client and get the UNIX UTC timestamp
    timeClient.update();
    unsigned long epochTime = timeClient.getEpochTime();

    // convert received time stamp to time_t object
    time_t local, utc;
    utc = epochTime;

    // Then convert the UTC UNIX timestamp to local time
    TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 60}; //Central European Summer Time
    TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 0};   //Central European Standard Time
    Timezone CE(CEST, CET);
    local = CE.toLocal(utc);

    // now format the Time variables into strings with proper names for month, day etc
    date += day(local);
    date += ".";
    date += months[month(local) - 1];
    date += ".";
    date += year(local);

    // format the time to 12-hour format with AM/PM and add seconds. t (time) is made up of the variables below which are then printed as a string
    if (hour(local) < 10)
        t += " ";
    t += hour(local);
    t += ":";
    if (minute(local) < 10) // add a zero if minute is under 10
        t += "0";
    t += minute(local);
    t += " ";
    // if (second(local) < 10)
    //     t += "0";
    // t += second(local);                      // uncomment if you want to see the seconds on the display
    //t += ampm[isPM(local)];                   // I don't use AM / PM in the project
}

void fetchData() //downloading data from the API
{
    HTTPClient http;
    http.begin("http://api.apify.com/v2/key-value-stores/3Po6TV7wTht4vIEid/records/LATEST?disableRedirect=true");
    int httpCode = http.GET();
    if (httpCode > 0)
    {
        const size_t capacity = JSON_ARRAY_SIZE(16) + 16 * JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(6) + 1080;
        DynamicJsonDocument doc(capacity);
        deserializeJson(doc, http.getString());
        JsonArray infectedByRegion = doc["infectedByRegion"];

        // Parameters
        infected = doc["infected"];
        deceased = doc["deceased"]; //Incidence and deaths throughout Poland

        JsonObject infectedByRegion_14 = infectedByRegion[14];
        regionInfectedCount = infectedByRegion_14["infectedCount"];
        regionDeceasedCount = infectedByRegion_14["deceasedCount"]; //Illnesses and deaths in my region

        // If you want to download the entire Parsing program with the declaration of variables for your region, I invite you to the file: Parsing program.txt. Copy and paste. Declare the variables at the beginning of the code, as in my case
    }
    http.end(); //Close connection
}
