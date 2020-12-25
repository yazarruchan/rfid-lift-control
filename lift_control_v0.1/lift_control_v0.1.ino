#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>     // We are going to read and write PICC's UIDs from/to EEPROM
/*---------------------------------------------------------------*/
#define BUZZER_PIN 5
/*---------------------------------------------------------------*/
#define RELAY_PIN 4
/*---------------------------------------------------------------*/
/*RFID Reader Rc522 modules pins
 * RST  PIN  9
 * SDA  PIN 10
 * SCK  PIN 13
 * MOSI PIN 11
 * MISO PIN 12
 * IRQ  PIN 
*/
#define SS_PIN 10
#define RST_PIN 9
/*---------------------------------------------------------------*/
#define LED_RED
#define LED_GREEN
#define LED_BLUE
#define LED_VCC
/*---------------------------------------------------------------*/
/*Bos pinler
*2
*3
*6
*7
*8
*A0
*A1
*A2
*A3
*/
/*---------------------------------------------------------------*/
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
//MFRC522::MIFARE_Key key;
/*---------------------------------------------------------------*/
uint64_t lastReadTime;
uint8_t successRead; // Variable integer to keep if we have Successful Read from Reader
byte storedCard[4]; // Stores an ID read from EEPROM
byte readCard[4];   // Stores scanned ID read from RFID Module
byte lastReadCard[4]; //Store previous scanned ID read from RFID Module
byte masterCard[4]={183, 70, 146, 53}; // Stores master card's ID read from EEPROM
//mastercard UID: B7 46 92 35 - 183 70 146 53
/*---------------------------------------------------------------*/
bool RELAY_STATE = false;
/*---------------------------------------------------------------*/
bool programMode = false;  // initialize programming mode to false
bool eraseMode = false;    // initilalize programming mode to register
/*---------------------------------------------------------------*/
void setup() {
  // PIN CONFIG
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_GREEN, LOW);
  //PROTOCOL CONFIG
  Serial.begin(9600);
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522

  Serial.println("Asansor Kat Kontrol v0.1");
  ShowReaderDetails();

  /*WipeMode not necessary now*/
  
  /*Mastercard not store eeprom now, hardcoded in program */
  Serial.println(F("-------------------"));
  Serial.println(F("MasterCard's UID"));
  for ( uint8_t i = 0; i < 4; i++ ) {          // Read Master Card's UID from EEPROM
    //masterCard[i] = EEPROM.read(2 + i);    // Write it to masterCard[]
    Serial.print(masterCard[i], HEX);
  }
  /*End of config sequence*/

  Serial.println("");
  Serial.println(F("-------------------"));
  Serial.println(F("Her sey Hazir!"));
  Serial.println(F("RFID kartin okutulmasi bekleniliyor."));
  Serial.println(F("-----------------------------"));
  lastReadTime = 0;
}

