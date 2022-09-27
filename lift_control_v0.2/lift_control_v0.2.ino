
/*
  Change Log
  v0.1:
  - made base code

  v0.2:
  - added functions of buzzer, led, relay
  - discarded rfid UID delete function (erase mode).
  In future will be add smart deleting fuction.

  v0.2.1
  - made some code arrangement
  - made simpleDef.h header file.
  - normalModeOn name changed to idleMode.
  - Corrected RGB LED pin numbers
  - Added some led and buzzer animation:
    - Authorized card animation
    - Unauthorized card animation
    - Program mode animation
    - Idle mode animation
    - RFID reader error animation
  ++ Yapilacaklar
  + Led arayuzunu olustur --%65--
  + buzzer ve relay ayarlarini duzenle
  + sisteme ne gibi esneklik katilabilir

  V0.3.0
  - added external EEPROM functions and libraries
  - revise eeprom functions
  - rezerve edilen adres sayısı baştan 12'ye çıkarıldı.
  - bir kartı iki defa kayıt etmekte düzeltilecek.
*/

#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>     // We are going to read and write PICC's UIDs from/to EEPROM
#include <Wire.h>
/*---------------------------------------------------------------*/
typedef uint8_t u8;
//typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
/*---------------------------------------------------------------*/
#define EXT_EEPROM 1
//#define ADDR_Ax 0b000 //A2, A1, A0
#define ADDR 0x50//(0b1010 << 3) + ADDR_Ax
/*---------------------------------------------------------------*/
#define BUZZER_PIN 8
/*---------------------------------------------------------------*/
#define RELAY_PIN 7
#define ACIK_SURE 0
#define ACIK_100MS 5
/*---------------------------------------------------------------*/
/*RFID Reader Rc522 modules pins
   RST  PIN  9
   SDA  PIN 10
   SCK  PIN 13
   MOSI PIN 11
   MISO PIN 12
   IRQ  PIN --
*/
#define SS_PIN 10
#define RST_PIN 9
/*---------------------------------------------------------------*/
#define LED_R 6
#define LED_G 5
#define LED_B 3
/*---------------------------------------------------------------*/
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
//MFRC522::MIFARE_Key key;
/*---------------------------------------------------------------*/
u32 lastReadTime;
u8 successRead; // Variable integer to keep if we have Successful Read from Reader
byte storedCard[4]; // Stores an ID read from EEPROM
byte readCard[4];   // Stores scanned ID read from RFID Module
byte lastReadCard[4]; //Store previous scanned ID read from RFID Module
byte masterCard[4] = {183, 70, 146, 53}; // Stores master card's ID read from EEPROM
//mastercard UID: B7 46 92 35 - 183 70 146 53

/*---------------------------------------------------------------*/
bool BUZZER_UP = true;
bool programMode = false;  // initialize programming mode to programMode
u32 progModeBeginTime = 0;
u16 maxProgModeTime = 10;
bool eraseMode = false;    // initialize programming mode to erase mode
/*---------------------------------------------------------------*/
void setup() {
  initEeprom();
  initLed();
  initBuzzer();
  initRelay();
  Serial.begin(9600);
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522
  Serial.println("Asansor Kat Kontrol v0.3.2");
  buzzerBip(2, 250);
  ShowReaderDetails();
  /*WipeMode not necessary now*/
  /*Mastercard not store eeprom now, hardcoded in program */
  Serial.println(F("-------------------"));
  Serial.println(F("MasterCard's UID"));
  for ( uint8_t i = 0; i < 4; i++ ) {          // Read Master Card's UID from EEPROM
    //masterCard[i] = readEeprom(2 + i);    // Write it to masterCard[]
    Serial.print(masterCard[i], HEX);
  }
  buzzerBip(1, 500);
  /*End of init sequence*/
  Serial.println("");
  Serial.println(F("-------------------"));
  Serial.println(F("Her sey Hazir!"));
  Serial.println(F("RFID kartin okutulmasi bekleniliyor."));
  Serial.println(F("-----------------------------"));
  buzzerBip(3, 250);
  lastReadTime = 0;
}

