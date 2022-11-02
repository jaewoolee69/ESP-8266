/// 2
/// 2
///   dfdsljdlfjadfjasdfjas;dfjas

//This is second for GitHub

/********************************************************
 
    ** Pin Mapping **
    
        ----------------------------------------------------
        DHT22 v               Wemos              
        ----------------------------------------------------
        
        DHT22(D4)     -------- D4
        (Temperature)
        3.3V(Wemos)   -------- 3.3V
        GND           -------- GND


        ----------------------------------------------------
        Wemos              P/B                     Wemos
        ----------------------------------------------------
                           PB LED ---------------  D1
                           GND    ---------------  GND
                             
        3.3V(Wemos) ------ P/B ------------------  D0
                                 |
                                 |---R(220옴)----- GND
       

        ---------------------------------------------------
        외부 DC Power Supply                       Wemos
        ---------------------------------------------------                         
        5V        ---------------------------     5V
        GND       ---------------------------     GND
       
                                 
                              
    거실 : (ESP-51CB2D)(고정IP)
    안방 : (ESP-51D6A1)(고정IP)
    찬우 : (ESP-528E95)(고정IP)
    형우 : (ESP-DA9A2B)(고정IP)

    메인 : 192.168.0.14 (ESP-FB4139)(고정IP)

    V2.1 수정사항

    (1) 찬우 PB Led의 밝기조절을 위해 analogWrite(D1,xx) 변경 --> 찬우가 PB LED 밝기가 눈에 거슬린다고 함

    V2.5 수정사항

    (1) Warm_Water_Supply_Time 23분을 38분으로 변경 
        * 이 변수는 사실상 필요없슴. V/V Close시간계산은 다른 프로그램에서 계산함. 
        

    V2.7 수정사항
    (1) 고정IP로 수정
    (2) 찬우방 LED밝기 100--> 10 으로 수정함

    V2.8 수정사항
    (1) 푸쉬버튼 LED의 밝기 표현을 digitalWrite에서 모두 analogWrite로 변경함.(이유: 안방 LED가 너무 밝아서 눈에 거슬림)

    V.2.10 수정사항
    (1) 거실에 한하여 오전 6시부터 오전 9시까지 앱에서 설정하면 난방이 되겠끔 수정함
        loop() 함수내에 거실부분을 수정을 함 ----> 거실새벽난방 부분 첨가함
            
                          제작일: 2021년 3월 28일(일)
********************************************************/

#include "FirebaseESP8266.h" 
#include <ESP8266WiFi.h> 
#include <time.h> 
#include <ESP8266mDNS.h>  // OTA
#include <WiFiUdp.h> // OTA
#include <ArduinoOTA.h> // OTA

#include "DHT.h"


// *************************************************
// 반드시 통신포트를(=IP) 확인하고, Uploading해야 합니다.
// *************************************************
#define RoomName 1    // 1:거실, 2:안방, 3:찬우방, 4:형우방   // <------ 필요시 변경
#define Temp_Calibration_Value -1.4 // 온도 보정값  (거실:-1.4 / 안방:-3.3 / 찬우:-1.7 / 형우:-2.2  // <------ 필요시 변경
                                  
#define FIREBASE_HOST "myboiler-a2f07.firebaseio.com" // http달린거 빼고 적어야 됩니다.
#define FIREBASE_AUTH "gHfRRhpY46ROGp8ukBhkRpYi1VLHYeHH87HpkahS" // 데이터베이스 비밀번호

#define WIFI_SSID "Jae_BackUp" // 연결 가능한 wifi의 ssid
#define WIFI_PASSWORD "@9669J63C5" // wifi 비밀번호


//DHTesp dht;
DHT dht(D4, DHT22);


// NTP Timer를 위한 변수
unsigned long previousMillis = 0;
unsigned long currentMillis = 0;   
const long interval = 1000;

