#include "CommandShell.h"
#include "dateTimeValidator.h"
CommandShell CommandLine;

#include <RTClock.h>
#include <TimeLib.h>
#include <WS2812B.h>
#include <ColorConverterLib.h>

#include <Timezone.h>   // https://github.com/JChristensen/Timezone

#include <MicroNMEA.h>
char gpsBuffer[85];
MicroNMEA nmea(gpsBuffer, sizeof(gpsBuffer));
#define GPSSerial Serial2
#define GPS_INTERVAL (60 * 60 * 1000ul)
uint32_t lastGPS;

// US Eastern Time Zone (New York, Detroit)
TimeChangeRule myDST = {"EDT", Second, Sun, Mar, 2, -240};    // Daylight time = UTC - 4 hours
TimeChangeRule mySTD = {"EST", First, Sun, Nov, 2, -300};     // Standard time = UTC - 5 hours
Timezone myTZ(myDST, mySTD);

RTClock rtclock (RTCSEL_LSE);
WS2812B strip = WS2812B(36);

uint8_t minuteOnesColour[] = {
  255, 0, 0};
uint8_t minuteTensColour[] = {
  0, 255, 0};
uint8_t hourOnesColour[] = {
  0, 0, 255};
uint8_t hourTensColour[] = {
  127, 0, 127};

#define MaxBlanks 2

int setDateFunc(char * args[], char num_args);
int setTimeFunc(char * args[], char num_args);
int printTimeFunc(char * args[], char num_args);
int gpsInfoFunc(char * args[], char num_args);

/* UART command set array, customized commands may add here */
commandshell_cmd_struct_t uart_cmd_set[] =
{
  {
    "setDate", "\tsetDate [day] [month] [year]", setDateFunc      }
  ,
  {
    "setTime", "\tsetTime [hours] [minutes] [seconds]", setTimeFunc      }
  ,
  {
    "printTime", "\tprintTime", printTimeFunc      }
  ,
  {
    "gpsInfo", "\tgpsInfo", gpsInfoFunc      }
  ,
  {
    0,0,0      }
};

int setDateFunc(char * args[], char num_args) {
  if(3 != num_args) {
    Serial.println(F("Insufficient arguments!"));
    return 1;
  }

  uint8_t dayNum = atoi(args[0]);
  uint8_t monthNum = atoi(args[1]);
  uint16_t yearNum = atoi(args[2]);

  uint8_t tmp = validateDate(yearNum, monthNum, dayNum);

  if(tmp == 2) {
    Serial.println(F("Invalid year!"));
    return 2;
  } else if(tmp == 3) {
    Serial.println(F("Invalid month!"));
    return 3;
  } else if(tmp == 4) {
    Serial.println(F("Invalid day!"));
    return 4;
  }
  
  tmElements_t newTime;
  time_t tmp_t;
  tmp_t = now();
  tmp_t = myTZ.toLocal(tmp_t);
  breakTime(tmp_t, newTime);
  newTime.Year = CalendarYrToTm(yearNum);
  newTime.Month = monthNum;
  newTime.Day = dayNum;
  
  tmp_t = makeTime(newTime);
  tmp_t = myTZ.toUTC(tmp_t);
  rtclock.setTime(tmp_t);
  setTime(tmp_t);

  Serial.print(F("Setting date to "));
  Serial.print(dayNum);
  Serial.print('/');
  Serial.print(monthNum);
  Serial.print('/');
  Serial.println(yearNum);
  return 0;  
}

int setTimeFunc(char * args[], char num_args) {
  if(3 != num_args) {
    Serial.println(F("Insufficient arguments!"));
    return 1;
  }

  int hourNum = atoi(args[0]);
  int minNum = atoi(args[1]);
  int secNum = atoi(args[2]);

  uint8_t tmp = validateTime(hourNum, minNum, secNum);

  if(tmp == 2) {
    Serial.println(F("Invalid hours!"));
    return 2;
  } else if(tmp == 3) {
    Serial.println(F("Invalid minutes!"));
    return 3;
  } else if(tmp == 4) {
    Serial.println(F("Invalid seconds!"));
    return 4;
  }

  tmElements_t newTime;
  time_t tmp_t;
  tmp_t = now();
  tmp_t = myTZ.toLocal(tmp_t);
  breakTime(tmp_t, newTime);
  newTime.Hour = hourNum;
  newTime.Minute = minNum;
  newTime.Second = secNum;
  tmp_t = makeTime(newTime);
  tmp_t = myTZ.toUTC(tmp_t);
  rtclock.setTime(tmp_t);
  setTime(tmp_t);

  Serial.print(F("Setting time to "));
  Serial.print(hourNum);
  Serial.print(F(":"));
  Serial.print(minNum);
  Serial.print(F(":"));
  Serial.println(secNum);
  return 0;  
}