void loop() {
  do {
    successRead = getID();  // sets successRead to 1 when we get read from reader otherwise 0
    if (programMode) {
      ledMagenta();              // Program Mode light Magenta waiting to read a new card
      if ((millis() - progModeBeginTime) > (maxProgModeTime * 1000)) {
        programMode = false;
        Serial.println(F("Program Mode suresi bitti, cikiliyor!"));
        buzzerBip(5, 300);
        Serial.println(F("-----------------------------"));
        Serial.println(F("RFID kartin okutulmasi bekleniliyor."));

      }
    }
    if (!programMode) {
      idleMode();     // Normal mode, blue Power LED is on, all others are off
    }
  }
  while (!successRead);//the program will not go further while you are not getting a successful read


  if (programMode) {
    if ( isMaster(readCard) /*&& !eraseMode*/ ) { //When in program mode check First If master card scanned again to (not change erase mode) exit
      Serial.println(F("Master Kart tekrar okutuldu."));
      //Serial.println(F("Kayit silmek icin lütfen kayitli bir kart okutun"));
      //Serial.println(F("-----------------------------"));
      //eraseMode = true;
      programMode = false;
      Serial.println(F("Program moddan cikiliyor!"));
      buzzerBip(3, 300);
      return;
    }
    /*Bu bölumde eraseMode surekli false olacagından silme moduna hic girmeyecek
       asagidaki kismi askiya almaya gerek kalmadi.
    */
    eraseMode = false;
    if ( eraseMode ) { // If eraseMode true delete it
      /*
          if(isMaster(readCard)){
         Serial.print("Master kart silinemez!");
        }
        else{
         if(findID(readCard)){
           Serial.println(F("Okutulan kart kayitli, siliniyor..."));
           deleteID(readCard);
           Serial.println(F("Kart basariyla silindi."));
         }
         else{
           Serial.println(F("Kart kayitli degil silinemiyor."));
         }
        }

        Serial.println(F("Silme modundan cikiliyor."));
        Serial.println("-----------------------------");
        eraseMode = false;
        programMode = false;
        return;
      */
    }/*Bu kısma kadar*/
    else {                    // If scanned card is not known add it
      if (findID(readCard)) {
        Serial.println(F("Kart daha onceden kaydedilmis."));
        buzzerOn();
        ledRgbW(150, 0, 0, 500);
        buzzerOff();
        progModeBeginTime = millis();
      }
      else {
        //Serial.println(F("This PICC UID not exist, registering..."));
        Serial.println(F("Bu PICC UID mevcut degil, kaydediliyor..."));
        writeID(readCard);
        //Serial.println(F("Kart basariyla kaydedildi."));
        buzzerOn();
        ledRgbW(0, 150, 0, 500);
        buzzerOff();
        progModeBeginTime = millis();
      }
      /*
        Serial.println(F("Kayit modundan cikiliyor."));
        Serial.println(F("-----------------------------"));
        programMode = false; //if we want exit prog mode after added id
      */
    }
  }
  else {
    if ( isMaster(readCard)) {    // If scanned card's ID matches Master Card's ID - enter program mode
      programMode = true;
      progModeBeginTime = millis();
      Serial.println(F("Masterkart okutuldu - Program Modu aktif"));
      u8 count = readEeprom(0);   // Read the first Byte of EEPROM that
      Serial.print(F("Hafizada kayitli "));     // stores the number of ID's in EEPROM
      Serial.print(count);
      Serial.print(F(" adet kart var."));
      Serial.println("");
      Serial.println(F("Kayit edilecek karti okutun."));
      //Serial.println(F("Kayitli karti silmek icin Masterkarti tekrar okutun."));
      Serial.println(F("-----------------------------"));
      cycleLed(500);
    }
    else {
      if ( findID(readCard) ) { // If not, see if the card is in the EEPROM
        Serial.println(F("Gecis Onaylandi"));
        granted((ACIK_SURE * 1000) + (ACIK_100MS * 100));   // Open the door lock for 3000 ms
        Serial.println(F("-----------------------------"));
      }
      else {      // If not, show that the ID was not valid
        Serial.println(F("Gecis Onaylanmadi"));
        denied();
        Serial.println(F("-----------------------------"));
      }
    }
  }
}

////////////////////////////////////////// Functions //////////////////////////////////////////////////

