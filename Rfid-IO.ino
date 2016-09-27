/*
 * Manage RFID Devices - Check out the RFID library for Arduino for further explanations
 */
//
#include <SPI.h>
#include "MFRC522.h"

#define RST_PIN         9          // Configurable, see typical pin layout above
#define SS_PIN          10         // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
//
byte  DEFAULT_KEY[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; //{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; //
byte  PRIVATE_KEY[] = {0xFF, 0x42, 0x42, 0x42, 0x42, 0xFF};

MFRC522::MIFARE_Key MFIRE_DEFAULT_KEY;
MFRC522::MIFARE_Key MFIRE_PRIVATE_KEY;

#define SECTOR_CUSTOM_ID         1
#define BLOCK_CUSTOM_ID          4
#define TRAILER_BLOCK_CUSTOM_ID  7

void displayUuid();
void changeKeysToPrivate();
void displayCustomId();
void setCustomId();
void dumpAllSectorsWithDefaultKey();
void dumpSector();
bool waitForNewDevice();
//
void setup() {
  Serial.begin(9600);		// Initialize serial communications with the PC
  while (!Serial);		// Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();			// Init SPI bus
  
  mfrc522.PCD_Init();		// Init MFRC522
  mfrc522.PCD_DumpVersionToSerial();	// Show details of PCD - MFRC522 Card Reader details
 
  memcpy(MFIRE_DEFAULT_KEY.keyByte, DEFAULT_KEY, 6);
  memcpy(MFIRE_PRIVATE_KEY.keyByte, PRIVATE_KEY, 6);
    
  Serial.println("MFRC initialized. Ready to use.");
}

void displayMainMenu() {
  Serial.println("");
  Serial.println("********** MENU **********");
  Serial.println("1: Display UUID");
  Serial.println("2: Change keys to private");
  Serial.println("3: Display custom ID");
  Serial.println("4: Set custom ID");
  Serial.println("5: Dump all sectors with default key");
  Serial.println("6: Dump sector");
  Serial.println("**************************");
  Serial.println("");
}

void loop() {
  displayMainMenu();
  
  while (!Serial.available());
  
  int choice = Serial.parseInt();
    
  if (choice == 1) displayUuid();
  else if (choice == 2) changeKeysToPrivate();
  else if (choice == 3) displayCustomId();
  else if (choice == 4) setCustomId();
  else if (choice == 5) dumpAllSectorsWithDefaultKey();
  else if (choice == 6) dumpSector();
  else Serial.println("Wrong choice. Try again.");
}

void dumpAllSectorsWithDefaultKey() {
  if (!waitForNewDevice()) // Aborted
    return;
    
  // Dump debug info about the card; PICC_HaltA() is automatically called
  mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
}

void dumpSector() {
  Serial.println("Sector ID:");
  while (!Serial.available());
  char sector_id = (char) Serial.parseInt();
  Serial.println((int) sector_id);
  
  Serial.println("Key (0:default | 1:private):");
  while (!Serial.available());
  delay(25);
  byte chosen_key;
  Serial.readBytes((char *) &chosen_key, 1);
  chosen_key -= '0';
  Serial.println((int) chosen_key);
  
  if (!waitForNewDevice()) // Aborted
    return;
    
  Serial.print("Dumping sector ");
  Serial.print((int) sector_id);
  Serial.print(" with ");
  Serial.print(chosen_key == 0 ? "default" : "private");
  Serial.println(" key");
  
  mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), chosen_key == 0 ? &MFIRE_DEFAULT_KEY : &MFIRE_PRIVATE_KEY, sector_id);
  
  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
}

bool waitForNewDevice() {
  Serial.println("Searching for device... Enter 0 to go back to the main menu.");
  while (true) {
    if (Serial.available()) {
      if (Serial.parseInt() == 0)
        return false;
    }
    
    // Look for new cards
    if (!mfrc522.PICC_IsNewCardPresent())
      continue;
    
    // Select one of the cards
    if (!mfrc522.PICC_ReadCardSerial())
      continue;
     
    return true;
  }
}

bool isDeviceValid() {
  // Check for compatibility
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);

  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI
    &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
    &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("This sample only works with MIFARE Classic cards."));
    return false;
  }
    
  return true;
}

void displayUuid() {
  if (!waitForNewDevice()) // Aborted
    return;
    
  // Dump UUID
  Serial.print(F("Card UUID:"));
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  } 
  Serial.println();
}

