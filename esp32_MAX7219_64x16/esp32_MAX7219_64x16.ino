#include <WiFi.h>
#include <WiFiServer.h>
#include <EEPROM.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <Bonezegei_DS3231.h>
#include <PS2Keyboard.h>
#include <TimeLib.h>
#include "Font_Data.h"

#define PRINT(s, v) { Serial.print(F(s)); Serial.println(v); }
#define PRINTS(s)   { Serial.println(F(s)); }

const char* ssid     = "ESP32_LedDisplay";
const char* password = "1234567890";
WiFiServer server(80);
Bonezegei_DS3231 rtc(0x68);
PS2Keyboard keyboard;

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_ZONES 2
#define ZONE_SIZE 8
#define MAX_DEVICES (MAX_ZONES * ZONE_SIZE)

#define ZONE_UPPER  1
#define ZONE_LOWER  0

#define CLK_PIN   18  // or SCK
#define DATA_PIN  23  // or MOSI
#define CS_PIN    5   // or SS
#define PS2_DATA  36  // INPUT
#define PS2_CLOCK 39  // INPUT

#define BARCODE_DISPLAY_TIME 30
#define TIMEMODE_DISPLAY_TIME 30

MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

int scroll_speed=100;
const uint8_t MESG_SIZE = 100;
char newMessage[MESG_SIZE];
bool newMessageAvailable = false;
bool PAUSE_DISPLAY = false;
bool TIME_MODE = false;
unsigned long ulPauseTime=0;
unsigned long ulTimeModeStart=0;
char dynamicWebPage[3000];

const char WebResponse[] = "HTTP/1.1 200 OK\nContent-Type:text/html\n\n";
const char WebPage[] =
"<!DOCTYPE html>" \
"<html>" \
"<head>" \
"<META HTTP-EQUIV=\"EXPIRES\" CONTENT=\"Mon, 22 Jul 2002 11:12:01 GMT\">" \
"<META name=\"viewport\" content=\"width=device-width, initial-scale=1\">" \
"<title>LED Matrix Text Setting</title>" \

"<script>" \
"strLine = \"\";" \

"function SendText() {" \
"  nocache = \"/&nocache=\" + Math.random() * 1000000;" \
"  var request = new XMLHttpRequest();" \
"  strLine = \"&MSG1=\" + document.getElementById(\"txt_form\").Message1.value + \"/&MSG2=\" + document.getElementById(\"txt_form\").Message2.value + \"/&MSG3=\" + document.getElementById(\"txt_form\").Message3.value+ \"/&TIME=\" + Date.now();" \
"  request.open(\"GET\", strLine + nocache, false);" \
"  request.send(null);" \
"}" \

"function ResetText() {" \
"  nocache = \"/&nocache=\" + Math.random() * 1000000;" \
"  var request = new XMLHttpRequest();" \
"  strLine = \"&MSG1=MK330/&MSG2=B/E Maker Space/&MSG3=Chihhao Lai/&TIME=\" + Date.now();" \
"  request.open(\"GET\", strLine + nocache, false);" \
"  request.send(null);" \
"}" \

"function ResetTime() {" \
"  nocache = \"/&nocache=\" + Math.random() * 1000000;" \
"  var request = new XMLHttpRequest();" \
"  strLine = \"&TIME=\" + Date.now();" \
"  request.open(\"GET\", strLine + nocache, false);" \
"  request.send(null);" \
"}" \

"</script>" \
"</head>" \

"<body>" \
"<p><b>Text Setting</b></p>" \

"<form id=\"txt_form\" name=\"frmText\">" \
"<label>1: <input type=\"text\" name=\"Message1\" maxlength=\"100\" value=\"__MSG1__\" ></label><br><br>" \
"<label>2: <input type=\"text\" name=\"Message2\" maxlength=\"100\" value=\"__MSG2__\" ></label><br><br>" \
"<label>3: <input type=\"text\" name=\"Message3\" maxlength=\"100\" value=\"__MSG3__\" ></label><br><br>" \
"</form>" \
"<br>" \
"<input type=\"button\" value=\"Send Text\" onclick=\"SendText()\">" \
"<input type=\"button\" value=\"Reset Text\" onclick=\"ResetText()\">" \
"<input type=\"button\" value=\"Reset Time\" onclick=\"ResetTime()\">" \
"</body>" \
"</html>";

