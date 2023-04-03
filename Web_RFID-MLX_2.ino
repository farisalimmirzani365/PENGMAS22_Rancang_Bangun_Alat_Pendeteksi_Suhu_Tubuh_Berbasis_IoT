#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_MLX90614.h>
#include "Adafruit_VL53L0X.h"
#include <LiquidCrystal_I2C.h>

// Wifi Variable and host server
const char* ssid = "Raif";
const char* pass = "raif123456";
const char* host = "192.168.0.4"; //server
const int httpPort = 80; //Port

//Define Pin
#define LED_Pin 15 //D8
#define Btn_Pin 5 //D1
#define SS_PIN 2 //D4
#define RST_PIN 90 //D3

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
LiquidCrystal_I2C lcd(0x27,20,4);   // Create LCD instance.
Adafruit_VL53L0X lox = Adafruit_VL53L0X();    // Create Range instance.
Adafruit_MLX90614 mlx = Adafruit_MLX90614();  // Create Temp. instance.

WiFiClient client;
VL53L0X_RangingMeasurementData_t measure;


void setup() {
  Serial.begin(9600);
  while (! Serial) {
    delay(1);
  }

  //WiFi.hostname("NodeMCU");
  WiFi.begin(ssid, pass);
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(500);
    }
  Serial.println("");
  Serial.println("Connected");
  Serial.println("VLX-RFID-LCD-MLX-LocalHost Test");

  pinMode(LED_Pin, OUTPUT);
  pinMode(Btn_Pin, OUTPUT);

  SPI.begin(); // Initiate  SPI bus
  mfrc522.PCD_Init(); // Initiate MFRC522
  lox.begin(); // VLX Init
  mlx.begin(); // MLX Init
  lcd.init(); // LCD Init
  lcd.backlight();
  Serial.println();
}


void loop() {
  lcd.clear();
  lcd.setCursor (4,0);
  lcd.print("Silahkan");
  lcd.setCursor (3,1);
  lcd.print("Tap  Kartu");
  while (! mfrc522.PICC_IsNewCardPresent())
  {
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }
  
  String IDTAG = "";
  String content = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
   IDTAG += mfrc522.uid.uidByte[i];
   Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
   Serial.print(mfrc522.uid.uidByte[i], HEX);
   content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
   content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  Serial.println("");
  Serial.println("Message : ");

  if ((content.substring(1) == "CC 66 0C AE") || (content.substring(1) == "CC CC A3 16"))
  {
    Serial.println("Authorized access");
    int mde = 0;
    
    //WiFiClient client
    while(!client.connect(host, httpPort)){
      Serial.println("Connection Failed");
      Serial.println(client);
    }
    
    //HTTP Send for RFID
    String Link_tag;
    HTTPClient http;
    Link_tag = "http://192.168.0.4/absensi/kirimkartu.php?nokartu=" + IDTAG;
    http.begin(client, host, httpPort, Link_tag); //client, host, port, url
    int httpCode = http.GET();
    String payload = http.getString();
    Serial.println(httpCode);
    Serial.println(payload);
    Serial.println(Link_tag);
    http.end();

    do{
      lox.rangingTest(&measure, false);
      if (measure.RangeStatus != 4) {
        int vlx_cm = measure.RangeMilliMeter / 10;
        int i = 0;
        float rata_rata = 0;
        
        if(vlx_cm >= 15 && vlx_cm <= 17){
          for (int i = 0; i < 20; i++){
            VL53L0X_RangingMeasurementData_t measure;
            lox.rangingTest(&measure, false);
            int vlx_cm = measure.RangeMilliMeter / 10;
            float c_obj = mlx.readObjectTempC();
            rata_rata = rata_rata + c_obj;
            
            Serial.print("Dist (cm): "); Serial.println(vlx_cm);
            Serial.print("Celcius_Uncalibrated: "); Serial.println(c_obj);
            Serial.println("Mode: 1");
            
            if(mde != 1){
              lcd.clear();
              lcd.setCursor (0,0);
              lcd.print("Suhu Tubuh");
            }
            delay(100);
            mde = 1;
          }
        rata_rata = rata_rata/20;
        Serial.print("Rata-rata suhu: "); Serial.println(rata_rata);
        if(mde != 12){
          lcd.setCursor (0,1);
          lcd.print(rata_rata);
        }
        
        //HTTP Send for Suhu
        String Link_suhu;
        String Suhu = "";
        Suhu.concat(String(rata_rata));
        HTTPClient http1;
        Link_suhu = "http://192.168.0.4/absensi/kirimdata.php?suhu=" + Suhu;
        http1.begin(client, host, httpPort, Link_suhu); //client, host, port, url
        int httpCode1 = http1.GET();
        String payload1 = http1.getString();
        Serial.println(httpCode1);
        Serial.println(payload1);
        Serial.println(Link_suhu);
        http1.end();
        
        mde = 12;
        delay(3000);
        break;
        }
      
        else if(vlx_cm < 15){
          Serial.print("Dist (cm): "); Serial.println(vlx_cm);
          Serial.println("Mode: 2");
          if(mde != 2){
            lcd.clear();
            lcd.setCursor (0,0);
            lcd.print("Terlalu Dekat");
            lcd.setCursor (0,1);
            lcd.print("Jauhkan Dahi");
          }
          mde = 2;
        }
  
        else{
          Serial.print("Dist (cm): "); Serial.println(vlx_cm);
          Serial.println("Mode: 3");
          if(mde != 3){
            lcd.clear();
            lcd.setCursor (0,0);
            lcd.print("Dekatkan");
            lcd.setCursor (0,1);
            lcd.print("Dahi");
          }
          mde = 3;
        }
      }
      
      else{
        if(mde != 99){
          lcd.clear();
          lcd.setCursor (0,0);
          lcd.print("Dekatkan");
          lcd.setCursor (0,1);
          lcd.print("Dahi");
        }
        Serial.println("Out of range!");
        mde = 99;
      }
      delay(200);
    }while(true);
  }

  else{
    Serial.println("Access denied");
    digitalWrite(LED_BUILTIN, LOW); 
    //delay(1000); // delay 1 second
    lcd.setCursor(0,0);
    lcd.print("Akun Tdk Terbaca");
    lcd.setCursor(1,1);
    lcd.print("   Coba Lagi    ");
    delay(3000); // delay 3 second
    lcd.clear();
  }
  
  delay (500);
}
