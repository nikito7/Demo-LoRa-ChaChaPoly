#include <RadioLib.h>

// ESP8266
// NSS pin:   15
// DIO0 pin:  5
// RESET pin: -1
// DIO1 pin:  4

SX1276 radio = new Module(15, 5, -1, 4);

// save transmission state between loops
int transmissionState = RADIOLIB_ERR_NONE;

// flag to indicate that a packet was sent
volatile bool transmittedFlag = false;

// this function is called when a complete packet
// is transmitted by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  // we sent a packet, set the flag
  transmittedFlag = true;
}

// counter to keep track of transmitted packets
int count = 0;

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
  // when packet transmission is finished
  radio.setPacketSentAction(setFlag);

  // start transmitting the first packet
  Serial.print(F("[SX1278] Sending first packet ... "));

  // you can transmit C-string or Arduino string up to
  // 255 characters long
  transmissionState = radio.startTransmit("Hello World!");

  // you can also transmit byte array up to 255 bytes long
  /*
    byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
                      0x89, 0xAB, 0xCD, 0xEF};
    transmissionState = radio.startTransmit(byteArr, 8);
  */
}


void loop() {
  // check if the previous transmission finished
  if(transmittedFlag) {
    // reset flag
    transmittedFlag = false;

    if (transmissionState == RADIOLIB_ERR_NONE) {
      // packet was successfully sent
      Serial.println(F("transmission finished!"));

      // NOTE: when using interrupt-driven transmit method,
      //       it is not possible to automatically measure
      //       transmission data rate using getDataRate()

    } else {
      Serial.print(F("failed, code "));
      Serial.println(transmissionState);

    }

    // clean up after transmission is finished
    // this will ensure transmitter is disabled,
    // RF switch is powered down etc.
    radio.finishTransmit();

    // wait a second before transmitting again
    delay(5000);

    // send another one
    Serial.print(F("[SX1278] Sending another packet ... "));

    // you can transmit C-string or Arduino string up to
    // 255 characters long
    String str = "Hello World! #" + String(count++);
    Serial.println(str);

    // ### CHA_CHA_POLY ###

    ChaChaPolyCipher.generateIv(iv);

    Serial.println("Encrypting...");  

    // construct plain text message
    byte plainText[CHA_CHA_POLY_MESSAGE_SIZE];
    String plain = str;
    plain.getBytes(plainText, CHA_CHA_POLY_MESSAGE_SIZE);

    // encrypt plain text message from plainText to cipherText
    byte cipherText[CHA_CHA_POLY_MESSAGE_SIZE];
    byte tag[CHA_CHA_POLY_TAG_SIZE];
    ChaChaPolyCipher.encrypt(key, iv, auth, plainText, cipherText, tag);

    Serial.println(" ");
    Serial.println("Encrypted:");

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
    Serial.println("Decrypted:");  
    String decrypted = String((char*)plainText);
    Serial.println(decrypted);
    }

    byte txArray[CHA_CHA_POLY_MESSAGE_SIZE + 
                 CHA_CHA_POLY_TAG_SIZE +
                 CHA_CHA_POLY_IV_SIZE];

    memcpy(txArray, cipherText, sizeof(cipherText));
    memcpy(&txArray[sizeof(cipherText)], tag, sizeof(tag));
    memcpy(&txArray[sizeof(cipherText) +
                    sizeof(tag)], iv, sizeof(iv));

    Serial.println(" ");
    Serial.println("txArray:");

    for(i=0; i<sizeof(txArray); i++)
    {
      printHex(txArray[i]);
    }

    // ### CHA_CHA_POLY EOF ###

    transmissionState = radio.startTransmit(txArray, sizeof(txArray));

    // tx eof
  }
}

// EOF