int printTimeFunc(char * args[], char num_args) {
  Serial.print(F("The current time is:"));

  tmElements_t newTime;
  time_t tmp_t;
  tmp_t = now();
  tmp_t = myTZ.toLocal(tmp_t);
  breakTime(tmp_t, newTime);
  Serial.print(tmYearToCalendar(newTime.Year), DEC);
  Serial.print('/');
  Serial.print(newTime.Month, DEC);
  Serial.print('/');
  Serial.print(newTime.Day, DEC);
  Serial.print(' ');
  Serial.print(newTime.Hour, DEC);
  Serial.print(':');
  Serial.print(newTime.Minute, DEC);
  Serial.print(':');
  Serial.print(newTime.Second, DEC);
  Serial.println();
  return 0;
}

int gpsInfoFunc(char *args[], char num_args) {
  if (nmea.isValid()) {
    Serial.println("GPS has fix");
  } else {
    Serial.println("GPS does not have fix");
  }
  Serial.print("GPS Date/time is: ");
  Serial.print(nmea.getYear());
  Serial.print('-');
  Serial.print(int(nmea.getMonth()));
  Serial.print('-');
  Serial.print(int(nmea.getDay()));
  Serial.print('T');
  Serial.print(int(nmea.getHour()));
  Serial.print(':');
  Serial.print(int(nmea.getMinute()));
  Serial.print(':');
  Serial.println(int(nmea.getSecond()));
  Serial.print("GPS last synced at mills() value ");
  Serial.println(lastGPS);
  Serial.print("Current millis() value is ");
  Serial.println(millis());
  Serial.print("GPS sync interval is ");
  Serial.println(GPS_INTERVAL);
  return 0;
}

time_t getHwTime(void) {
  return rtclock.getTime();
}

void setup(void) {
  Serial.begin(9600);
  Serial.println(F("Starting"));

  strip.begin();
  for(uint8_t i = 0; i < 36; i++)
    strip.setPixelColor(i, 0ul);
  strip.show();

  setTime(getHwTime());
  setSyncProvider(getHwTime);
  setSyncInterval(600);

  randomSeed(now());
  CommandLine.commandTable = uart_cmd_set;
  CommandLine.init(&Serial);

  GPSSerial.begin(9600);
}

