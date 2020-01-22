#include <M5StickC.h>
#include "DHT12.h"
#include <Wire.h>
#include "Adafruit_Sensor.h"
#include <Adafruit_BMP280.h>

#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "esp_sleep.h"

DHT12 dht12;
Adafruit_BMP280 bme;

#define T_PERIOD     10   // Transmission period
#define S_PERIOD     170  // Silent period
RTC_DATA_ATTR static uint8_t seq; // remember number of boots in RTC Memory

uint16_t temp;
uint16_t humid;
uint16_t press;
uint16_t systemp;
uint16_t vbat;

void setAdvData(BLEAdvertising *pAdvertising) {
    BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();

    oAdvertisementData.setFlags(0x06); // BR_EDR_NOT_SUPPORTED | LE General Discoverable Mode

    std::string strServiceData = "";
    strServiceData += (char)0x0e;   // 長さ
    strServiceData += (char)0xff;   // AD Type 0xFF: Manufacturer specific data
    strServiceData += (char)0xff;   // Test manufacture ID low byte
    strServiceData += (char)0xff;   // Test manufacture ID high byte
    strServiceData += (char)seq;                   // シーケンス番号
    strServiceData += (char)(temp & 0xff);         // 温度の下位バイト
    strServiceData += (char)((temp >> 8) & 0xff);  // 温度の上位バイト
    strServiceData += (char)(humid & 0xff);        // 湿度の下位バイト
    strServiceData += (char)((humid >> 8) & 0xff); // 湿度の上位バイト
    strServiceData += (char)(press & 0xff);        // 気圧の下位バイト
    strServiceData += (char)((press >> 8) & 0xff); // 気圧の上位バイト
    strServiceData += (char)(systemp & 0xff);         // 温度の下位バイト
    strServiceData += (char)((systemp >> 8) & 0xff);  // 温度の上位バイト
    strServiceData += (char)(vbat & 0xff);         // 電池電圧の下位バイト
    strServiceData += (char)((vbat >> 8) & 0xff);  // 電池電圧の上位バイト

    oAdvertisementData.addData(strServiceData);
    pAdvertising->setAdvertisementData(oAdvertisementData);
}

void setup() {
    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();
    if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
      Wire1.begin(21, 22);
      Wire1.setClock(400000);
      M5.Lcd.begin();
    } else {
      M5.begin(true, true, false);
      M5.Axp.ScreenBreath(7);    // 画面の輝度を下げる
    }
    M5.Lcd.setRotation(3);      // 左を上にする
    M5.Lcd.setTextSize(2);      // 文字サイズを2にする

    Wire.begin(0,26);           // I2Cを初期化する
    while (!bme.begin(0x76)) {  // BMP280を初期化する
        M5.Lcd.println("BMP280 init fail");
    }

    temp = (uint16_t)(dht12.readTemperature() * 100);
    humid = (uint16_t)(dht12.readHumidity() * 100);
    press = (uint16_t)(bme.readPressure() / 100 * 10);
    systemp = (uint16_t)(M5.Axp.GetTempInAXP192() * 100);
    
    vbat = (uint16_t)(M5.Axp.GetBatVoltage() * 100);

    M5.Lcd.setCursor(0, 0, 1);
    M5.Lcd.printf("temp: %4.1f'C\r\n", (float)temp / 100);
    M5.Lcd.printf("humid:%4.1f%%\r\n", (float)humid / 100);
    M5.Lcd.printf("press:%4.0fhPa\r\n", (float)press / 10);
    M5.Lcd.printf("stemp:%4.1f'C\r\n", (float)systemp / 100);
    M5.Lcd.printf("vbat: %4.2fV\r\n", (float)vbat / 100);

    BLEDevice::init("AmbientEnv-02");                  // デバイスを初期化
    BLEServer *pServer = BLEDevice::createServer();    // サーバーを生成

    BLEAdvertising *pAdvertising = pServer->getAdvertising(); // アドバタイズオブジェクトを取得
    setAdvData(pAdvertising);                          // アドバタイジングデーターをセット

    pAdvertising->start();                             // アドバタイズ起動
    delay(T_PERIOD * 1000);                            // T_PERIOD秒アドバタイズする
    pAdvertising->stop();                              // アドバタイズ停止

    seq++;                                             // シーケンス番号を更新
    delay(10);
    esp_deep_sleep(1000000LL * S_PERIOD);              // S_PERIOD秒Deep Sleepする
}

void loop() {
}
