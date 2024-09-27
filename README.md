# ESP32 跑馬燈  

## 功能  
1. 跑馬燈，三條訊息輪播 (僅支援英文)  
2. 顯示時間  
3. 顯示條碼機輸入 (最多20碼)  
4. 透過 wifi 設定跑馬燈的訊息與時間  
    (1) 連接 SSID: ESP32_LedDisplay，密碼: 1234567890  
    (2) 瀏覽 http://192.168.4.1  

![image](https://github.com/Chihhao/esp32_MAX7219_64x16/blob/main/Images/01.jpg)  
![image](https://github.com/Chihhao/esp32_MAX7219_64x16/blob/main/Images/02.jpg)  
![image](https://github.com/Chihhao/esp32_MAX7219_64x16/blob/main/Images/03.jpg)  
![image](https://github.com/Chihhao/esp32_MAX7219_64x16/blob/main/Images/04.jpg)  
![image](https://github.com/Chihhao/esp32_MAX7219_64x16/blob/main/Images/05.jpg)  
![image](https://github.com/Chihhao/esp32_MAX7219_64x16/blob/main/Images/06.jpg)  

## 材料
(1) NodeMCU-32S  
(2) MAX7219 LED點矩陣顯示模組 (64x16)  
(3) DS3231 RTC模組  
(4) Reset Button  
(5) PS/2 母座  
(6) PCB (JLCPCB)  
(7) 3D 列印外殼  
(8) 強力磁鐵 (直徑 18.2mm / 厚 2mm)  

## Arduino Library Version  
(1) IDE: Arduino 1.8.19  
(2) Board Library: ESP32 3.0.4  
(3) MD_MAX72XX 3.5.1  
(4) MD_Parola  3.7.3  
(5) Bonezegei_DS3231 1.0.2  
(6) PS2Keyboard 2.4  
(7) Time 1.6.1  

## 原理圖  
![image](https://github.com/Chihhao/esp32_MAX7219_64x16/blob/main/Images/scheme.png)  

## PCB
![image](https://github.com/Chihhao/esp32_MAX7219_64x16/blob/main/Images/pcb1.png)  
![image](https://github.com/Chihhao/esp32_MAX7219_64x16/blob/main/Images/pcb2.png)  

## 3D Model  
* Tinkercad Model: https://www.tinkercad.com/things/0vOz0woKdCv-  
![image](https://github.com/Chihhao/esp32_MAX7219_64x16/blob/main/Images/case.png)  
