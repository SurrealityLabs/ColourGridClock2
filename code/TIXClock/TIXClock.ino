#include "CommandShell.h"
#include "dateTimeValidator.h"
CommandShell CommandLine;

#include <RTClock.h>
#include <WS2812B.h>

RTClock rtclock (RTCSEL_LSE);
WS2812B strip = WS2812B(36);

#define TZ -5

volatile uint8_t displayArray[3][32];
uint8_t newDisplayArray[3][32];

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
  
  tm_t newTime;
  rtclock.breakTime(rtclock.now(), newTime);
  newTime.year = yearNum - 1970;
  newTime.month = monthNum;
  newTime.day = dayNum;
  rtclock.setTime(rtclock.TimeZone(rtclock.makeTime(newTime), TZ));

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

  tm_t newTime;
  rtclock.breakTime(rtclock.now(), newTime);
  newTime.hour = hourNum;
  newTime.minute = minNum;
  newTime.second = secNum;
  rtclock.setTime(rtclock.TimeZone(rtclock.makeTime(newTime), TZ));

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

  tm_t newTime;
  rtclock.breakTime(rtclock.now(), newTime);
  Serial.print(newTime.year, DEC);
  Serial.print('/');
  Serial.print(newTime.month, DEC);
  Serial.print('/');
  Serial.print(newTime.day, DEC);
  Serial.print(' ');
  Serial.print(newTime.hour, DEC);
  Serial.print(':');
  Serial.print(newTime.minute, DEC);
  Serial.print(':');
  Serial.print(newTime.second, DEC);
  Serial.println();
  return 0;
}

void setup(void) {
  for(uint8_t i = 0; i < 36; i++)
    strip.setPixelColor(i, 0ul);
  strip.show();

  Serial.begin(9600);
  Serial.println(F("Starting"));

  randomSeed(rtclock.now());
  CommandLine.commandTable = uart_cmd_set;
  CommandLine.init(&Serial);
}

void loop(void) {
  tm_t nowTime;
  rtclock.breakTime(rtclock.now(), nowTime);

  CommandLine.runService();

  static uint8_t alreadyRan = 2;

  uint8_t ShuffleData[9];
  uint8_t minuteOnes = nowTime.minute % 10;
  uint8_t minuteTens = nowTime.minute / 10;
  uint8_t hourOnes = nowTime.hour % 10;
  uint8_t hourTens = nowTime.hour / 10;

  uint8_t r, g, b, tempH, lastH;

  if(((nowTime.second % 30 == 0) && (alreadyRan == 1)) || (alreadyRan == 2)) {
    tempH = random(255);
    h2rgb(tempH, r, g, b);
    minuteOnesColour[0] = r;
    minuteOnesColour[1] = g;
    minuteOnesColour[2] = b;

    if(nowTime.minute > 9) {
      lastH = tempH;
      while(!checkDifference(lastH, tempH, 75)) tempH = random(255);
      h2rgb(tempH, r, g, b);
      minuteTensColour[0] = r;
      minuteTensColour[1] = g;
      minuteTensColour[2] = b;
    }

    if(hourOnes != 0) {
      lastH = tempH;
      while(!checkDifference(lastH, tempH, 75)) tempH = random(255);
      h2rgb(tempH, r, g, b);
      hourOnesColour[0] = r;
      hourOnesColour[1] = g;
      hourOnesColour[2] = b;
    }

    lastH = tempH;
    while(!checkDifference(lastH, tempH, 75)) tempH = random(255);
    h2rgb(tempH, r, g, b);
    hourTensColour[0] = r;
    hourTensColour[1] = g;
    hourTensColour[2] = b;

    for(uint8_t i = 0; i < 36; i++)
      strip.setPixelColor(i, 0ul);

    uint32_t tempColour = 0;

    // Hours 10 digit...
    for (uint8_t i = 0; i < 6; ++i)
      ShuffleData[i] = (i < hourTens) ? 1 : 0;

    ShuffleIfNeeded(ShuffleData, 3, hourTens);
    tempColour = strip.Color(hourTensColour[0], hourTensColour[1], hourTensColour[2]);
    if(ShuffleData[0]) strip.setPixelColor(0, tempColour);
    if(ShuffleData[1]) strip.setPixelColor(23, tempColour);
    if(ShuffleData[2]) strip.setPixelColor(24, tempColour);

    // Hours 1 digit...
    for (uint8_t i = 0; i < 9; ++i)
      ShuffleData[i] = (i < hourOnes) ? 1 : 0;

    ShuffleIfNeeded(ShuffleData, 9, hourOnes);
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

    ShuffleIfNeeded(ShuffleData, 6, minuteTens);
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

    ShuffleIfNeeded(ShuffleData, 9, minuteOnes);
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
  else if(nowTime.second % 30 == 1) {
    alreadyRan = 1;
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

inline void ShuffleIfNeeded(uint8_t ShuffleData[9], uint8_t NumElems, uint8_t NumSetElems)
{
  // Don't bother shuffling when all elements have an identical value.
  if (NumSetElems != NumElems && NumSetElems != 0)
    DoShuffle(ShuffleData, NumElems);
}

void h2rgb(uint8_t Hint, uint8_t& R, uint8_t& G, uint8_t& B) {

  int var_i;
  float H, S=1, V=1, var_1, var_2, var_3, var_h, var_r, var_g, var_b;

  H = (float)Hint / 256.0;

  if ( S == 0 )                       //HSV values = 0 ÷ 1
  {
    R = V * 255;
    G = V * 255;
    B = V * 255;
  }
  else
  {
    var_h = H * 6;
    if ( var_h == 6 ) var_h = 0;      //H must be < 1
    var_i = int( var_h ) ;            //Or ... var_i = floor( var_h )
    var_1 = V * ( 1 - S );
    var_2 = V * ( 1 - S * ( var_h - var_i ) );
    var_3 = V * ( 1 - S * ( 1 - ( var_h - var_i ) ) );

    if      ( var_i == 0 ) {
      var_r = V     ;
      var_g = var_3 ;
      var_b = var_1 ;
    }
    else if ( var_i == 1 ) {
      var_r = var_2 ;
      var_g = V     ;
      var_b = var_1 ;
    }
    else if ( var_i == 2 ) {
      var_r = var_1 ;
      var_g = V     ;
      var_b = var_3 ;
    }
    else if ( var_i == 3 ) {
      var_r = var_1 ;
      var_g = var_2 ;
      var_b = V     ;
    }
    else if ( var_i == 4 ) {
      var_r = var_3 ;
      var_g = var_1 ;
      var_b = V     ;
    }
    else                   {
      var_r = V     ;
      var_g = var_1 ;
      var_b = var_2 ;
    }

    R = (1-var_r) * 255;                  //RGB results = 0 ÷ 255
    G = (1-var_g) * 255;
    B = (1-var_b) * 255;
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