// previousMillis1 --> 온도값얻기위한 10초짜리 /  previousMillis2 --> On/Off 상태값을 얻기위한 3초짜리
unsigned long previousMillis1 = 0, previousMillis2 = 0;
unsigned long currentMillis1 = 0, currentMillis2 =0;

String String_Hour, String_Min, String_Sec;  // NTP Server로부터 받은 시간, 분, 초 값 
int i_Hour = 0, i_Min = 0, i_Sec = 0;  // 위 스트링값의 분,초 값을 Int로 변경할 변수

char* ssid = WIFI_SSID;  
char* password = WIFI_PASSWORD;

double current_temp = 0.0;

String Korean_NTP; //NTP에서 받은 시간정보를 한국시간으로 변경하여 받기 위한 변수

int val = 0;  // D0의 Momentary PB의 값(누르면 1, 누르지 않을때 0)
int tval = 0; // 상태변경이 있을시만, RTDB에 쓰기위한 임시변수
int state = 0;  // Momentary PB값을 toggle값으로 변경하기 위해 사용한 변수

int Br = 255; // 푸쉬버튼 Led(=D1)의 밝기표현 변수
String L_Mor_Heating;


FirebaseData firebaseData;
FirebaseJson json;
 

void setup() { 
 
  Serial.begin(115200);
  
  pinMode (D0, INPUT);  // PB
  //pinMode (D1, OUTPUT); // LED


  if (RoomName == 1) { // 거실이 선택되었다면
    Br = 255;  // 푸쉬버튼 LED(=D1)의 밝기
  }

  if (RoomName == 2) { // 안방이 선택되었다면
    Br = 30;
  }


  if (RoomName == 3) { // 찬우방이 선택되었다면
    Br = 10;
  }


  if (RoomName == 4) { // 형우방이 선택되었다면
    Br = 30;
  }

  
  //dht.setup(D4, DHTesp::DHT22);
  dht.begin();

  WiFi.begin(ssid, password);  //위의 SSID와 패스워드로 접속시도하기
  Serial.println("\nConnecting to WiFi");
  WiFi.mode(WIFI_STA);  
  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("Local IP address: ");   // 내부 IP 주소는:

  Serial.println(WiFi.localIP());      //내부IP를 얻는다.


  delay(300);
   
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());


   // 표준시와 한국시간이 -9시간 차이
   configTime(-9 * 3600, 0, "pool.ntp.org", "time.nist.gov"); 
   
   Serial.println("\nWaiting for time"); 
   while (!time(nullptr)) { 
     Serial.print("."); 
     delay(100); 
   }
   Serial.println("");
   

  // 파이어베이스
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
 
  //Set the size of WiFi rx/tx buffers in the case where we want to work with large data.
    firebaseData.setBSSLBufferSize(1024, 1024);
  
  //Set the size of HTTP response buffers in the case where we want to work with large data.
    firebaseData.setResponseSize(1024);
  
  //Set database read timeout to 1 minute (max 15 minutes)
    Firebase.setReadTimeout(firebaseData, 1000 * 60);
  
  //tiny, small, medium, large and unlimited.
  //Size and its write timeout e.g. tiny (1s), small (10s), medium (30s) and large (60s).
  //  Firebase.setwriteSizeLimit(firebaseData, "small");
  Firebase.setwriteSizeLimit(firebaseData, "tiny");

Read_OnOff_State();  // 현재의 PB 상태를 RTDB에서 읽어와서, PB Lamp의 On/Off를 동기화 시킨다.

}



