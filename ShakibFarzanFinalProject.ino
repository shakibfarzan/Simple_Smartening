#include <Wire.h>
#include <DS3231.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>

#define LOCK 3
#define BUZZER 10
#define CLIENT_ROOM 0
#define WC 1
#define SALES_ROOM 2
#define MEETING_ROOM 3
#define MANAGER_ROOM 4
#define REPAIRS 5
#define PRODUCTION_ROOM 6
#define RESTAURANT 7
#define CORIDOR_SENSOR 3
#define RESTAURANT_SENSOR 4
#define SWITCH_LAMPS 7
#define FAN A3
#define WATER_PUMP 13
#define COOLER_MOTOR 12
#define COOLER_SPEED 11
#define HEATER 2
#define TEMP_SENSOR A1
#define TIMES_INDEX 113

LiquidCrystal lcd(8,9,4,5,6,7);
DS3231 rtc(A4, A5);

struct Person{
  unsigned long personalCode;
  unsigned int passCode;
  char accessbility;
  byte IO;
};

Person persons[14];
boolean isAllPersonelOut;
Time t;

//-----------LCDStates------------
void mainMenu();
void personelList();
void enterPassCode();
void settingMenu();
void editIdealTemp();
void weeklyProgramList();
void editTime();
void (*LCDState) (void) = mainMenu;
//--------------------------------

//---------Lighting System States---------
void coridorLampOff();
void coridorLampOn();
void coridorLampStayOn();
void (*coridorLampState) (void) = coridorLampOff;

void restaurantLampOff();
void restaurantLampOn();
void restaurantLampStayOn();
void (*restaurantLampState) (void) = restaurantLampOff;

//----------------------------------------

//----Air Conditioning System States----
void offAirSystem();
void onPump();
void onPump_onMotor();
void onMotor();
void onSpeed();
void onHeater();
void (*airState) (void) = offAirSystem;
//--------------------------------------

