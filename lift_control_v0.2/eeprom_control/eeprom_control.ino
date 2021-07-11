
#include <Wire.h>

#define RELAY_PIN 7
#define BUZZER_PIN 8
#define LED_R 6
#define LED_G 5
#define LED_B 3
/*---------------------------------------------------------------*/
#define EXT_EEPROM 1
//#define ADDR_Ax 0b000 //A2, A1, A0
#define ADDR 0x50//(0b1010 << 3) + ADDR_Ax
/*---------------------------------------------------------------*/
typedef uint8_t u8;
//typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
/*---------------------------------------------------------------*/

void setup() {
  Wire.begin(); // join i2c bus (address optional for master)
  Serial.begin(9600);
  initpinler();

  int deger = 0;
  delay(500);
  for(int i=0; i<50 ; i++){
    //writeEeprom(i,0);
    delay(1);
    deger = readEeprom(i);
    delay(1);
    Serial.print(i);
    Serial.print(" adresindeki deger = ");
    Serial.println(deger);
    delay(10);
    
  }
  
}



void loop() {

}
//////////////////////////////////////// EEPROM /////////////////////////////////////
void initEeprom(){
  if(EXT_EEPROM){
    Wire.begin();
    }
    
}

void writeEeprom(u8 address, u8 data){
  if(EXT_EEPROM){
    Wire.beginTransmission(ADDR);
    Wire.write(address);
    Wire.write(data);
    Wire.endTransmission();
    delay(1);
  }
  else if(!EXT_EEPROM){
//  EEPROM.write(address, data);
  }
 
}

u8 readEeprom(u8 address){
  u8 data = NULL;
  if(EXT_EEPROM){
    Wire.beginTransmission(ADDR);
    Wire.write(address);
    Wire.endTransmission();
    Wire.requestFrom(ADDR,1); //retrive 1 returned byte
    delay(1);
    if(Wire.available()){
      data = Wire.read();
    }
  }
  else if(!EXT_EEPROM){
//    data = EEPROM.read(address);
  }
  return data;
}


void initpinler(){
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  pinMode(LED_R, OUTPUT);
  digitalWrite(LED_R, LOW);
  pinMode(LED_G, OUTPUT);
  digitalWrite(LED_G, LOW);
  pinMode(LED_B, OUTPUT);
  digitalWrite(LED_B, LOW);
}