void GetTime_from_NTP() {

            ///////////////////////////////////////////
            //             NTP 시계 만들기            //
            //////////////////////////////////////////  
            currentMillis = millis();
    
            if (currentMillis - previousMillis >= interval) {
              
                                previousMillis = currentMillis;
                                time_t now = time(nullptr); 
 
                                Korean_NTP = (ctime(&now));  //time.h에 정의된 함수 ctime();은 현재년도,날짜,요일,시간정보를 가지고 있음
                
                                String Day_of_The_Week = Korean_NTP.substring(0,3);
                                String Month = Korean_NTP.substring(4,7);
                                String Day = Korean_NTP.substring(8,10);
                                String Time = Korean_NTP.substring(10,19);
                                String Year = Korean_NTP.substring(20,24);


                                // NTP에서 온 시간정보를 String_Hour, _Min, _Sec에 넣음 (두자리 형태임: 01,02..)
                                String_Hour = Time.substring(1,3);
                                String_Min = Time.substring(4,6);
                                String_Sec = Time.substring(7,9);
                                
                             /*   Serial.print(String_Hour);
                                Serial.print(":");
                                Serial.print(String_Min);
                                Serial.print(":");
                                Serial.println(String_Sec);
                              */

                                i_Hour = String_Hour.toInt();
                                i_Min = String_Min.toInt();
                                i_Sec = String_Sec.toInt();


                                Serial.print(i_Hour);
                                Serial.print(":");
                                Serial.print(i_Min);
                                Serial.print(":");
                                Serial.println(i_Sec);
                                
                  
            }

                       
              
} //End   



void Read_OnOff_State() {

                //////////////////////////////////
                //  거실 On/Off State값 읽어오기  //
                /////////////////////////////////

          if (RoomName == 1) {  // 거실이면

                if (Firebase.getString(firebaseData, "MyBoiler/Living Room On")) { // 거실의 On/Off State값을 읽어온다
                                            
                    String OnOff = firebaseData.stringData(); // 값을 문자열로 받아와서
      
                          if (OnOff == "1"){
                          state = 1;
                          analogWrite(D1, Br);
                          }
                          else {
                          state = 0;  
                          analogWrite(D1, 0);    
                          }
                                      
                }  
                          
                else {
                      Serial.println(firebaseData.errorReason()); // 에러 메시지 출력
                }

          }




                //////////////////////////////////
                //  안방 On/Off State값 읽어오기  //
                /////////////////////////////////

          if (RoomName == 2) {  // 안방 이면

                if (Firebase.getString(firebaseData, "MyBoiler/An Bang On")) { // 안방의 On/Off State값을 읽어온다
                                            
                    String OnOff = firebaseData.stringData(); // 값을 문자열로 받아와서
      
                          if (OnOff == "1"){
                          state = 1;
                          analogWrite(D1, Br);
                          }
                          else {
                          state = 0;  
                          analogWrite(D1, 0);    
                          }
                                      
                }  
                          
                else {
                      Serial.println(firebaseData.errorReason()); // 에러 메시지 출력
                }

          }

          

                //////////////////////////////////
                //  찬우방 On/Off State값 읽어오기 //
                //////////////////////////////////

          if (RoomName == 3) {  // 찬우방 이면                

                if (Firebase.getString(firebaseData, "MyBoiler/ChanWoo Bang On")) { // 찬우방의 On/Off State값을 읽어온다
                                            
                    String OnOff = firebaseData.stringData(); // 값을 문자열로 받아와서
      
                          if (OnOff == "1"){
                          state = 1;
                          
                          analogWrite(D1, Br);
                          
                          }
                          else {
                          state = 0;  
                          
                          analogWrite(D1, 0);
                          
                          }
                                      
                }  
                          
                else {
                      Serial.println(firebaseData.errorReason()); // 에러 메시지 출력
                }

          }
          


                //////////////////////////////////
                //  형우방 On/Off State값 읽어오기 //
                //////////////////////////////////


          if (RoomName == 4) {  // 형우방 이면                

                if (Firebase.getString(firebaseData, "MyBoiler/HyungWoo Bang On")) { // 형우방의 On/Off State값을 읽어온다
                                            
                    String OnOff = firebaseData.stringData(); // 값을 문자열로 받아와서
      
                          if (OnOff == "1"){
                          state = 1;
                          analogWrite(D1, Br);
                          }
                          else {
                          state = 0;  
                          analogWrite(D1, 0);    
                          }
                                      
                }  
                          
                else {
                      Serial.println(firebaseData.errorReason()); // 에러 메시지 출력
                }

          }
  
}



