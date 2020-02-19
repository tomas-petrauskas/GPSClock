#include <Arduino.h>

#include <Timezone.h>
#include <TM1637Display.h>

#include <NeoSWSerial.h>
#include <NMEAGPS.h>

// ------------------------ Application Settings ---------------------- //
#define SHOW_ZERO_IN_HOURS false
#define BLINK_SEPARATOR_ON_GPS_DATA_INPUT true

// Brightness level 0..7
#define LCD_BRIGHTNESS 7

// ------------------------- Lithuania TimeZone ----------------------- //

TimeChangeRule SummerTime = {"Sum", Last, Sun, Mar, 3, 180};
TimeChangeRule WinterTime = {"Win", Last, Sun, Oct, 4, 120};
Timezone timeZone(SummerTime, WinterTime);

// -------------------------------------------------------------------- //

// GPS Module
NMEAGPS gps;
gps_fix fix;
uint8_t awayCount = 100;
NeoSWSerial serialgps(10,11);

// Display Module
#define CLK 9
#define DIO 8
uint8_t displayData[] = { 0xff, 0xff, 0xff, 0xff };
uint8_t displayDataAllSegments[] = { 0xff, 0xff, 0xff, 0xff };
TM1637Display display(CLK, DIO);

int8_t TimeDisp[] = {0x00,0x00,0x00,0x00};
bool SeparatorSymbolValue = true;
unsigned char displayMinute = 88;
unsigned char displayHour = 88;

void countAway() {
  if(awayCount > 5) {
    display.setSegments(displayDataAllSegments);
  }

  if(awayCount < 10) {
    awayCount++;
  }
}

time_t tmConvert_t(int YYYY, byte MM, byte DD, byte hh, byte mm, byte ss = 0)
{
  tmElements_t tmSet;
  tmSet.Year = YYYY - 1970;
  tmSet.Month = MM;
  tmSet.Day = DD;
  tmSet.Hour = hh;
  tmSet.Minute = mm;
  tmSet.Second = ss;
  return makeTime(tmSet);
}

void updateTime() {
  time_t timeObject = tmConvert_t(fix.dateTime.year, fix.dateTime.month, fix.dateTime.date, fix.dateTime.hours, fix.dateTime.minutes, fix.dateTime.seconds);
  time_t local = timeZone.toLocal(timeObject);
  displayHour = hour(local);
  displayMinute = fix.dateTime.minutes;
}

void setupTimers() {
  cli();

  // set timer1 interrupt at 1Hz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 1hz increments
  OCR1A = 15624;// = (16*10^6) / (1*1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS12 and CS10 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);

  sei();
}

void displaySetup() {
  display.setBrightness(LCD_BRIGHTNESS);
}

ISR(TIMER1_COMPA_vect) {
  countAway();
}

void displayUpdate(void)
{
  if(displayHour > 9 || SHOW_ZERO_IN_HOURS) {
    displayData[0] = display.encodeDigit(displayHour / 10);
  } else {
    displayData[0] = 0;
  }

  displayData[1] = display.encodeDigit(displayHour % 10);

  if(SeparatorSymbolValue) {
    displayData[1] = displayData[1] | 128;
  }

  displayData[2] = display.encodeDigit(displayMinute / 10);
  displayData[3] = display.encodeDigit(displayMinute % 10);
  display.setSegments(displayData);
}

void setup()
{
 displaySetup();
 setupTimers();
 serialgps.begin(9600);
}

void loop()
{
 if (gps.available( serialgps )) 
 {
  fix = gps.read();
  if (fix.valid.time && fix.valid.date)
  {
    updateTime();    
    awayCount = 0;

    if (BLINK_SEPARATOR_ON_GPS_DATA_INPUT) {
      SeparatorSymbolValue = !SeparatorSymbolValue;
    }
  
    displayUpdate();
  }
 }
}