void loop() {
  // put your main code here, to run repeatedly:

  do {
    successRead = getID();  // sets successRead to 1 when we get read from reader otherwise 0
    if (programMode) {
      //cycleLeds();              // Program Mode cycles through Red Green Blue waiting to read a new card
    }
    else {
      normalModeOn();     // Normal mode, blue Power LED is on, all others are off
    }
  }
  while (!successRead);   //the program will not go further while you are not getting a successful read


  if (programMode) {   
    if ( isMaster(readCard) && !eraseMode ) { //When in program mode check First If master card scanned again to change erase mode
      Serial.println(F("Master Kart tekrar okutuldu."));
      Serial.println(F("Kayit silmek icin lütfen kayitli bir kart okutun"));
      //Serial.println(F("-----------------------------"));
      eraseMode = true;
      return;
     }
    if ( eraseMode ) { // If eraseMode true delete it
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
    }
    else {                    // If scanned card is not known add it
      if(findID(readCard)){
        Serial.println(F("Kart daha onceden kaydedilmis."));
      }
      else{
        Serial.println(F("This PICC UID not exist, registering..."));
        writeID(readCard);
        Serial.println(F("Kart basariyla kaydedildi."));
      }
      Serial.println(F("Kayit modundan cikiliyor."));
      Serial.println(F("-----------------------------"));
      programMode = false;
    }  
  }
  else {
    if ( isMaster(readCard)) {    // If scanned card's ID matches Master Card's ID - enter program mode
      programMode = true;
      Serial.println(F("Masterkart okutuldu - Program Modu aktif"));
      uint8_t count = EEPROM.read(0);   // Read the first Byte of EEPROM that
      Serial.print(F("Hafizada kayitli "));     // stores the number of ID's in EEPROM
      Serial.print(count);
      Serial.print(F(" adet kart var."));
      Serial.println("");
      Serial.println(F("Kayit edilecek karti okutun."));
      Serial.println(F("Kayitli karti silmek icin Masterkarti tekrar okutun."));
      Serial.println(F("-----------------------------"));
    }
    else {
      if ( findID(readCard) ) { // If not, see if the card is in the EEPROM
        Serial.println(F("Gecis Onaylandi"));
        granted(3000);         // Open the door lock for 3000 ms
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
*/    while (true); // do not go further
  }
}

///////////////////////////////////////// Get PICC's UID ///////////////////////////////////
uint8_t getID() {
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
  for ( uint8_t i = 0; i < 4; i++) {  //
    lastReadCard[i] = readCard[i];
    readCard[i] = rfid.uid.uidByte[i];
    Serial.print(readCard[i], HEX);
  }
  rfid.PICC_HaltA(); // Stop reading
  Serial.println("");
  if(isSame() && millis()-lastReadTime < 3000){
    Serial.println("Sakin ol sampion");
    return 0;
  }
  else{
    lastReadTime = millis();
    return 1;
  }
}

//////////////////////////////////////// Normal Mode Led  ///////////////////////////////////
void normalModeOn () {
/*digitalWrite(blueLed, LED_ON);  // Blue LED ON and ready to read card
  digitalWrite(redLed, LED_OFF);  // Make sure Red LED is off
  digitalWrite(greenLed, LED_OFF);  // Make sure Green LED is off  
*/
  //Serial.println("Normal Mode");
  digitalWrite(RELAY_PIN, HIGH);    // Make sure Door is Locked
}

//////////////////////////////////////// Read an ID from EEPROM //////////////////////////////
void readID( uint8_t number ) {
  uint8_t start = (number * 4 ) + 2;    // Figure out starting position
  for ( uint8_t i = 0; i < 4; i++ ) {     // Loop 4 times to get the 4 Bytes
    storedCard[i] = EEPROM.read(start + i);   // Assign values read from EEPROM to array
  }
}

///////////////////////////////////////// Add ID to EEPROM   ///////////////////////////////////
void writeID( byte a[] ) {
  if ( !findID( a ) ) {     // Before we write to the EEPROM, check to see if we have seen this card before!
    uint8_t num = EEPROM.read(0);     // Get the numer of used spaces, position 0 stores the number of ID cards
    uint8_t start = ( num * 4 ) + 6;  // Figure out where the next slot starts
    num++;                // Increment the counter by one
    EEPROM.write( 0, num );     // Write the new count to the counter
    for ( uint8_t j = 0; j < 4; j++ ) {   // Loop 4 times
      EEPROM.write( start + j, a[j] );  // Write the array values to EEPROM in the right position
    }
    //successWrite();
    Serial.println(F("Succesfully added ID record to EEPROM"));
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
    uint8_t num = EEPROM.read(0);   // Get the numer of used spaces, position 0 stores the number of ID cards
    uint8_t slot;       // Figure out the slot number of the card
    uint8_t start;      // = ( num * 4 ) + 6; // Figure out where the next slot starts
    uint8_t looping;    // The number of times the loop repeats
    uint8_t j;
    uint8_t count = EEPROM.read(0); // Read the first Byte of EEPROM that stores number of cards
    slot = findIDSLOT( a );   // Figure out the slot number of the card to delete
    start = (slot * 4) + 2;
    looping = ((num - slot) * 4);
    num--;      // Decrement the counter by one
    EEPROM.write( 0, num );   // Write the new count to the counter
    for ( j = 0; j < looping; j++ ) {         // Loop the card shift times
      EEPROM.write( start + j, EEPROM.read(start + 4 + j));   // Shift the array values to 4 places earlier in the EEPROM
    }
    for ( uint8_t k = 0; k < 4; k++ ) {         // Shifting loop
      EEPROM.write( start + j + k, 0);
    }
    //successDelete();
    Serial.println(F("Succesfully removed ID record from EEPROM"));
  }
}

///////////////////////////////////////// Check Bytes   ///////////////////////////////////
bool checkTwo ( byte a[], byte b[] ) {   
  for ( uint8_t k = 0; k < 4; k++ ) {   // Loop 4 times
    if ( a[k] != b[k] ) {     // IF a != b then false, because: one fails, all fail
       return false;
    }
  }
  return true;  
}

//////////////////////////////////////// Find Slot   ///////////////////////////////////
uint8_t findIDSLOT( byte find[] ) {
  uint8_t count = EEPROM.read(0);       // Read the first Byte of EEPROM that
  for ( uint8_t i = 1; i <= count; i++ ) {    // Loop once for each EEPROM entry
    readID(i);                // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) {   // Check to see if the storedCard read from EEPROM
      // is the same as the find[] ID card passed
      return i;         // The slot number of the card
    }
  }
}

///////////////////////////////////////// Find ID From EEPROM   ///////////////////////////////////
bool findID( byte find[] ) {
  uint8_t count = EEPROM.read(0);     // Read the first Byte of EEPROM that
  for ( uint8_t i = 1; i < count; i++ ) {    // Loop once for each EEPROM entry
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
bool isSame(){
  return checkTwo(readCard, lastReadCard);
}
/////////////////////////////////////////  Access Granted    ///////////////////////////////////
void granted ( uint32_t interval) {
  uint64_t startTime;
  int i=1;
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(BUZZER_PIN, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  startTime = millis();
  do {
    if(millis()-startTime == 150){
      digitalWrite(BUZZER_PIN, LOW);
    }
    if(millis()-startTime == 300){
      digitalWrite(BUZZER_PIN, HIGH);
    }
    if(millis()-startTime == 450){
      digitalWrite(BUZZER_PIN, LOW);
    }
  }
  while((millis()-startTime) < interval);
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(LED_GREEN, LOW);
}

///////////////////////////////////////// Access Denied  ///////////////////////////////////
void denied() {
  uint64_t startTime = millis();
  digitalWrite(BUZZER_PIN, HIGH);
  delay(300);
  digitalWrite(BUZZER_PIN, LOW);
  delay(300);  
  digitalWrite(BUZZER_PIN, HIGH);
  delay(300);  
  digitalWrite(BUZZER_PIN, LOW);
  delay(300);  
  digitalWrite(BUZZER_PIN, HIGH);
  delay(300);  
  digitalWrite(BUZZER_PIN, LOW);
}

//////////////////////////////////////// Toggle Buzzer///////////////////////////////////////////////////
void buzzerToggle(int buzzerPin, int volume){
  if(digitalRead(buzzerPin)){
    digitalWrite(buzzerPin, LOW);
  }
  else {
    digitalWrite(buzzerPin, volume);
  }
}

///////////////////////////////////////////Toggle Pin////////////////////////////////////////////////
void togglePin(int tPin){
  if(digitalRead(tPin)){
   digitalWrite(tPin, LOW); 
  }
  else{
    digitalWrite(tPin, HIGH);
  }
}