void Check_Push_Button_and_Write_Warm_Water_Supply_Time() {

    val = digitalRead (D0); // PB값 얻기

    if (val == 1) {   // PB이 눌러지면

      state = 1 - state;   // state는 val값 toggle하기 위한 변수 (누르면 0 계속---> 다시 누르면 1계속)
      
    }

    Serial.print(" state = ");
    Serial.println(state);
    
    delay(1000);
    
        if (state == 1){
              
              analogWrite(D1, Br);  
              
        } else {
              
              analogWrite(D1, 0);  
              
      
        }
  

        
        if (RoomName == 1) {  // 거실 이면       

              if (tval != val) {  // PB 버튼값이 서로 다른때만 RTDB에 Update(= PB값의 변화가 있을때만 RTDB에 쓴다)
          
                  tval = val;
          
                     
                          Firebase.setString(firebaseData, "MyBoiler/Living Room On", (String)state);   // PB버튼의 On/Off값을 RTDB 거실 On/Off 에 ON/Off값을 쓴다.
                          delay(100);
          
                        
                          Firebase.setInt(firebaseData, "MyBoiler/L_StartMinute", i_Min);   // 밸브를 Open할 분 값을 RTDB에 쓴다.
                          delay(100);

                
              }

        }



        
        if (RoomName == 2) {  // 안방 이면       

              if (tval != val) {  // PB 버튼값이 서로 다른때만 RTDB에 Update(= PB값의 변화가 있을때만 RTDB에 쓴다)
          
                  tval = val;
          
                     
                          Firebase.setString(firebaseData, "MyBoiler/An Bang On", (String)state);   // PB버튼의 On/Off값을 RTDB 거실 On/Off 에 ON/Off값을 쓴다.
                          delay(100);
          
                        
                          Firebase.setInt(firebaseData, "MyBoiler/A_StartMinute", i_Min);   // 밸브를 Open할 분 값을 RTDB에 쓴다.
                          delay(100);
                
              }

        }




        
        if (RoomName == 3) {  // 찬우방 이면       

              if (tval != val) {  // PB 버튼값이 서로 다른때만 RTDB에 Update(= PB값의 변화가 있을때만 RTDB에 쓴다)
          
                  tval = val;
          
                     
                          Firebase.setString(firebaseData, "MyBoiler/ChanWoo Bang On", (String)state);   // PB버튼의 On/Off값을 RTDB 거실 On/Off 에 ON/Off값을 쓴다.
                          delay(100);
          
                        
                          Firebase.setInt(firebaseData, "MyBoiler/C_StartMinute", i_Min);   // 밸브를 Open할 분 값을 RTDB에 쓴다.
                          delay(100);
                
              }

        }



        
        if (RoomName == 4) {  // 형우방 이면       

              if (tval != val) {  // PB 버튼값이 서로 다른때만 RTDB에 Update(= PB값의 변화가 있을때만 RTDB에 쓴다)
          
                  tval = val;
          
                     
                          Firebase.setString(firebaseData, "MyBoiler/HyungWoo Bang On", (String)state);   // PB버튼의 On/Off값을 RTDB 거실 On/Off 에 ON/Off값을 쓴다.
                          delay(100);
          
                        
                          Firebase.setInt(firebaseData, "MyBoiler/H_StartMinute", i_Min);   // 밸브를 Open할 분 값을 RTDB에 쓴다.
                          delay(100);

                
              }

        }



    
}



