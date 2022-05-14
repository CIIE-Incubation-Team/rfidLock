#include <deprecated.h>
#include <MFRC522.h>
#include <MFRC522Extended.h>
#include <require_cpp11.h>
#include "base64.hpp"

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#define SERVER_IP "rfid-lock.herokuapp.com"

#ifndef STASSID
#define STASSID "M"
#define STAPSK  ""
#endif

#define RST_PIN         D0           // Configurable, see typical pin layout above
#define SS_PIN          D8          // Configurable, see typical pin layout above
MFRC522 mfrc522(SS_PIN, RST_PIN);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println();

  WiFi.begin(STASSID, STAPSK);
  pinMode(D3,OUTPUT);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
  SPI.begin();                                                  // Init SPI bus
  mfrc522.PCD_Init();
  
}

void loop() {
  digitalWrite(D3,HIGH);
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  //some variables we need
  byte block;
  byte len;
  MFRC522::StatusCode status;

  //-------------------------------------------

  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  Serial.println(F("**Card Detected:**"));

  //-------------------------------------------

  mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid)); //dump some details about the card

  //mfrc522.PICC_DumpToSerial(&(mfrc522.uid));      //uncomment this to see all blocks in hex

  //-------------------------------------------

  Serial.print(F("Name: "));

  byte buffer1[18];

  block = 4;
  len = 18;

  //------------------------------------------- GET FIRST NAME
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 4, &key, &(mfrc522.uid)); //line 834 of MFRC522.cpp file
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  status = mfrc522.MIFARE_Read(block, buffer1, &len);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Reading failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
   unsigned char arr1[24];
   String a1="";
   unsigned int base64_length1 = encode_base64(buffer1, 16, arr1);
  //PRINT FIRST NAME
  for (uint8_t i = 0; i < 16; i++)
  {
    if (buffer1[i] != 32)
    {
      Serial.write(buffer1[i]);
      if(String((char )buffer1[i])!=" "){
        a1 += String((char )buffer1[i]);
        } else {
          break;
          }
      
    }
  }
  Serial.print(" ");

  //---------------------------------------- GET LAST NAME

  byte buffer2[18];
  block = 1;

  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 1, &key, &(mfrc522.uid)); //line 834
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  status = mfrc522.MIFARE_Read(block, buffer2, &len);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Reading failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  unsigned char arr2[24];
  String a2="";
  unsigned int base64_length2 = encode_base64(buffer2, 16, arr2);
  //PRINT LAST NAME
  for (uint8_t i = 0; i < 16; i++) {
    Serial.write(buffer2[i]);
    if(String((char )buffer2[i])!=" "){
        a2 += String((char )buffer2[i]);
        } else {
          break;
          }
  }


  //----------------------------------------

  Serial.println(F("\n**End Reading**\n"));

  delay(1000); //change value if you want to read cards faster

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  if ((WiFi.status() == WL_CONNECTED)) {

    WiFiClient client;
    HTTPClient http;

  http.begin(client, "http://" SERVER_IP "/authOpen"); //HTTP
    http.addHeader("Content-Type", "application/json");
//    String a1="",a2="";

    if(arr2 && arr1){
//     for(int i=0;i<18;i++){
//      Serial.print(buffer1[i]);
//      a1+=String(buffer1[i]);
//      a1+=String(',');
//      }
//      Serial.println();
//    for(int j=0;j<18;j++){
//      Serial.print(buffer2[j]);
//      a2+=String(buffer2[j]);
//      a2+=String(',');
//      }
    Serial.println(a1);
    Serial.println(a2);
    Serial.print("[HTTP] POST...\n");
    // start connection and send HTTP header and body
    String body = "{\"user\":\""+a1+"\",\"enc\":0,\"password\":\""+a2+"\"}";
    Serial.println(body);
    body.trim();
    int httpCode = http.POST(body);
    if (httpCode > 0) {
      StaticJsonDocument<10000> doc;
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] POST... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        const String& payload = http.getString();
        Serial.println("received payload:\n<<");
        Serial.println(payload);
        Serial.println(">>");
        DeserializationError error = deserializeJson(doc, payload);
        if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      }
      if(doc["status"] == "open"){
          digitalWrite(D3,LOW);
          delay(5000);
          digitalWrite(D3,HIGH);
        }
      } else {
          delay(5000);
        }
  }}
}
