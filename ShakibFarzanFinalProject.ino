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
void setup() {
//  fillPersonelListOnEEPROM();
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
  if(isUpBtn() && tmpIdealTemp<255){
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

byte currentItemPersonel = 0;
void personelList(){
  printPersonelOnLCD();
  if (isOkBtn()){
    lcd.clear(); 
    frstNum = 0, secNum = 0, trdNum = 0, frtNum = 0;
    LCDState = enterPassCode;
    while(isOkBtn()){}
  }
  if(isUpBtn()){
    lcd.clear();
    if(currentItemPersonel > 0){
      currentItemPersonel--;
    }
    while(isUpBtn()){}
  }
  if(isDownBtn()){
    lcd.clear();
    if(currentItemPersonel < 13){
      currentItemPersonel++;
    }
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

byte getIdealTemp(){
  return EEPROM.read(112);
}

void setIdealTemp(byte temp){
  EEPROM.update(112, temp);
}

int getCurrentTemp(){
  return (long)analogRead(A1) * 500 / 1024;
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

//  AUTHOR: Rob Tillaart
// VERSION: 2013-09-01
// PURPOSE: experimental day of week code (hardly tested)
// Released to the public domain
#define LEAP_YEAR(Y)     ( (Y>0) && !(Y%4) && ( (Y%100) || !(Y%400) ))     // from time-lib

int dayOfWeek(int year, int month, int day)
{
  int months[] = {
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365         };   // days until 1st of month

  int days = year * 365;        // days until year 
  for (int i = 4; i < year; i += 4) if (LEAP_YEAR(i) ) days++;     // adjust leap years, test only multiple of 4 of course

  days += months[month-1] + day;    // add the days of this year
  if ((month > 2) && LEAP_YEAR(year)) days++;  // adjust 1 if this year is a leap year, but only after febr

  return days % 7;   // remove all multiples of 7
}

// --------------------------------------------------------