void loop() {


            ArduinoOTA.handle();  // OTA

            GetTime_from_NTP();

            Check_Push_Button_and_Write_Warm_Water_Supply_Time();
            


            ///////////////////////////////////////////////////////////////
            //      온도값을 가져와서, RTDB에 Write한다.                     //
            ///////////////////////////////////////////////////////////////

            currentMillis1 = millis();
    
            if (currentMillis1 - previousMillis1 >= 2000) { // 2초에 한번씩
              
                    previousMillis1 = currentMillis1;


                   // double t = dht.getTemperature();
                    double t = dht.readTemperature();

                    double tt = ((floor (t * 10)) / 10.00) + Temp_Calibration_Value ;


                    Serial.print(" Current Temperature =  ");
                    Serial.print(tt);
                    Serial.println(" ℃");

                    if (current_temp != tt) {  // 온도변화가 있으면, RTDB에 Write한다.

                          current_temp = tt;

  
                          if (RoomName == 1) {
                      
                                Firebase.setDouble(firebaseData, "MyBoiler/L_Temp", current_temp);   // 거실 온도값
  
                          }
  
                          if (RoomName == 2) {
                      
                                Firebase.setDouble(firebaseData, "MyBoiler/A_Temp", current_temp);   // 안방 온도값
                          }
  
                          if (RoomName == 3) {
                      
                                Firebase.setDouble(firebaseData, "MyBoiler/C_Temp", current_temp);   // 찬우방 온도값
  
                          }
  
                          if (RoomName == 4) {
                      
                                Firebase.setDouble(firebaseData, "MyBoiler/H_Temp", current_temp);   // 형우방 온도값
  
  
                          }                                                                        
                          


                    }

            }



            /////////////////////////////////////////////////////////////////////////////////////
            //              RTDB로 부터 On/Off 값을 가져와서, PB Lamp(D1) 에 표시한다.              //
            ////////////////////////////////////////////////////////////////////////////////////
            
            currentMillis2 = millis();

            if (currentMillis2 - previousMillis2 >= 3000) {  // 3초
              
                    previousMillis2 = currentMillis2;


                                    ////////////////////////////////
                                    //  거실  On/Off 상태 읽어오기  //
                                    ////////////////////////////////

                if (RoomName == 1) {                                    
                                      
                          if (Firebase.getString(firebaseData, "MyBoiler/Living Room On")) { // 해당하는 key가 있으면 (앱 인벤터에서는 tag)
                            
                              String OnOff = firebaseData.stringData(); // 값을 문자열로 받아와서


                                        if (OnOff == "1") {
                                            state = 1;
                                            analogWrite(D1, Br);
                                        }
                                        else {
                                            state = 0;
                                            analogWrite(D1, 0);
                                        }
                                        
                                      
                          }  
                          
                          else {
                                Serial.println(firebaseData.errorReason()); // 에러 메시지 출력
                          }



                                    ////////////////////////////////
                                    //         거실  새벽난방       //
                                    ////////////////////////////////
                          
                          ///// 오전 6시에 자동 껴기
 
                          if ((i_Hour == 5) && (i_Min == 59) && (i_Sec >= 55 && i_Sec <= 59)) { // 거실 새벽 Heating여부 읽어오기


                               if (Firebase.getString(firebaseData, "MyBoiler/L_Mor_Heating")) {
                                    L_Mor_Heating = firebaseData.stringData(); // 값을 문자열로 받아와서
                                    //delay(100);
                                    
                               } else {
                                
                                        Serial.println(firebaseData.errorReason()); // 에러 메시지 출력
                               }

                          }
                               
                       
                          if ((L_Mor_Heating == "1") && (i_Hour == 6 ) && (i_Min == 0) && (i_Sec >= 0 && i_Sec <= 4)) {

              
                              state = 1;
                              analogWrite(D1, Br);
                              
                              Firebase.setString(firebaseData, "MyBoiler/Living Room On", (String)state);   // RTDB 거실 On/Off 에 On 값을 쓴다.
                              //delay(100);

                              Firebase.setInt(firebaseData, "MyBoiler/L_StartMinute", 57);   // 밸브를 Open할 분 값으로 57분 RTDB에 쓰게하여 6시부터 v/v Open기동을 하게한다.
                              //delay(100);


                          } else {

                                  Serial.println(firebaseData.errorReason()); // 에러 메시지 출력
                            
                          }


                          ///// 오전 9시에 자동 끄기
                          ///// 오전 6시부터 9시사에에 WDT Error로 인하여 L_Mor_Heating 값이 1--> 0으로 될수 있으므로, 직전에 다시한번
                          ///// L_Mor_Heating의 값을 읽어온 뒤 수행토록 프로그램을 수정하였습니다.
                          
                          if ((i_Hour == 8) && (i_Min == 59) && (i_Sec >= 55 && i_Sec <= 59)) { // 거실 새벽 Heating여부 다시 한번 읽어오기(이유:WDT Error)


                               if (Firebase.getString(firebaseData, "MyBoiler/L_Mor_Heating")) {
                                    L_Mor_Heating = firebaseData.stringData(); // 값을 문자열로 받아와서
                                    //delay(100);
                                    
                               } else {
                                
                                        Serial.println(firebaseData.errorReason()); // 에러 메시지 출력
                               }

                          }

               
                          if ((L_Mor_Heating == "1") && (i_Hour == 9 ) && (i_Min == 0) && (i_Sec >= 0 && i_Sec <= 4)) {

              
                              state = 0;
                              analogWrite(D1, 0);
                              
                              Firebase.setString(firebaseData, "MyBoiler/Living Room On", "0");   // RTDB 거실 On/Off 에 Off 값을 쓴다.
                              //delay(100);


                          } else {

                                  Serial.println(firebaseData.errorReason()); // 에러 메시지 출력
                            
                          }
           
                } //Room No.1
                

                                    ////////////////////////////////
                                    //  안방  On/Off 상태 읽어오기   //
                                    ////////////////////////////////

                if (RoomName == 2) {                                         
                                      
                          if (Firebase.getString(firebaseData, "MyBoiler/An Bang On")) { // 해당하는 key가 있으면 (앱 인벤터에서는 tag)
                            
                              String OnOff = firebaseData.stringData(); // 값을 문자열로 받아와서
                          
                                        if (OnOff == "1") {
                                            state = 1;
                                            analogWrite(D1, Br);
                                        }
                                        else {
                                            state = 0;
                                            analogWrite(D1, 0);
                                        }     
                                    
                                      
                          }  
                          
                          else {
                                Serial.println(firebaseData.errorReason()); // 에러 메시지 출력
                          }
                    

                }

                                    ////////////////////////////////
                                    //  찬우방  On/Off 상태 읽어오기 //
                                    ////////////////////////////////

                if (RoomName == 3) {                                         
                                      
                          if (Firebase.getString(firebaseData, "MyBoiler/ChanWoo Bang On")) { // 해당하는 key가 있으면 (앱 인벤터에서는 tag)
                            
                              String OnOff = firebaseData.stringData(); // 값을 문자열로 받아와서
                          
                                        if (OnOff == "1") {
                                            state = 1;
                                            
                                            analogWrite(D1, Br);
                                        }
                                        else {
                                            state = 0;
                                            
                                            analogWrite(D1,0);
                                        }  
                                    
                                      
                          }  
                          
                          else {
                                Serial.println(firebaseData.errorReason()); // 에러 메시지 출력
                          }
                    
                }


                                    ////////////////////////////////
                                    //  형우방  On/Off 상태 읽어오기 //
                                    ////////////////////////////////

                if (RoomName == 4) {                                         
                                      
                          if (Firebase.getString(firebaseData, "MyBoiler/HyungWoo Bang On")) { // 해당하는 key가 있으면 (앱 인벤터에서는 tag)
                            
                              String OnOff = firebaseData.stringData(); // 값을 문자열로 받아와서
                          
                                        if (OnOff == "1") {
                                            state = 1;
                                            analogWrite(D1, Br);
                                        }
                                        else {
                                            state = 0;
                                            analogWrite(D1, 0);
                                        }
                                    
                                      
                          }  
                          
                          else {
                                Serial.println(firebaseData.errorReason()); // 에러 메시지 출력
                          }

                }

                    
            } // 3초마다 On/Off 값 가져온다. D4에 On/Off 상태 표시한다.

      

            
}  // loop