void loop(void) {
  tmElements_t nowTime;
  time_t tmp_t;
  tmp_t = now();
  tmp_t = myTZ.toLocal(tmp_t);
  breakTime(tmp_t, nowTime);
  static bool clockIsSet = false;

  CommandLine.runService();

  static uint8_t alreadyRan = 2;

  uint8_t ShuffleData[9];
  uint8_t minuteOnes = nowTime.Minute % 10;
  uint8_t minuteTens = nowTime.Minute / 10;
  uint8_t hourOnes = nowTime.Hour % 10;
  uint8_t hourTens = nowTime.Hour / 10;

  uint8_t r, g, b, tempH, lastH;

  if(((nowTime.Second % 30 == 0) && (alreadyRan == 1)) || (alreadyRan == 2)) {
    tempH = random(255);
    ColorConverter::HsvToRgb(((float)tempH)/256.0, 1.0, 1.0, r, g, b);
    minuteOnesColour[0] = r;
    minuteOnesColour[1] = g;
    minuteOnesColour[2] = b;

    if(nowTime.Minute > 9) {
      lastH = tempH;
      while(!checkDifference(lastH, tempH, 75)) tempH = random(255);
      ColorConverter::HsvToRgb(((float)tempH)/256.0, 1.0, 1.0, r, g, b);
      minuteTensColour[0] = r;
      minuteTensColour[1] = g;
      minuteTensColour[2] = b;
    }

    if(hourOnes != 0) {
      lastH = tempH;
      while(!checkDifference(lastH, tempH, 75)) tempH = random(255);
      ColorConverter::HsvToRgb(((float)tempH)/256.0, 1.0, 1.0, r, g, b);
      hourOnesColour[0] = r;
      hourOnesColour[1] = g;
      hourOnesColour[2] = b;
    }

    lastH = tempH;
    while(!checkDifference(lastH, tempH, 75)) tempH = random(255);
    ColorConverter::HsvToRgb(((float)tempH)/256.0, 1.0, 1.0, r, g, b);
    hourTensColour[0] = r;
    hourTensColour[1] = g;
    hourTensColour[2] = b;

    for(uint8_t i = 0; i < 36; i++)
      strip.setPixelColor(i, 0ul);

    uint32_t tempColour = 0;

    // Hours 10 digit...
    for (uint8_t i = 0; i < 6; ++i)
      ShuffleData[i] = (i < hourTens) ? 1 : 0;

    DoShuffle(ShuffleData, 3);
    tempColour = strip.Color(hourTensColour[0], hourTensColour[1], hourTensColour[2]);
    if(ShuffleData[0]) strip.setPixelColor(0, tempColour);
    if(ShuffleData[1]) strip.setPixelColor(23, tempColour);
    if(ShuffleData[2]) strip.setPixelColor(24, tempColour);

    // Hours 1 digit...
    for (uint8_t i = 0; i < 9; ++i)
      ShuffleData[i] = (i < hourOnes) ? 1 : 0;

    DoShuffle(ShuffleData, 9);
    tempColour = strip.Color(hourOnesColour[0], hourOnesColour[1], hourOnesColour[2]);
    if(ShuffleData[0]) strip.setPixelColor(2, tempColour);
    if(ShuffleData[1]) strip.setPixelColor(3, tempColour);
    if(ShuffleData[2]) strip.setPixelColor(4, tempColour);
    if(ShuffleData[3]) strip.setPixelColor(19, tempColour);
    if(ShuffleData[4]) strip.setPixelColor(20, tempColour);
    if(ShuffleData[5]) strip.setPixelColor(21, tempColour);
    if(ShuffleData[6]) strip.setPixelColor(26, tempColour);
    if(ShuffleData[7]) strip.setPixelColor(27, tempColour);
    if(ShuffleData[8]) strip.setPixelColor(28, tempColour);
    
    // Minutes 10 digit...
    for (uint8_t i = 0; i < 9; ++i)
      ShuffleData[i] = (i < minuteTens) ? 1 : 0;

    DoShuffle(ShuffleData, 6);
    tempColour = strip.Color(minuteTensColour[0], minuteTensColour[1], minuteTensColour[2]);
    if(ShuffleData[0]) strip.setPixelColor(6, tempColour);
    if(ShuffleData[1]) strip.setPixelColor(7, tempColour);
    if(ShuffleData[2]) strip.setPixelColor(16, tempColour);
    if(ShuffleData[3]) strip.setPixelColor(17, tempColour);
    if(ShuffleData[4]) strip.setPixelColor(30, tempColour);
    if(ShuffleData[5]) strip.setPixelColor(31, tempColour);
    
    // Minutes 1 digit...
    for (uint8_t i = 0; i < 9; ++i)
      ShuffleData[i] = (i < minuteOnes) ? 1 : 0;

    DoShuffle(ShuffleData, 9);
    tempColour = strip.Color(minuteOnesColour[0], minuteOnesColour[1], minuteOnesColour[2]);
    if(ShuffleData[0]) strip.setPixelColor(9, tempColour);
    if(ShuffleData[1]) strip.setPixelColor(10, tempColour);
    if(ShuffleData[2]) strip.setPixelColor(11, tempColour);
    if(ShuffleData[3]) strip.setPixelColor(12, tempColour);
    if(ShuffleData[4]) strip.setPixelColor(13, tempColour);
    if(ShuffleData[5]) strip.setPixelColor(14, tempColour);
    if(ShuffleData[6]) strip.setPixelColor(33, tempColour);
    if(ShuffleData[7]) strip.setPixelColor(34, tempColour);
    if(ShuffleData[8]) strip.setPixelColor(35, tempColour);
    
    strip.show();
    alreadyRan = 0;
  } 
  else if(nowTime.Second % 30 == 1) {
    alreadyRan = 1;
  }

  while (GPSSerial.available()) {
    char c = GPSSerial.read();
    if (nmea.process(c)) {
      if (nmea.isValid()) {
        if ((millis() - lastGPS >  GPS_INTERVAL) || (!clockIsSet)) {
          nowTime.Year = CalendarYrToTm(nmea.getYear());
          nowTime.Month = nmea.getMonth();
          nowTime.Day = nmea.getDay();
          nowTime.Hour = nmea.getHour();
          nowTime.Minute = nmea.getMinute();
          nowTime.Second = nmea.getSecond();

          tmp_t = makeTime(nowTime);
          rtclock.setTime(tmp_t);
          setTime(tmp_t);
  
          lastGPS = millis();
          clockIsSet = true;
        }
      }
    }
  }
}

void DoShuffle(uint8_t ShuffleData[9], uint8_t NumElems)
{
  for (uint8_t i = NumElems - 1; i > 0; --i)
  {   // Swap ShuffleData[i] with a random element from the set of 
    // ShuffleData[0..i] (yes, possibly itself.)
    uint8_t SwapElemIdx = ((uint8_t) random(255)) % (i + 1);

    if (SwapElemIdx == i)
      continue; // Swapping the current element with itself.

    uint8_t Temp = ShuffleData[i];
    ShuffleData[i] = ShuffleData[SwapElemIdx];
    ShuffleData[SwapElemIdx] = Temp;
  }
}

uint8_t checkDifference(uint8_t itemOne, uint8_t itemTwo, uint8_t threshold) {
  uint8_t diff;

  if(itemOne > itemTwo) {
    diff = itemOne - itemTwo;
  } 
  else {
    diff = itemTwo - itemOne;
  }

  if(diff >= threshold) {
    return 1;
  } 
  else {
    return 0;
  }
}