void changeKeysToPrivate() {
  if (!waitForNewDevice()) // Aborted
    return;
    
  if (!isDeviceValid()) // Device not supported
    return;
    
  MFRC522::StatusCode status;
  // Authenticate
  Serial.println(F("Authenticating using default key..."));
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, TRAILER_BLOCK_CUSTOM_ID, &MFIRE_DEFAULT_KEY, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
      Serial.print(F("PCD_Authenticate() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return;
  }
  
  // Overwrite default key
  byte new_trailer_block[16];
  memcpy(new_trailer_block, PRIVATE_KEY, 6);
  byte default_access_bits[4] = { 0xFF, 0x07, 0x80, 0x89 };
  memcpy(&(new_trailer_block[6]), default_access_bits, 4);
  memcpy(&(new_trailer_block[10]), PRIVATE_KEY, 6);
  Serial.println(F("Overwriting default key..."));
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(TRAILER_BLOCK_CUSTOM_ID, new_trailer_block, 16);
  if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Write() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
  }
  else
    Serial.println("Device is now private");
  
  
  mfrc522.PICC_HaltA(); // Halt PICC
  mfrc522.PCD_StopCrypto1(); // Stop encryption on PCD
}

void displayCustomId() {   
  if (!waitForNewDevice()) // Aborted
    return;
    
  if (!isDeviceValid()) // Device not supported
    return;
  
  MFRC522::StatusCode status;
  byte custom_id[18];
  memset(custom_id, 0, sizeof(custom_id));
  byte size_custom_id = sizeof(custom_id);
  
  // Authenticate
  Serial.println(F("Authenticating using private key..."));
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, TRAILER_BLOCK_CUSTOM_ID, &MFIRE_PRIVATE_KEY, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
      Serial.print(F("PCD_Authenticate() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return;
  }

  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(BLOCK_CUSTOM_ID, custom_id, &size_custom_id);
  if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Read() check failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
  }
  
  Serial.println("Block Custom ID: ");
  dump_byte_array(custom_id, 16);
  Serial.println("");
  Serial.println("Custom ID: ");
  Serial.println(String((const char *) custom_id));
  
  
  mfrc522.PICC_HaltA(); // Halt PICC
  mfrc522.PCD_StopCrypto1(); // Stop encryption on PCD
}

void setCustomId() {   
  Serial.println("Enter new custom ID : (Max 15 characters | 0 to go back)");
  
  while (!Serial.available());
  delay(25); // Populate serial buffer
  
  byte new_custom_id[16];
  memset(new_custom_id, 0, sizeof(new_custom_id));
  Serial.readBytes((char *) new_custom_id, 15);
  Serial.println(String((const char *) new_custom_id));
  
  if (new_custom_id[0] == '0')
    return;
    
  if (!waitForNewDevice()) // Aborted
    return;
    
  if (!isDeviceValid()) // Device not supported
    return;
  
  MFRC522::StatusCode status;
  byte check_custom_id[18];
  byte size_check = sizeof(check_custom_id);

  // Authenticate
  Serial.println(F("Authenticating using private key..."));
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, TRAILER_BLOCK_CUSTOM_ID, &MFIRE_PRIVATE_KEY, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
      Serial.print(F("PCD_Authenticate() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return;
  }

  // Write data to the block
  Serial.print(F("Writing data into block ")); Serial.print(BLOCK_CUSTOM_ID);
  Serial.println(F(":"));
  dump_byte_array(new_custom_id, 16); Serial.println();
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(BLOCK_CUSTOM_ID, new_custom_id, 16);
  if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Write() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
  }

  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(BLOCK_CUSTOM_ID, check_custom_id, &size_check);
  if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Read() check failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
  }
      
  // Check that data in block is what we have written
  bool ids_match = true;
  for (byte i = 0; i < 16; i++)
      if (check_custom_id[i] != new_custom_id[i]) {
          ids_match = false;
          break;
      }

  if (ids_match) {
      Serial.print("Success : ");
      Serial.print(String((const char *) new_custom_id));
      Serial.println(" is the new custom id");
  }
  else
      Serial.println(F("Failure : no match found for custom id in block"));

  mfrc522.PICC_HaltA(); // Halt PICC
  mfrc522.PCD_StopCrypto1(); // Stop encryption on PCD
}

void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}