void ShowReaderDetails() {
  // Get the MFRC522 software version
  byte v = rfid.PCD_ReadRegister(rfid.VersionReg);
  Serial.print(F("MFRC522 Software Version: 0x"));
  Serial.print(v, HEX);
  if (v == 0x91)
    Serial.print(F(" = v1.0"));
  else if (v == 0x92)
    Serial.print(F(" = v2.0"));
  else
    Serial.print(F(" (unknown),probably a chinese clone?"));
  Serial.println("");
  // When 0x00 or 0xFF is returned, communication probably failed
  if ((v == 0x00) || (v == 0xFF)) {
    Serial.println(F("WARNING: Communication failure, is the MFRC522 properly connected?"));
    Serial.println(F("SYSTEM HALTED: Check connections."));
    // Visualize system is halted
    /*    digitalWrite(greenLed, LED_OFF);  // Make sure green LED is off
        digitalWrite(blueLed, LED_OFF);   // Make sure blue LED is off
        digitalWrite(redLed, LED_ON);   // Turn on red LED
    */    while (true) { // do not go further
      u16 interval = 500;
      u16 timeVal = millis() % interval;
      if (timeVal > 0 && timeVal < 10) {
        ledRed();
      }
      else if (timeVal > (interval / 2) - 10 && timeVal < (interval / 2) + 10) {
        ledRgbOff();
      }
    }
  }
}

///////////////////////////////////////// Get PICC's UID ///////////////////////////////////
u8 getID() {
  // Getting ready for Reading PICCs
  if ( ! rfid.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
    return 0;
  }
  if ( ! rfid.PICC_ReadCardSerial()) {   //Since a PICC placed get Serial and continue
    return 0;
  }
  // There are Mifare PICCs which have 4 byte or 7 byte UID care if you use 7 byte PICC
  // I think we should assume every PICC as they have 4 byte UID
  // Until we support 7 byte PICCs
  Serial.println(F("Scanned PICC's UID:"));
  for ( u8 i = 0; i < 4; i++) {  //
    lastReadCard[i] = readCard[i];
    readCard[i] = rfid.uid.uidByte[i];
    Serial.print(readCard[i], HEX);
  }
  rfid.PICC_HaltA(); // Stop reading
  Serial.println("");
  if (isSame() && millis() - lastReadTime < 1500) {
    Serial.println("Ardarda cok fazla okutma yapildi");
    return 0;
  }
  else {
    lastReadTime = millis();
    return 1;
  }
}

//////////////////////////////////////// idle mode  ///////////////////////////////////
void idleMode() {/*aka normalModeOn*/
  //Serial.println("Idle Mode");
  relayOff();// Make sure Door is Locked
  //ledBlue();
  ledBlue();
}

//////////////////////////////////////// EEPROM /////////////////////////////////////
void initEeprom() {
  if (EXT_EEPROM) {
    Wire.begin();
  }

}

void writeEeprom(u8 address, u8 data) {
  if (EXT_EEPROM) {
    Wire.beginTransmission(ADDR);
    Wire.write(address);
    Wire.write(data);
    Wire.endTransmission();
    delay(10);
  }
  else if (!EXT_EEPROM) {
    EEPROM.write(address, data);
  }

}

u8 readEeprom(u8 address) {
  u8 data = NULL;
  if (EXT_EEPROM) {
    Wire.beginTransmission(ADDR);
    Wire.write(address);
    Wire.endTransmission();
    Wire.requestFrom(ADDR, 1); //retrive 1 returned byte
    delay(1);
    if (Wire.available()) {
      data = Wire.read();
    }
  }
  else if (!EXT_EEPROM) {
    data = EEPROM.read(address);
  }
  return data;
}

//////////////////////////////////////// Read an ID from EEPROM //////////////////////////////
void readID( u8 number ) {
  u8 start = (number * 4 ) + 12;    // Figure out starting position - ilk 12 adres kart sayısı için rezerve edilmiştir.
  for ( u8 i = 0; i < 4; i++ ) {     // Loop 4 times to get the 4 Bytes
    storedCard[i] = readEeprom(start + i);   // Assign values read from EEPROM to array
  }
}

///////////////////////////////////////// Add ID to EEPROM   ///////////////////////////////////
void writeID( byte a[] ) {
  byte isWrited[4];
  if ( !findID( a ) ) {     // Before we write to the EEPROM, check to see if we have seen this card before!
    u8 num = readEeprom(0);     // Get the numer of used spaces, position 0 stores the number of ID cards
    u8 start = ( num * 4 ) + 12;  // Figure out where the next slot starts - ilk 12 adres kart sayısı için rezerve edilmiştir.

    for ( u8 j = 0; j < 4; j++ ) {   // Loop 4 times
      writeEeprom( start + j, a[j] );  // Write the array values to EEPROM in the right position
    }
    for ( u8 k = 0; k < 4; k++ ) {     // Loop 4 times to get the 4 Bytes
      isWrited[k] = readEeprom(start + k);   // Assign values read from EEPROM to array
    }
    if (checkTwo(a, isWrited)) {
      num++;                // Increment the counter by one            //burası sona inecek
      writeEeprom( 0, num );     // Write the new count to the counter //burasıda onaylandıktan sonra artacak
      Serial.print(F("Kayit Basarili. Toplam Kayitli kart adedi: "));
      Serial.println(num);
    }
    else {
      Serial.println(F("Kayit Basarisis"));
    }

    //successWrite();
    //Serial.println(F("Succesfully added ID record to EEPROM"));
  }
  else {
    //failedWrite();
    Serial.println(F("Failed! There is something wrong with ID or bad EEPROM"));
  }
}