char msg[3][200];
uint8_t msgIdx = 3;
char timeStamp[50];

char szTimeL[12];
char szTimeH[12];

void initializeFirstTime(){
  strcpy(msg[0], "MK330");
  strcpy(msg[1], "B/E Maker Space");
  strcpy(msg[2], "Chihhao Lai");
  saveBasicData();
}

void saveBasicData(){
  EEPROM_writeAnything(0, msg);
  delay(2);
}

//https://forum.arduino.cc/index.php?topic=41497.0
template <class T> int EEPROM_writeAnything(int ee, const T& value){
   const byte* p = (const byte*)(const void*)&value;
   int i;
   for (i = 0; i < sizeof(value); i++){
       EEPROM.write(ee++, *p++);
       vTaskDelay(1); //避免觸發看門狗
   }
   EEPROM.commit(); 
   return i;
}

//https://forum.arduino.cc/index.php?topic=41497.0
template <class T> int EEPROM_readAnything(int ee, T& value){
   byte* p = (byte*)(void*)&value;
   int i;
   for (i = 0; i < sizeof(value); i++){
       *p++ = EEPROM.read(ee++);
   }
   return i;
}

void readBasicData(){
  EEPROM_readAnything(0, msg);
  PRINT("msg[0]", msg[0]);
  PRINT("msg[1]", msg[1]);
  PRINT("msg[2]", msg[2]);
}

void setup(void){    
    Serial.begin(115200);
    PRINTS("[start]");

    //從EEPROM載入設定
    PRINTS("enable EEPROM");
    EEPROM.begin(4096); 
    //initializeFirstTime();
    readBasicData();

    PRINTS("enable RTC");
    rtc.begin();
    rtc.setFormat(24);

    PRINTS("enable KEYBOARD");
    keyboard.begin(PS2_DATA, PS2_CLOCK);
    keyboard.clear();

    PRINTS("Setting AP (Access Point)…");
    WiFi.softAP(ssid, password);
    IPAddress IP = WiFi.softAPIP();
    PRINT("AP IP address:", IP);
    server.begin();
      
    P.begin(MAX_ZONES);
    P.setZone(ZONE_UPPER, 0, ZONE_SIZE - 1);  
    P.setZone(ZONE_LOWER, ZONE_SIZE, MAX_DEVICES-1);
    P.setZoneEffect(ZONE_UPPER, true, PA_FLIP_UD);
    P.setZoneEffect(ZONE_UPPER, true, PA_FLIP_LR);
    P.setZoneEffect(ZONE_LOWER, true, PA_FLIP_UD);    
    P.setZoneEffect(ZONE_LOWER, true, PA_FLIP_LR);


}