void setup() {
//  fillPersonelListOnEEPROM();
//  fillAllTimesToZero();
  fillPersonelList();
  lcd.begin(20,4);
  Serial.begin(9600);
  Serial.setTimeout(20000);
  rtc.begin();
  rtc.setDate(6,2,2022);
  rtc.setTime(11,13,8);
  pinMode(LOCK, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(FAN, OUTPUT);
  pinMode(WATER_PUMP, OUTPUT);
  pinMode(COOLER_MOTOR, OUTPUT);
  pinMode(COOLER_SPEED, OUTPUT);
  pinMode(HEATER, OUTPUT);
  updateIsAllPersonelOut();
  initTimerRTC();
}

void loop() {
  (*LCDState)();
  lightingSystem();
  printListPerHours();
  checkManagerRoom();
  checkSensors();
  getListFromSerial();
  off_onWCFan();
  airConditioningSystem();
  delay(50);
}

//---------------------Work with LCD----------------------
void printMenu(){
  lcd.setCursor(2,0);
  lcd.print("Current temp: ");
  lcd.print(getCurrentTemp());
  lcd.setCursor(2,1);
  lcd.print("Ideal temp: ");
  lcd.print(getIdealTemp());
  lcd.setCursor(2,2);
  lcd.print("Personel list");
  lcd.setCursor(2,3);
  lcd.print("Setting");
}

byte currentItemMain = 2;
byte cursorPlace = 0;
byte frstNum = 0, secNum = 0, trdNum = 0, frtNum = 0;
void mainMenu(){
  printMenu();
  lcd.setCursor(0,currentItemMain);
  lcd.print('>');
  cursorPlace = 0;
  if (isUpBtn() && currentItemMain == 3){
    currentItemMain--;
    lcd.clear();
    while(isUpBtn()){}
  }else if(isDownBtn() && currentItemMain == 2){
    currentItemMain++;
    lcd.clear();
    while(isDownBtn()){}
  }else if(isOkBtn() && currentItemMain == 2){
    lcd.clear();
    LCDState = personelList;
    while(isOkBtn()){}
  }else if(isOkBtn() && currentItemMain == 3){
    lcd.clear();
    frstNum = 0, secNum = 0, trdNum = 0, frtNum = 0;
    LCDState = enterPassCode;
    while(isOkBtn()){}
  }
}

byte currentSettingItem = 0;
byte tmpIdealTemp;
void settingMenu(){
  lcd.setCursor(2,0);
  lcd.print("Edit ideal temp");
  lcd.setCursor(2,1);
  lcd.print("Set weekly program");
  lcd.setCursor(0,currentSettingItem);
  lcd.print('>');
  if(isUpBtn() && currentSettingItem == 1){
    currentSettingItem--;
    lcd.clear();
    while(isUpBtn()){}
  }else if(isDownBtn() && currentSettingItem == 0){
    currentSettingItem++;
    lcd.clear();
    while(isDownBtn()){}
  }else if(isOkBtn() && currentSettingItem == 0){
    lcd.clear();
    currentSettingItem = 0;
    tmpIdealTemp = getIdealTemp();
    LCDState = editIdealTemp;
    while(isOkBtn()){}
  }else if(isOkBtn() && currentSettingItem == 1){
    lcd.clear();
    currentSettingItem = 0;
    LCDState = weeklyProgramList;
    while(isOkBtn()){}
  }else if(isCancelBtn()){
    lcd.clear();
    currentSettingItem = 0;
    LCDState = mainMenu;
    while(isCancelBtn()){}
  }
}

void editIdealTemp(){
  lcd.setCursor(2,0);
  lcd.print("Ideal Temp: ");
  lcd.print(tmpIdealTemp);
  if(isUpBtn() && tmpIdealTemp<99){
    tmpIdealTemp++;
    while(isUpBtn()){}
  }else if(isDownBtn() && tmpIdealTemp>0){
    tmpIdealTemp--;
    while(isDownBtn()){}
  }else if(isOkBtn()){
    setIdealTemp(tmpIdealTemp);
    LCDState = settingMenu;
    while(isOkBtn()){}
  }else if(isCancelBtn()){
    LCDState = settingMenu;
    while(isCancelBtn()){}
  }
}

byte currentDayItem = 0, hourOn, minOn, hourOff, minOff, cursorPlaceTime = 0; 
void weeklyProgramList(){
  printTimesOnLCD();
  if(isUpBtn() && currentDayItem > 0){
    lcd.clear();
    currentDayItem--;
    while(isUpBtn()){}
  }else if(isDownBtn() && currentDayItem < 6){
    lcd.clear();
    currentDayItem++;
    while(isDownBtn()){}
  }else if(isCancelBtn()){
    lcd.clear();
    LCDState = settingMenu;
    while(isCancelBtn()){}
  }else if(isOkBtn()){
    lcd.clear();
    int index = TIMES_INDEX + (currentDayItem*4);
    hourOn = EEPROM.read(index);
    minOn = EEPROM.read(index+1);
    hourOff = EEPROM.read(index+2);
    minOff = EEPROM.read(index+3); 
    cursorPlaceTime = 0;
    LCDState = editTime;
    while(isOkBtn()){}
  }
}

void editTime(){
    lcd.setCursor(7,0);
    lcd.print("ON");
    lcd.setCursor(13,0);
    lcd.print("OFF");
    lcd.setCursor(0,1);
    printRowWithoutEEPROM();
    if(isOkBtn()){
      setTimes(hourOn,minOn,hourOff,minOff,currentDayItem);
      lcd.clear();
      LCDState = weeklyProgramList;
      while(isOkBtn()){}
    }else if(isRightBtn() && cursorPlaceTime < 3){
      cursorPlaceTime++;
      while(isRightBtn()){}
    }else if(isLeftBtn() && cursorPlaceTime > 0){
      cursorPlaceTime--;
      while(isLeftBtn()){}
    }else if(isUpBtn()){
      if(cursorPlaceTime == 0 && hourOn < 24){
        hourOn++;
      }else if(cursorPlaceTime == 1 && minOn < 59){
        minOn++;
      }else if(cursorPlaceTime == 2 && hourOff < 24){
        hourOff++;
      }else if(cursorPlaceTime == 3 && minOff < 59){
        minOff++;
      }
      while(isUpBtn()){}
    }else if(isDownBtn()){
      if(cursorPlaceTime == 0 && hourOn > 0){
        hourOn--;
      }else if(cursorPlaceTime == 1 && minOn > 0){
        minOn--;
      }else if(cursorPlaceTime == 2 && hourOff > 0){
        hourOff--;
      }else if(cursorPlaceTime == 3 && minOff > 0){
        minOff--;
      }
      while(isDownBtn()){}
    }else if(isCancelBtn()){
      lcd.clear();
      LCDState = weeklyProgramList;
      while(isCancelBtn()){}
    }
}

byte currentItemPersonel = 0;
void personelList(){
  printPersonelOnLCD();
  if (isOkBtn()){
    lcd.clear(); 
    frstNum = 0, secNum = 0, trdNum = 0, frtNum = 0;
    LCDState = enterPassCode;
    while(isOkBtn()){}
  }
  if(isUpBtn() && currentItemPersonel > 0){
    lcd.clear();
    currentItemPersonel--;
    while(isUpBtn()){}
  }
  if(isDownBtn() && currentItemPersonel < 13){
    lcd.clear();
    currentItemPersonel++;
    while(isDownBtn()){}
  }
  if(isCancelBtn()){
    lcd.clear();
    LCDState = mainMenu;
    while(isCancelBtn()){}
  }
}

void enterPassCode(){
  lcd.setCursor(0,0);
  lcd.print("Enter passcode:");
  lcd.setCursor(0,1);
  lcd.print(frstNum);
  lcd.print(secNum);
  lcd.print(trdNum);
  lcd.print(frtNum);
  if(isRightBtn()){
    if(cursorPlace < 3){
      cursorPlace++;
    }
    while(isRightBtn()){}
  }
  if(isLeftBtn()){
    if(cursorPlace > 0){
      cursorPlace--;
    }
    while(isLeftBtn()){}
  }
  if(isUpBtn()){
    if(cursorPlace == 0 && frstNum < 9){
      frstNum++;
    }else if(cursorPlace == 1 && secNum < 9){
      secNum++;
    }else if(cursorPlace == 2 && trdNum < 9){
      trdNum++;
    }else if(cursorPlace == 3 && frtNum < 9){
      frtNum++;
    }
    while(isUpBtn()){}
  }
  if(isDownBtn()){
    if(cursorPlace == 0 && frstNum > 0){
      frstNum--;
    }else if(cursorPlace == 1 && secNum > 0){
      secNum--;
    }else if(cursorPlace == 2 && trdNum > 0){
      trdNum--;
    }else if(cursorPlace == 3 && frtNum > 0){
      frtNum--;
    }
    while(isDownBtn()){}
  }
  if(isOkBtn()){
    unsigned int pc = ((String(frstNum))+(String(secNum))+(String(trdNum))+(String(frtNum))).toInt();
    if(currentItemMain == 3){
      if(pc == getManager().passCode){
        lcd.clear();
        LCDState = settingMenu;
      }else{
        lcd.setCursor(0,2);
        lcd.print("Wrong passcode!");
      }
    }else {
      Person currentPerson = persons[currentItemPersonel];
      if(pc == currentPerson.passCode){
        String packet = "Person "+String(currentPerson.personalCode)+","+String(t.hour)+":"+String(t.min)+",";
        int item = currentItemPersonel * 7 + 7 + currentItemPersonel;
        if(currentPerson.IO == 1){
          packet+="OUT";
          persons[currentItemPersonel].IO = 0;
          EEPROM.update(item,0);
          updateIsAllPersonelOut();
        }else if(currentPerson.IO == 0){
          packet+="IN";
          persons[currentItemPersonel].IO = 1;
          EEPROM.update(item,1);
          isAllPersonelOut = false;
        }
        Serial.println(packet);
        lcd.clear();
        LCDState = mainMenu;
      }else{
        lcd.setCursor(0,2);
        lcd.print("Wrong passcode!");
      }
    }
    
    while(isOkBtn()){}
  }
  if(isCancelBtn()){
    lcd.clear();
    LCDState = mainMenu;
    while(isCancelBtn()){}
  }
}
//---------------------------------------------------------


//------------------print per hour list--------------------
long printTimer = 0;
void printListPerHours(){
  if(printTimer >= 72000){
    printTimer = 0;
    printPersonelListOnTerminal();
  }else{
    printTimer++;
  }
}
//---------------------------------------------------------

//---------------------Manager room lock-------------------
void checkManagerRoom(){
  Person manager = getManager();
  if(manager.IO == 1){
    digitalWrite(LOCK, HIGH);
  }else{
    digitalWrite(LOCK, LOW);
  }
}
//---------------------------------------------------------

//--------------------Safety System------------------------
void checkSensors(){
  String defaultWarn = "Warning: Unsecure-> in "; 
  Wire.requestFrom(0x26, 1);
  byte r = Wire.read();
  if(isAllPersonelOut){
    if(bitRead(r,CLIENT_ROOM) == 0){
      digitalWrite(BUZZER, HIGH);
      Serial.print(defaultWarn);
      Serial.println("client room");
    }
    if(bitRead(r,WC) == 0){
      digitalWrite(BUZZER, HIGH);
      Serial.print(defaultWarn);
      Serial.println("W.C");
    }
    if(bitRead(r,SALES_ROOM) == 0){
      digitalWrite(BUZZER, HIGH);
      Serial.print(defaultWarn);
      Serial.println("sales room");
    }
    if(bitRead(r,MEETING_ROOM) == 0){
      digitalWrite(BUZZER, HIGH);
      Serial.print(defaultWarn);
      Serial.println("meeting room");
    }
    if(bitRead(r,MANAGER_ROOM) == 0){
      digitalWrite(BUZZER, HIGH);
      Serial.print(defaultWarn);
      Serial.println("manager room");
    }
    if(bitRead(r,REPAIRS) == 0){
      digitalWrite(BUZZER, HIGH);
      Serial.print(defaultWarn);
      Serial.println("repairs");
    }
    if(bitRead(r,PRODUCTION_ROOM) == 0){
      digitalWrite(BUZZER, HIGH);
      Serial.print(defaultWarn);
      Serial.println("production room");
    }
    if(bitRead(r,RESTAURANT) == 0){
      digitalWrite(BUZZER, HIGH);
      Serial.print(defaultWarn);
      Serial.println("restaurant");
    }
  }
  if(getManager().IO == 0){
    if(bitRead(r,MANAGER_ROOM) == 0){
      digitalWrite(BUZZER, HIGH);
      Serial.print(defaultWarn);
      Serial.println("manager room");
    }
  }
}
//---------------------------------------------------------

//--------------------Lighting System----------------------
int stayingCoridorLampOn = 0;
void coridorLampOff(){
  Wire.requestFrom(0x25,1);
  byte r = Wire.read();
  if(bitRead(r,CORIDOR_SENSOR) == 0){
    coridorLampState = coridorLampOn;
  }else{
    Wire.beginTransmission(0x25);
    byte newIn = r & 254;
    Wire.write(newIn);
    Wire.endTransmission();
  }
}

void coridorLampOn(){
  stayingCoridorLampOn = 0;
  Wire.requestFrom(0x25,1);
  byte r = Wire.read();
  Wire.beginTransmission(0x25);
  byte newIn = r | 1;
  Wire.write(newIn);
  Wire.endTransmission();
  coridorLampState = coridorLampStayOn;
}

void coridorLampStayOn(){
  Wire.requestFrom(0x25,1);
  byte r = Wire.read();
  Wire.beginTransmission(0x25);
  byte newIn = r | 8;
  Wire.write(newIn);
  Wire.endTransmission();
  if(bitRead(r,CORIDOR_SENSOR) == 1){
    stayingCoridorLampOn++;
    if(stayingCoridorLampOn >= 100){
      coridorLampState = coridorLampOff;
    }
  }else{
    stayingCoridorLampOn = 0;
  }
}

int stayingRestaurantLampOn = 0;
void restaurantLampOff(){
  Wire.requestFrom(0x25,1);
  byte r = Wire.read();
  if(bitRead(r,RESTAURANT_SENSOR) == 0){
    restaurantLampState = restaurantLampOn;
  }else{
    Wire.beginTransmission(0x25);
    byte newIn = r & 253;
    Wire.write(newIn);
    Wire.endTransmission();
  }
}

void restaurantLampOn(){
  stayingRestaurantLampOn = 0;
  Wire.requestFrom(0x25,1);
  byte r = Wire.read();
  Wire.beginTransmission(0x25);
  byte newIn = r | 2;
  Wire.write(newIn);
  Wire.endTransmission();
  restaurantLampState = restaurantLampStayOn;
}

void restaurantLampStayOn(){
  Wire.requestFrom(0x25,1);
  byte r = Wire.read();
  Wire.beginTransmission(0x25);
  byte newIn = r | 16;
  Wire.write(newIn);
  Wire.endTransmission();
  if(bitRead(r,RESTAURANT_SENSOR) == 1){
    stayingRestaurantLampOn++;
    if(stayingRestaurantLampOn >= 200){
      restaurantLampState = restaurantLampOff;
    }
  }else{
    stayingRestaurantLampOn = 0;
  }
}

void lightingSystem(){
  if(isAllPersonelOut){
    Wire.beginTransmission(0x25);
    Wire.write(248);
    Wire.endTransmission();
  }else{
    (*coridorLampState)();
    (*restaurantLampState)();
    Wire.requestFrom(0x25,1);
    byte r = Wire.read();
    Wire.beginTransmission(0x25);
    byte newIn;
    if(bitRead(r,SWITCH_LAMPS) == 0){
      newIn = r | 132;
    }else{
      newIn = r & 251;
    }
    Wire.write(newIn);
    Wire.endTransmission();
  }
}
//---------------------------------------------------------

//--------------------Input in serial----------------------
void getListFromSerial(){
  if(Serial.available()){
    String cmd = Serial.readStringUntil('\r');
    Serial.println(cmd);
    if(cmd.equals("SET")){
      printPersonelListOnTerminal();
    }else{
      Serial.println("Wrong input!");
      Serial.println("You can input 'SET' to get personel list");
    }
  }
}
//---------------------------------------------------------

//------------------------WC Fan---------------------------
int fanTimer = 0;
void off_onWCFan(){
  if(isAllPersonelOut){
    fanTimer++;
    if(fanTimer >= 140){
      digitalWrite(FAN, LOW);
    }
  }else{
    fanTimer = 0;
    digitalWrite(FAN, HIGH);
  }
}
//---------------------------------------------------------

//----------------Air conditioning system------------------
void airConditioningSystem(){
  
  if(!isAllPersonelOut || isCorrectTime()){
    (*airState)();
  }else{
    digitalWrite(WATER_PUMP,LOW);
    digitalWrite(COOLER_MOTOR, LOW);
    digitalWrite(COOLER_SPEED, LOW);
    digitalWrite(HEATER,LOW);
  }
}

boolean isCorrectTime(){
  int weekday = dayOfWeek(t.date, t.mon, t.year);
  byte p = weekday * 4;
  byte ho = EEPROM.read(TIMES_INDEX + p);
  byte mo = EEPROM.read(TIMES_INDEX + p + 1);
  byte hof = EEPROM.read(TIMES_INDEX + p + 2);
  byte mof = EEPROM.read(TIMES_INDEX + p + 3);

  return (t.hour > ho && t.hour < hof) || (t.hour == ho && t.min >= mo) || (t.hour == hof && t.min < mof);
}

void offAirSystem(){
  
}
//---------------------------------------------------------

// ---------------------Intitialize------------------------
void fillPersonelListOnEEPROM(){
// fake data
  int j = 0;
  for (int i = 0; i < 112; i+=8){
    unsigned long personalCode = 100000 + j;
    unsigned int passCode = 1000 + j;
    char accessbility;
    if (i == 0) {
      accessbility = 'M';
    }else {
      accessbility = 'A';
    }
    EEPROM.update(i,personalCode & 0xFF);
    EEPROM.update(i+1,(personalCode>>8) & 0xFF);
    EEPROM.update(i+2,(personalCode>>16) & 0xFF);
    EEPROM.update(i+3,(personalCode>>24) & 0xFF);
    byte hpac = highByte(passCode);
    byte lpac = lowByte(passCode);
    EEPROM.update(i+4,hpac);
    EEPROM.update(i+5,lpac);
    EEPROM.update(i+6,accessbility);
    EEPROM.update(i+7,0);
    j++;
  }
}

void fillPersonelList(){
  int j = 0;
  for (int i = 0; i < 112; i+=8){
    Person p;
    byte a1 = EEPROM.read(i);
    byte a2 = EEPROM.read(i+1);
    byte a3 = EEPROM.read(i+2);
    byte a4 = EEPROM.read(i+3);
    p.personalCode = (unsigned long)a4 << 24 | (unsigned long)a3 << 16 | (unsigned long)a2 << 8 | (unsigned long)a1;
    byte hpac = EEPROM.read(i+4);
    byte lpac = EEPROM.read(i+5);
    p.passCode = word(hpac,lpac);
    p.accessbility = (char)EEPROM.read(i+6);
    p.IO = EEPROM.read(i+7);
    persons[j++] = p;
  }
}

void fillAllTimesToZero(){
  for(int i = 113; i < 142; i++){
    EEPROM.update(i,0);
  }
}

byte getIdealTemp(){
  return EEPROM.read(112);
}

void setIdealTemp(byte temp){
  EEPROM.update(112, temp);
}

int getCurrentTemp(){
  return (long)analogRead(A1) * 500 / 1024;
}

Person getManager(){
//  with searching by loop
//  for(int i = 0; i < 14; i++){
//    if(persons[i].accessbility == 'M'){
//      return persons[i];
//    }
//  }
//  get from specific index
  return persons[0];
} 

// --------------------------------------------------------

// ------------------------Utils---------------------------
boolean isRightBtn(){
  return analogRead(A0) < 145;
}
boolean isUpBtn(){
  return analogRead(A0) >= 145 && analogRead(A0) < 330;
}
boolean isDownBtn(){
  return analogRead(A0) >= 330 && analogRead(A0) < 506;
}
boolean isLeftBtn(){
  return analogRead(A0) >= 506 && analogRead(A0) < 741;
}
boolean isOkBtn(){
  return analogRead(A0) >= 741 && analogRead(A0) < 745;
}
boolean isCancelBtn(){
  return analogRead(A0) >= 830 && analogRead(A0) < 1023;
}

void setTimes(byte onHour, byte onMin, byte offHour, byte offMin, byte weekday){
  int p = weekday * 4;
  EEPROM.write(TIMES_INDEX + p, onHour);
  EEPROM.write(TIMES_INDEX + p + 1, onMin);
  EEPROM.write(TIMES_INDEX + p + 2, offHour);
  EEPROM.write(TIMES_INDEX + p + 3, offMin);
}

void printTimesOnLCD(){
  lcd.setCursor(7,0);
  lcd.print("ON");
  lcd.setCursor(13,0);
  lcd.print("OFF");
  if(currentDayItem < 3){
    lcd.setCursor(0, currentDayItem+1);
    lcd.print('>');
    lcd.setCursor(1,1);
    printRow(0);
    lcd.setCursor(1,2);
    printRow(1);
    lcd.setCursor(1,3);
    printRow(2);
  }else if(currentDayItem < 6){
    lcd.setCursor(0, currentDayItem-2);
    lcd.print('>');
    lcd.setCursor(1,1);
    printRow(3);
    lcd.setCursor(1,2);
    printRow(4);
    lcd.setCursor(1,3);
    printRow(5);
  }else if(currentDayItem == 6){
    lcd.setCursor(0,1);
    lcd.print('>');
    lcd.setCursor(1,1);
    printRow(6);
  }
}

void printRow(byte day){
  int index = TIMES_INDEX + (day*4);
  String weekday = getWeekdayName(day);
  byte ho = EEPROM.read(index);
  byte mo = EEPROM.read(index+1);
  byte hof = EEPROM.read(index+2);
  byte mof = EEPROM.read(index+3);
  lcd.print(weekday);
  lcd.print(": ");
  if(ho < 10){
    lcd.print("0");
  }
  lcd.print(ho);
  lcd.print(":");
  if(mo < 10){
    lcd.print("0");
  }
  lcd.print(mo);
  lcd.print(" ");
  if(hof < 10){
    lcd.print("0");
  }
  lcd.print(hof);
  lcd.print(":");
  if(mof < 10){
    lcd.print("0");
  }
  lcd.print(mof);
}

void printRowWithoutEEPROM(){
  String weekday = getWeekdayName(currentDayItem);
  lcd.print(weekday);
  lcd.print(": ");
  if(hourOn < 10){
    lcd.print("0");
  }
  lcd.print(hourOn);
  lcd.print(":");
  if(minOn < 10){
    lcd.print("0");
  }
  lcd.print(minOn);
  lcd.print(" ");
  if(hourOff < 10){
    lcd.print("0");
  }
  lcd.print(hourOff);
  lcd.print(":");
  if(minOff < 10){
    lcd.print("0");
  }
  lcd.print(minOff);
}

String getWeekdayName(byte day){
  switch(day){
    case 0: return "SUN";
    case 1: return "MON";
    case 2: return "TUE";
    case 3: return "WED";
    case 4: return "TUR";
    case 5: return "FRI";
    case 6: return "SAT";
  }
}

void printPersonelOnLCD(){
  if(currentItemPersonel < 4){
    lcd.setCursor(0, currentItemPersonel);
    lcd.print('>');
    for (int i = 0; i < 4; i++){
      lcd.setCursor(2, i);
      lcd.print(i+1);
      lcd.print('.');
      lcd.print(persons[i].personalCode);
    }
  } else if(currentItemPersonel < 8){
    lcd.setCursor(0, currentItemPersonel - 4);
    lcd.print('>');
    for (int i = 4; i < 8; i++){
      lcd.setCursor(2, i - 4);
      lcd.print(i+1);
      lcd.print('.');
      lcd.print(persons[i].personalCode);
    }
  } else if(currentItemPersonel < 12){
    lcd.setCursor(0, currentItemPersonel - 8);
    lcd.print('>');
    for (int i = 8; i < 12; i++){
      lcd.setCursor(2, i - 8);
      lcd.print(i+1);
      lcd.print('.');
      lcd.print(persons[i].personalCode);
    }
  } else if(currentItemPersonel < 14){
    lcd.setCursor(0, currentItemPersonel - 12);
    lcd.print('>');
    for (int i = 12; i < 14; i++){
      lcd.setCursor(2, i - 12);
      lcd.print(i+1);
      lcd.print('.');
      lcd.print(persons[i].personalCode);
    }
  }
}

void printPersonelListOnTerminal(){
  String list = "SET ";
  for(int i = 0; i < 14; i++){
    list+=String(persons[i].personalCode)+","+String(persons[i].passCode)+","+String(persons[i].accessbility)+",";
  }
  Serial.println(list);
}

void updateIsAllPersonelOut(){
  for(int i = 0; i < 14; i++){
    if(persons[i].IO == 1){
      isAllPersonelOut = false;
      return;
    }
  }
  isAllPersonelOut = true;
}

void initTimerRTC(){
  TCNT1 = 0;
  OCR1A = 15625 - 1;
  TCCR1A = 0;
  TCCR1B = 0x0D;

  TIMSK1 = (1<<OCIE1A);
  interrupts();
}

ISR(TIMER1_COMPA_vect){
  t = rtc.getTime();
}

int dayOfWeek(int d, int m, int y)
{
    static int t[] = { 0, 3, 2, 5, 0, 3,
                       5, 1, 4, 6, 2, 4 };
    y -= m < 3;
    return ( y + y / 4 - y / 100 +
             y / 400 + t[m - 1] + d) % 7;
}

// --------------------------------------------------------