///////////////////////////////////////// Remove ID from EEPROM   ///////////////////////////////////
void deleteID( byte a[] ) {
  if ( !findID( a ) ) {     // Before we delete from the EEPROM, check to see if we have this card!
    //failedWrite();      // If not
    Serial.println(F("Failed! There is something wrong with ID or bad EEPROM"));
  }
  else {
    u8 num = readEeprom(0);   // Get the numer of used spaces, position 0 stores the number of ID cards
    u8 slot;       // Figure out the slot number of the card
    u8 start;      // = ( num * 4 ) + 6; // Figure out where the next slot starts
    u8 looping;    // The number of times the loop repeats
    u8 j;
    u8 count = readEeprom(0); // Read the first Byte of EEPROM that stores number of cards
    slot = findIDSLOT( a );   // Figure out the slot number of the card to delete
    start = (slot * 4) + 2;
    looping = ((num - slot) * 4);
    num--;      // Decrement the counter by one
    writeEeprom( 0, num );   // Write the new count to the counter
    for ( j = 0; j < looping; j++ ) {         // Loop the card shift times
      writeEeprom( start + j, readEeprom(start + 4 + j));   // Shift the array values to 4 places earlier in the EEPROM
    }
    for ( u8 k = 0; k < 4; k++ ) {         // Shifting loop
      writeEeprom( start + j + k, 0);
    }
    //successDelete();
    Serial.println(F("Succesfully removed ID record from EEPROM"));
  }
}

///////////////////////////////////////// Check Bytes   ///////////////////////////////////
bool checkTwo ( byte a[], byte b[] ) {
  for ( u8 k = 0; k < 4; k++ ) {   // Loop 4 times
    if ( a[k] != b[k] ) {     // IF a != b then false, because: one fails, all fail
      return false;
    }
  }
  return true;
}

//////////////////////////////////////// Find Slot   ///////////////////////////////////
u8 findIDSLOT( byte find[] ) {
  u8 count = readEeprom(0);       // Read the first Byte of EEPROM that
  for ( u8 i = 1; i <= count; i++ ) {    // Loop once for each EEPROM entry
    readID(i);                // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) {   // Check to see if the storedCard read from EEPROM
      // is the same as the find[] ID card passed
      return i;         // The slot number of the card
    }
  }
}

///////////////////////////////////////// Find ID From EEPROM   ///////////////////////////////////
bool findID( byte find[] ) {
  u8 count = readEeprom(0);     // Read the first Byte of EEPROM that
  for ( u8 i = 1; i < count; i++ ) {    // Loop once for each EEPROM entry
    readID(i);          // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) {   // Check to see if the storedCard read from EEPROM
      return true;
    }
    else {    // If not, return false
    }
  }
  return false;
}

////////////////////// Check readCard IF is masterCard   ///////////////////////////////////
// Check to see if the ID passed is the master programing card
bool isMaster( byte test[] ) {
  return checkTwo(test, masterCard);
}
///////////////////// Check if readed card and previous card is same //////////////////////////
bool isSame() {
  return checkTwo(readCard, lastReadCard);
}
/////////////////////////////////////////  Access Granted    ///////////////////////////////////
void granted ( u32 interval) {
  u64 startTime;
  relayOn();
  buzzerOn();
  ledRgb(0, 255, 0);
  if (interval < 500) {
    interval = 500;
  }
  startTime = millis();
  do {
    if (millis() - startTime > 140 && millis() - startTime < 160) {
      //buzzerOff();
    }
    if (millis() - startTime > 290 && millis() - startTime < 310) {
      buzzerOff();
    }
  }
  while ((millis() - startTime) < interval);
  if (buzzerState()) {
    buzzerOff();
  }
  relayOff();
  ledRgbOff();
}