void loop(void){
    handleWiFi();
    String lotString = checkBarCode();
    if(lotString.length() > 0){
        char message[50];
        lotString.toCharArray(message, lotString.length()+1);
        message[20] = '\0';
        PRINT("ScanInput: ", message);
        
        PAUSE_DISPLAY = true;
        ulPauseTime = millis();

        P.setFont(ZONE_UPPER, NULL);
        P.setFont(ZONE_LOWER, NULL);
        P.setCharSpacing(1);
        if(strlen(message)<11){
          P.displayZoneText(ZONE_UPPER, "Input:", PA_CENTER, 0, 0, PA_NO_EFFECT, PA_NO_EFFECT);
          P.displayZoneText(ZONE_LOWER, message, PA_CENTER, 0, 0, PA_NO_EFFECT, PA_NO_EFFECT);
        }
        else{
          char s1_10[11], s11_20[11];
          memcpy( s1_10,  &message[0],  10);  s1_10[10] = '\0';
          memcpy( s11_20, &message[10], 10); s11_20[10] = '\0';
          P.displayZoneText(ZONE_UPPER, s1_10, PA_CENTER, 0, 0, PA_NO_EFFECT, PA_NO_EFFECT);
          P.displayZoneText(ZONE_LOWER, s11_20, PA_CENTER, 0, 0, PA_NO_EFFECT, PA_NO_EFFECT); 
        }
        
        P.displayClear();
        P.synchZoneStart();  
        P.displayAnimate();
    }

    if(PAUSE_DISPLAY){
        if(millis() - ulPauseTime >= BARCODE_DISPLAY_TIME * 1000){  
            PAUSE_DISPLAY = false;
        }
    }

    if(!PAUSE_DISPLAY){    
        
        P.displayAnimate();
        if (isAnimationCompleted()){  
            switch (msgIdx){
            case 0:
                P.setFont(ZONE_UPPER, BigFontUpper);
                P.setFont(ZONE_LOWER, BigFontLower);
                scroll_speed = 50;
                P.setCharSpacing(1); 
                P.displayZoneText(ZONE_LOWER, msg[msgIdx], PA_CENTER, scroll_speed, 5000, PA_OPENING, PA_CLOSING);
                P.displayZoneText(ZONE_UPPER, msg[msgIdx], PA_CENTER, scroll_speed, 5000, PA_OPENING, PA_CLOSING);
                break;             
              
            case 1:  
            case 2:
                P.setFont(ZONE_UPPER, BigFontUpper);
                P.setFont(ZONE_LOWER, BigFontLower);
                scroll_speed = 30;
                P.setCharSpacing(2); 
                P.displayZoneText(ZONE_LOWER, msg[msgIdx], PA_LEFT, scroll_speed, 0, PA_SCROLL_RIGHT, PA_SCROLL_RIGHT);
                P.displayZoneText(ZONE_UPPER, msg[msgIdx], PA_LEFT, scroll_speed, 0, PA_SCROLL_RIGHT, PA_SCROLL_RIGHT);
                break;   
                
            case 3:  //Time Mode
                if(!TIME_MODE){
                  TIME_MODE = true;
                  ulTimeModeStart = millis();
                }      
                {      
                    getTime1();     
                    P.setFont(numeric7SegDouble);
                    P.setCharSpacing(2); 
                }
                {
//                    getTime2();
//                    P.setFont(ZONE_UPPER, NULL);
//                    P.setFont(ZONE_LOWER, NULL);
//                    P.setCharSpacing(1);
                }
                
                P.displayZoneText(ZONE_LOWER, szTimeL, PA_CENTER, 0, 1000, PA_PRINT, PA_NO_EFFECT);
                P.displayZoneText(ZONE_UPPER, szTimeH, PA_CENTER, 0, 1000, PA_PRINT, PA_NO_EFFECT);
                              
                break;  
                
            }

            if(TIME_MODE){
                if(millis() - ulTimeModeStart >= TIMEMODE_DISPLAY_TIME * 1000){  
                    TIME_MODE = false;
                }
            }
            
            if(!TIME_MODE){
              if(msgIdx++ > 3) msgIdx = 0;
            }            
            
            P.displayClear();
            P.synchZoneStart();
        }  
    }
    
    // delay(1);  
}

bool isAnimationCompleted(){
  return P.getZoneStatus(ZONE_LOWER) && P.getZoneStatus(ZONE_UPPER);
}

