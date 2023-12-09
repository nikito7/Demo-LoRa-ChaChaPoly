#include <RadioLib.h>

// C3
// NSS pin:   7
// DIO0 pin:  10
// RESET pin: -1
// DIO1 pin:  3

SX1276 radio = new Module(7, 10, -1, 3);

// flag to indicate that a packet was received
volatile bool receivedFlag = false;

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  // we got a packet, set the flag
  receivedFlag = true;
}

// ### CHA_CHA_POLY ###

#include "ChaChaPolyHelper.h"

#define CHA_CHA_POLY_KEY_SIZE 32
#define CHA_CHA_POLY_IV_SIZE 12
#define CHA_CHA_POLY_AUTH_SIZE 16
#define CHA_CHA_POLY_TAG_SIZE 16
#define CHA_CHA_POLY_MESSAGE_SIZE 100

byte key[CHA_CHA_POLY_KEY_SIZE] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
    0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38};

byte auth[CHA_CHA_POLY_AUTH_SIZE] = {
     0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
     0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18};

byte iv[CHA_CHA_POLY_IV_SIZE];

// #¥# CHA_CHA_POLY EOF ###

int i;

void printHex(uint8_t num) {
  char hexCar[2];

  sprintf(hexCar, "%02X", num);
  Serial.print(hexCar);
}

void setup() {
  Serial.begin(9600);

  // initialize SX1278 with default settings
  Serial.print(F("[SX1278] Initializing ... "));

  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  if (radio.setFrequency(868.0) == RADIOLIB_ERR_INVALID_FREQUENCY) {
    Serial.println(F("Selected frequency is invalid for this module!"));
    while (true);
  }

  // set the function that will be called
  // when new packet is received
  radio.setPacketReceivedAction(setFlag);

  // start listening for LoRa packets
  Serial.print(F("[SX1278] Starting to listen ... "));
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  // if needed, 'listen' mode can be disabled by calling
  // any of the following methods:
  //
  // radio.standby()
  // radio.sleep()
  // radio.transmit();
  // radio.receive();
  // radio.scanChannel();
}


void loop() {
  // check if the flag is set
  if(receivedFlag) {
    // reset flag
    receivedFlag = false;

    byte rxArray[CHA_CHA_POLY_MESSAGE_SIZE + 
                 CHA_CHA_POLY_TAG_SIZE +
                 CHA_CHA_POLY_IV_SIZE];

    int numBytes = radio.getPacketLength();
    int state = radio.readData(rxArray, numBytes);

    if (state == RADIOLIB_ERR_NONE) {
      // packet was successfully received
      Serial.println(F("[SX1278] Received packet!"));

      // ### CHA_CHA_POLY ###

      // ChaChaPolyCipher.generateIv(iv);

      byte tag[CHA_CHA_POLY_TAG_SIZE];
      byte plainText[CHA_CHA_POLY_MESSAGE_SIZE];
      byte cipherText[CHA_CHA_POLY_MESSAGE_SIZE];

      memcpy(cipherText, rxArray, sizeof(cipherText));
      memcpy(tag, &rxArray[sizeof(cipherText)], sizeof(tag));
      memcpy(iv, &rxArray[sizeof(cipherText) +
                    sizeof(tag)], sizeof(iv));

      Serial.println(" ");
      Serial.println("rxArray:");  

      for(i=0; i<sizeof(rxArray); i++)
      {
        printHex(rxArray[i]);
      }

      Serial.println(" ");
      Serial.println("cipherText:");  

      for(i=0; i<sizeof(cipherText); i++)
      {
        printHex(cipherText[i]);
      }

      Serial.println(" ");
      Serial.println("Tag:");

      for(i=0; i<sizeof(tag); i++)
      {
        printHex(tag[i]);
      } 

      Serial.println(" ");
      Serial.println("IV:");

      for(i=0; i<sizeof(iv); i++)
      {
        printHex(iv[i]);
      } 

      // decrypt message from cipherText to plainText
      // output is valid only if result is true
      bool verify = ChaChaPolyCipher.decrypt(key, iv, auth, cipherText, plainText, tag);

      if (verify)
      {
        Serial.println(" ");
        Serial.println("Decrypted:");  
        String decrypted = String((char*)plainText);
        Serial.println(decrypted);
      }

      // ### CHA_CHA_POLY EOF ###

      // print RSSI (Received Signal Strength Indicator)
      Serial.print(F("[SX1278] RSSI:\t\t"));
      Serial.print(radio.getRSSI());
      Serial.println(F(" dBm"));

      // print SNR (Signal-to-Noise Ratio)
      Serial.print(F("[SX1278] SNR:\t\t"));
      Serial.print(radio.getSNR());
      Serial.println(F(" dB"));

      // print frequency error
      Serial.print(F("[SX1278] Frequency error:\t"));
      Serial.print(radio.getFrequencyError());
      Serial.println(F(" Hz"));

    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      // packet was received, but is malformed
      Serial.println(F("[SX1278] CRC error!"));

    } else {
      // some other error occurred
      Serial.print(F("[SX1278] Failed, code "));
      Serial.println(state);

    }
  }
}

// EOF