///////////////////////////////////////// Access Denied  ///////////////////////////////////
void denied() {
  u64 startTime;
  buzzerOn();
  relayOff();
  ledRgb(250, 0, 0);
  startTime = millis();
  do {
    if (millis() - startTime > 140 && millis() - startTime < 160) { //150
      buzzerOff();
      ledRgbOff();
    }
    if (millis() - startTime > 290 && millis() - startTime < 310) { //300
      buzzerOn();
      ledRgb(250, 0, 0);
    }
    if (millis() - startTime > 440 && millis() - startTime < 460) { //450
      buzzerOff();
      ledRgbOff();
    }
    if (millis() - startTime > 590 && millis() - startTime < 610) { //600
      buzzerOn();
      ledRgb(250, 0, 0);
    }
    if (millis() - startTime > 740 && millis() - startTime < 760) { //750
      buzzerOff();
      ledRgbOff();
    }
  }
  while ((millis() - startTime) < 770);
}
/////////////////////   Init Led    /////////////////////////////
void initLed() {
  pinMode(LED_R, OUTPUT);
  digitalWrite(LED_R, LOW);
  pinMode(LED_G, OUTPUT);
  digitalWrite(LED_G, LOW);
  pinMode(LED_B, OUTPUT);
  digitalWrite(LED_B, LOW);
}
/////////////////////   RGB Led    //////////////////////////////
void ledRgb(uint8_t ledR, uint8_t ledG, uint8_t ledB) {
  analogWrite(LED_R, ledR);
  analogWrite(LED_G, ledG);
  analogWrite(LED_B, ledB);
}
/*
  ledRgb(255, 0, 0); // Red
  ledRgb(0, 255, 0); // Green
  ledRgb(0, 0, 255); // Blue
  ledRgb(255, 255, 125); // Raspberry
  ledRgb(0, 255, 255); // Cyan
  ledRgb(255, 0, 255); // Magenta
  ledRgb(255, 255, 0); // Yellow
  ledRgb(255, 255, 255); // White
*/

void ledRgbW(u8 ledR, u8 ledG, u8 ledB, u32 waitTime) {
  u32 startTime = millis();
  do {
    analogWrite(LED_R, ledR);
    analogWrite(LED_G, ledG);
    analogWrite(LED_B, ledB);
  }
  while (millis() - startTime < waitTime);
}

/////////////////////  Led Color  ////////////////////////////
void ledRed() {
  ledRgbOff();
  delay(1);
  ledRgb(250, 0, 0); // Red
}

void ledGreen() {
  ledRgbOff();
  ledRgb(0, 250, 0); // Green
}

void ledBlue() {
  ledRgbOff();
  ledRgb(0, 0, 250); // Blue
}

void ledMagenta() {
  ledRgbOff();
  ledRgb(75, 0, 230); // Magenta
}
///////////////////// All Red Off ////////////////////////////
void ledRgbOff() {
  ledRgb(0, 0, 0);
}

/////////////////////  Cycle Led  ////////////////////////////
void cycleLed(u16 interval) {
  u32 startTime = millis();
  do {
    if (millis() - startTime < 10) {
      ledRgb(0, 255, 0); // Green
      buzzerOn();
    }
    else if (millis() - startTime > ( interval / 2) - 10 && millis() - startTime < (interval / 2) + 10) {
      ledRgb(255, 0, 0); // Red
      buzzerOff();
    }
  }
  while (millis() - startTime < interval);
}
/////////////////////  Init Buzzer  /////////////////////////////
void initBuzzer() {
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
}

void buzzerOn() {
  if (BUZZER_UP)
    digitalWrite(BUZZER_PIN, HIGH);
}

void buzzerOff() {
  digitalWrite(BUZZER_PIN, LOW);
}

bool buzzerState() {
  if (digitalRead(BUZZER_PIN)) {
    return true;
  }
  return false;
}
void buzzerBip(u8 bipNumber, u16 bipTime) {
  for (int i = 0; i < bipNumber; i++) {
    u32 startTime = millis();
    do {
      if (millis() - startTime < 10) {
        buzzerOn();
      }
      else if (millis() - startTime > ( bipTime / 2) - 10 && millis() - startTime < (bipTime / 2) + 10) {
        buzzerOff();
      }
    }
    while (millis() - startTime < bipTime);

  }
}
/////////////////////  Init Relay   /////////////////////////////
void initRelay() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
}

void relayOn() {
  digitalWrite(RELAY_PIN, HIGH);
}

void relayOff() {
  digitalWrite(RELAY_PIN, LOW);
}

bool relayStatus() {
  return digitalRead(RELAY_PIN);
}