WiFiClient client;
enum { S_IDLE, S_WAIT_CONN, S_READ, S_EXTRACT, S_RESPONSE, S_DISCONN } state = S_IDLE;
char szBuf[1024];
uint16_t idxBuf = 0;  
uint32_t timeStart;
void handleWiFi(void){  
  bool needSave = false;
  switch (state){
  case S_IDLE:   // initialize
      PRINTS("S_IDLE");
      idxBuf = 0;
      state = S_WAIT_CONN;
      break;

  case S_WAIT_CONN:   // waiting for connection
      client = server.available();   // listen for incoming clients
      //client = server.accept();
      if (!client) break;
      if (!client.connected()) break;

      char szTxt[20];
      sprintf(szTxt, "%d:%d:%d:%d", client.remoteIP()[0], client.remoteIP()[1], client.remoteIP()[2], client.remoteIP()[3]);
      PRINT("New client @ ", szTxt);

      timeStart = millis();
      state = S_READ;
      break;

  case S_READ: // get the first line of data
      PRINTS("S_READ");
      while (client.available()){
          char c = client.read();
          if ((c == '\r') || (c == '\n')){
              szBuf[idxBuf] = '\0';
              //client.flush();
              PRINT("Recv: ", szBuf);
              state = S_EXTRACT;
              break;
          }
          else{
              szBuf[idxBuf++] = (char)c;
          }
      }
      if (millis() - timeStart > 1000){
          PRINTS("Wait timeout");
          state = S_DISCONN;
      }
      break;

  case S_EXTRACT: // extract data
      PRINTS("S_EXTRACT");
      needSave = false;
      if(getText(szBuf, newMessage, MESG_SIZE, "/&MSG1=")){          
          if(strlen(newMessage)>0){ strcpy(msg[0], newMessage); needSave = true;}
      }
      if(getText(szBuf, newMessage, MESG_SIZE, "/&MSG2=")){
          if(strlen(newMessage)>0){ strcpy(msg[1], newMessage); needSave = true;}
      }
      if(getText(szBuf, newMessage, MESG_SIZE, "/&MSG3=")){
          if(strlen(newMessage)>0){ strcpy(msg[2], newMessage); needSave = true;}
      }  
      if(getText(szBuf, timeStamp, 50, "/&TIME=")){
          PRINT("timeStamp: ", timeStamp);    
          setRtcTime(timeStamp);      
      }  

      if(needSave) { saveBasicData(); }
            
      readBasicData();

      
      state = S_RESPONSE;
      break;

  case S_RESPONSE: // send the response to the client
      PRINTS("S_RESPONSE");
      // Return the response to the client (web page)
      client.print(WebResponse);
      updateWebPage();
      client.print(dynamicWebPage);
      client.println();
      state = S_DISCONN;
      break;

  case S_DISCONN: // disconnect client
      PRINTS("S_DISCONN");
      client.flush();
      client.stop();
      state = S_IDLE;
      break;

  default:  state = S_IDLE;
  }
}

void setRtcTime(char *timeStr){
  timeStr[10] = '\0'; 
  unsigned long t = strtoul(timeStr, NULL, 10) + 28800;  //+8 hrs
  char _time[10], _date[10];
  sprintf(_date, "%d/%d/%d", month(t), day(t), year(t)-2000);  
  sprintf(_time, "%2d:%2d:%2d", hour(t), minute(t), second(t));
  rtc.setDate(_date);  //Set Date    Month/Date/Year
  rtc.setTime(_time);  //Set Time    Hour:Minute:Seconds  
  PRINT("_date: ", _date);
  PRINT("_time: ", _time);  
}

void updateWebPage(){  
  strcpy(dynamicWebPage, WebPage);
  strreplace(dynamicWebPage, "__MSG1__", msg[0]);
  strreplace(dynamicWebPage, "__MSG2__", msg[1]);
  strreplace(dynamicWebPage, "__MSG3__", msg[2]);  
}

char *strreplace(char *s, const char *s1, const char *s2) {
  // https://stackoverflow.com/questions/56043612/how-to-replace-a-part-of-a-string-with-another-substring
    char *p = strstr(s, s1);
    if (p != NULL) {
        size_t len1 = strlen(s1);
        size_t len2 = strlen(s2);
        if (len1 != len2)
            memmove(p + len2, p + len1, strlen(p + len1) + 1);
        memcpy(p, s2, len2);
    }
    return s;
}

