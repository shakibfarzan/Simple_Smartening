#include <Wire.h>
#include <DS3231.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>
#define Lock 3
#define Buzzer 10
#define clientRoom 0
#define wc 1
#define salesRoom 2
#define meetingRoom 3
#define managerRoom 4
#define repairs 5
#define productionRoom 6
#define restaurant 7
LiquidCrystal lcd(8,9,4,5,6,7);
DS3231 rtc(A4, A5);

struct Person{
  unsigned long personalCode;
  unsigned int passCode;
  char accessbility;
  byte IO;
};

Person persons[14];

//-----------LCDStates------------
void mainMenu();
void personelList();
void enterPassCode();
void (*LCDState) (void) = mainMenu;
//--------------------------------

void setup() {
//  fillPersonelListOnEEPROM();
  fillPersonelList();
  lcd.begin(20,4);
  Serial.begin(9600);
  Serial.setTimeout(20000);
  rtc.begin();
  rtc.setDate(6,2,2022);
  rtc.setTime(11,13,8);
  pinMode(Lock, OUTPUT);
  pinMode(Buzzer, OUTPUT);
}

void loop() {
  (*LCDState)();
  printListPerHours();
  checkManagerRoom();
  checkSensors();
  delay(50);
}

//---------------------Work with LCD----------------------
int currentItemMain = 0;
int cursorPlace = 0;
void mainMenu(){
  lcd.setCursor(0,0);
  cursorPlace = 0;
  if (isUpBtn()){
    currentItemMain++;
    while(isUpBtn()){}
  }else if(isDownBtn()){
    currentItemMain--;
    while(isDownBtn()){}
  }else if(isOkBtn() && currentItemMain == 1){
    lcd.clear();
    LCDState = personelList;
    while(isOkBtn()){}
  }
  lcd.print(currentItemMain);
}

int currentItemPersonel = 0;
byte frstNum = 0, secNum = 0, trdNum = 0, frtNum = 0;
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
    Person currentPerson = persons[currentItemPersonel];
    if(pc == currentPerson.passCode){
      Time t = rtc.getTime();
      String packet = "Person "+String(currentPerson.personalCode)+","+String(t.hour)+":"+String(t.min)+",";
      int item = currentItemPersonel * 7 + 7 + currentItemPersonel;
      if(currentPerson.IO == 1){
        packet+="OUT";
        persons[currentItemPersonel].IO = 0;
        EEPROM.update(item,0);
      }else if(currentPerson.IO == 0){
        packet+="IN";
        persons[currentItemPersonel].IO = 1;
        EEPROM.update(item,1);
      }
      Serial.println(packet);
      lcd.clear();
      LCDState = mainMenu;
    }else{
      lcd.setCursor(0,2);
      lcd.print("Wrong passcode!");
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
  if(printTimer >= 200){
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
    digitalWrite(Lock, HIGH);
  }else{
    digitalWrite(Lock, LOW);
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
  if(isAllPersonelOut()){
    if(bitRead(r,clientRoom) == 0){
      digitalWrite(Buzzer, HIGH);
      Serial.print(defaultWarn);
      Serial.println("client room");
    }
    if(bitRead(r,wc) == 0){
      digitalWrite(Buzzer, HIGH);
      Serial.print(defaultWarn);
      Serial.println("W.C");
    }
    if(bitRead(r,salesRoom) == 0){
      digitalWrite(Buzzer, HIGH);
      Serial.print(defaultWarn);
      Serial.println("sales room");
    }
    if(bitRead(r,meetingRoom) == 0){
      digitalWrite(Buzzer, HIGH);
      Serial.print(defaultWarn);
      Serial.println("meeting room");
    }
    if(bitRead(r,managerRoom) == 0){
      digitalWrite(Buzzer, HIGH);
      Serial.print(defaultWarn);
      Serial.println("manager room");
    }
    if(bitRead(r,repairs) == 0){
      digitalWrite(Buzzer, HIGH);
      Serial.print(defaultWarn);
      Serial.println("repairs");
    }
    if(bitRead(r,productionRoom) == 0){
      digitalWrite(Buzzer, HIGH);
      Serial.print(defaultWarn);
      Serial.println("production room");
    }
    if(bitRead(r,restaurant) == 0){
      digitalWrite(Buzzer, HIGH);
      Serial.print(defaultWarn);
      Serial.println("restaurant");
    }
  }
  if(getManager().IO == 0){
    if(bitRead(r,managerRoom) == 0){
      digitalWrite(Buzzer, HIGH);
      Serial.print(defaultWarn);
      Serial.println("manager room");
    }
  }
}

boolean isAllPersonelOut(){
  for(int i = 0; i < 14; i++){
    if(persons[i].IO == 1){
      return false;
    }
  }
  return true;
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
// --------------------------------------------------------
