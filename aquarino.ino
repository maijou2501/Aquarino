/*
 *  Aquarino
 *
 *  温度(水温)を測って、thingspeak.com に通知する。
 *  LCD には測定結果を表示する。
 *  その後、次の測定までは ESP-WROOM-02 の Deep-Sleepモードにて待機する。
 *  
 *  @author  kyohei ito
 *  @version 1.0
 */
#include <LiquidCrystal.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

//ESP-WROOM-02でアナログ入力をするため
extern "C" {
#include "user_interface.h"
}

LiquidCrystal lcd(14, 12, 13, 2, 5, 4);

const char* ssid     = "Your_Wi-Fi_network";
const char* password = "Your_Wi-Fi_network_password";

const char* host =   "api.thingspeak.com";
const char* apikey = "your_ThingSpeak_APIkey";
//const int httpPort = 80;
const int httpsPort = 443;
// Use web browser to view and copy
// SHA1 fingerprint of the certificate
const char* fingerprint = "78 60 18 44 81 35 BF DF 77 84 D4 0A 22 0D 9B 4E 6C DC 57 2C";

const int sleepTimeHour = 1;

//TOUTピンからの入力値を取得(許可されている電圧:0-1V)
float getTempAir(){
  float sumV = 0;
  float mV = 0;
  for(int i=0;i<5;i++){
    // MCP9700のTOUTの出力電圧を取得
    mV = system_adc_read();

    //Serial.print("TOUT from MCP9700 : ");
    //Serial.print(mV);
    //Serial.print(" mV (count ");
    //Serial.print(i);
    //Serial.println(")");

    sumV += mV;
    delay(2000); 
  }

  float averageV = sumV / 5;
  //Serial.print("TOUT Average      : ");
  //Serial.print(averageV);
  //Serial.print(" mV");

  // mVを摂氏温度[C]に変換
  // 0度で500mVのオフセットがのっているので500mVを引く
  // 10mV / 温度
  // +a 自分の端子の特性でちょっと引く
  float tempC = (averageV - 500 - 12) / 10;

  //Serial.print  (" ( ");
  //Serial.print  (tempC); 
  //Serial.println(" degress C)");

  return tempC; 
}

void setup() {
  //LCD setting
  pinMode(14, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(2,  OUTPUT);
  pinMode(5,  OUTPUT);
  pinMode(4,  OUTPUT);
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Aquarino-ver1.0");

  //Serial.begin(115200);
  delay(10);

  // connecting to a WiFi network
  //Serial.println();
  //Serial.println();
  //Serial.print  ("Connecting to ");
  //Serial.println(ssid);
  
  WiFi.begin(ssid, password);

  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //auto scroll
    lcd.setCursor(count % 17 , 1);
    lcd.print(".>>((#( ')");
    count++;
    //Serial.print(".");
  }

  //Serial.println("");
  //Serial.println("WiFi connected");  
  //Serial.print  ("IP address : ");
  //Serial.println(WiFi.localIP());
  //Serial.println("");
  lcd.setCursor(0, 1);
  lcd.print("WiFiOK (' )#))<<");

  //温度測定
  float tempAir = getTempAir();
  // We now create a URI for the request
  String url = "/update";
  url += "?api_key=";
  url += apikey;
  url += "&field1=";
  url += tempAir;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp : ");
  lcd.print(tempAir);
 
  // Use WiFiClientSecure class to create TCP connections
  WiFiClientSecure client;

  //Serial.print  ("Connecting HOST   : ");
  //Serial.println( host );

  // コネクションの確認
  lcd.setCursor(0, 1);
  if (!client.connect(host, httpsPort)) {
    //Serial.println("connection failed");
    lcd.print("Cx check:NETWORK");
  } else {
    //Serial.println("connection successed");
    lcd.print("Co");
    lcd.setCursor(2, 1);

    // SSLサーバー証明書の確認
    if (client.verify(fingerprint, host)) {
      //Serial.println("certificate matches");
      lcd.print("Vo");
  
      // 通信先が正規のサイトであるため、データ送信を行う
      //Serial.print("Requesting URL    : ");
      //Serial.println(url);
      
      // This will send the request to the server
      client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                   "Host: " + host + "\r\n" + 
                   "Connection: close\r\n\r\n");
      delay(3000);
      
      // Read all the lines of the reply from server and print them to Serial
      while(client.available()){
        String line = client.readStringUntil('\r');
        //Serial.print(line);
      }

      // 通信結果の確認
      lcd.setCursor(4, 1);
      if(client.available()){
        //Serial.println("GET success");
        lcd.print("So (' )#))<<");
      } else {
        //Serial.println("GET fail");
        lcd.print("Sx (' )#))<<");
      }
        
    } else {
      // SSLサーバー証明書が不正な場合はデータ送信を行わない
      //Serial.println("certificate doesn't match");
      lcd.print("VxSx checkHOST");
    }
   
  }

  // Deep-Sleepモード
  //sleepTimeHour * 60min * 60sec * 1000 * 1000
  ESP.deepSleep(sleepTimeHour * 3600000000, WAKE_RF_DEFAULT);
  //ESP.deepSleep(sleepTimeHour * 20000000, WAKE_RF_DEFAULT);
  //Deep-Sleepモード移行までのダミー命令
  delay(1000);
}

void loop() {
  //Deep-Sleepモードに移行するため loop() には入らない
  delay(5000);
}