boolean getText(char *szMesg, char *psz, uint8_t len, char *msgName){
  boolean isValid = false;  // text received flag
  char *pStart, *pEnd;      // pointer to start and end of text

  // get pointer to the beginning of the text
  //pStart = strstr(szMesg, "/&MSG=");
  pStart = strstr(szMesg, msgName);

  if (pStart != NULL)
  {
    pStart += strlen(msgName);  // skip to start of data
    pEnd = strstr(pStart, "/&");

    if (pEnd != NULL)
    {
      while (pStart != pEnd)
      {
        if ((*pStart == '%') && isxdigit(*(pStart+1)))
        {
          // replace %xx hex code with the ASCII character
          char c = 0;
          pStart++;
          c += (htoi(*pStart++) << 4);
          c += htoi(*pStart++);
          *psz++ = c;
        }
        else
          *psz++ = *pStart++;
      }

      *psz = '\0'; // terminate the string
      isValid = true;
    }
  }

  return(isValid);
}

uint8_t htoi(char c){
  c = toupper(c);
  if ((c >= '0') && (c <= '9')) return(c - '0');
  if ((c >= 'A') && (c <= 'F')) return(c - 'A' + 0xa);
  return(0);
}

String checkBarCode() {

  String barcodeString = "";   

  if (!keyboard.available()) { return ""; } //若無條碼輸入
  unsigned long timeStampKeyboard = 0;

  while (true) { //讀取條碼
    if (keyboard.available()) {    
      timeStampKeyboard = millis();  
      int c = keyboard.read();          
      if (c == 10 || c == 13) {continue;}  // 忽略換行
      if (!isGraph(c)) {continue;}  // 忽略印不出來的字元      
      if (c >= 97 && c <= 122) {c -= 32;} //轉大寫
      //Serial.print(c);

      barcodeString += String((char)c); 
    } 

    if(barcodeString.length() > 0){
      if(millis() - timeStampKeyboard >= 50) {                     
        break;                    
      }              
    }    
  }  
 
  if (barcodeString.length() == 0) {
      return ""; //若無條碼輸入
  }
  else{  
      //PRINT("Barcode Scan Input: ", barcodeString);
      return barcodeString;
  }
}

unsigned long ulLastGetTime=0;
bool flasher = true;
void getTime1(){  
  if (rtc.getTime()) {
    if(millis() - ulLastGetTime >= 1000){  
        ulLastGetTime = millis();  
        flasher = !flasher;
    }
    sprintf(szTimeL, "%02d%c%02d", rtc.getHour(), (flasher?':':' '), rtc.getMinute());
    createHString(szTimeH, szTimeL); 
    //PRINT("szTimeH: ", szTimeH);
    //PRINT("szTimeL: ", szTimeL);    
  }  
}
void createHString(char *pH, char *pL){
  for (; *pL != '\0'; pL++){
    *pH++ = *pL | 0x80;   // offset character
  }
  *pH = '\0'; // terminate the string
}
void getTime2(){  
  if (rtc.getTime()) {
    //Serial.printf("Time %02d:%02d:%02d ", rtc.getHour(), rtc.getMinute(), rtc.getSeconds());
    //Serial.printf("Date %02d-%02d-%d \n", rtc.getMonth(), rtc.getDate(), rtc.getYear());
    if(millis() - ulLastGetTime >= 1000){  
        ulLastGetTime = millis();  
        flasher = !flasher;
    }
    sprintf(szTimeH, "20%02d/%02d/%02d (%d)", rtc.getYear(), rtc.getMonth(), rtc.getDate()/*, rtc.getDay()*/);
    sprintf(szTimeL, "%02d%c%02d%c%02d", rtc.getHour(), (flasher?':':' '), rtc.getMinute(), (flasher?':':' '), rtc.getSeconds());
    
    PRINT("szTimeH: ", szTimeH);
    PRINT("szTimeL: ", szTimeL);    
  }  
}
