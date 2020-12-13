//=====================================================================
//  Leafony Platform sample sketch
//     Application  : BLE 4-Sensers demo
//     Processor    : STM32L452RE (Nucleo-64/Nucleo L452RE)
//     Arduino IDE  : 1.8.13
//     STM32 Core   : Ver1.9.0
//
//     Leaf configuration
//       (1) AC02 BLE Sugar
//       (2) AI01 4-Sensors
//       (3) AP03 STM32 MCU
//       (4) AZ01 USB
//
//    (c) 2020 Trillion-Node Study Group
//    Released under the MIT license
//    https://opensource.org/licenses/MIT
//
//      Rev.00 2020/12/08 First release
//=====================================================================
//---------------------------------------------------------------------
// difinition
//---------------------------------------------------------------------
#include <SoftwareSerial.h>                 // Software UART
#include <Wire.h>                           // I2C

#include <Adafruit_LIS3DH.h>                // 3-axis accelerometer
#include <Adafruit_HTS221.h>                // humidity and temperature sensor
#include <ClosedCube_OPT3001.h>             // Ambient Light Sensor
#include "TBGLib.h"                         // BLE
#include <ST7032.h>                         // LCD


//===============================================
// BLE Unique Name (Local device name)
// 最大16文字（ASCIIコード）まで
//===============================================
//                     |1234567890123456|
String strDeviceName = "Leafony_AC02";

//===============================================
// シリアルモニタへの出力
//      #define SERIAL_MONITOR = 出力あり
//    //#define SERIAL_MONITOR = 出力なし（コメントアウトする）
//===============================================
#define SERIAL_MONITOR

//===============================================
// シリアルモニタへのデバック出力
//      #define DEBUG = 出力あり
//    //#define DEBUG = 出力なし（コメントアウトする）
//===============================================
//#define DEBUG

//-----------------------------------------------
// 送信間隔の設定
//  SEND_INTERVAL  :送信間隔（センサーデータを送る間隔  1秒単位
//-----------------------------------------------
#define SEND_INTERVAL   (1)                 // 1s

//-----------------------------------------------
// IOピン一覧
//-----------------------------------------------
//  D0  PA3  (UART2_RXD)
//  D1  PA2  (UART2_TXD)
//  D2  PC7  (INT0)
//  D3  PB3  (INT1)
//  D4  PB5
//  D5  PB4
//  D6  PA8
//  D7  PB12
//  D8  PA9  (UART1_TX)
//  D9  PA10 (UART1_RX)
//  D10 PB6  (SS)
//  D11 PA7  (MOSI)
//  D12 PA6  (MISO)
//  D13 PA5  (SCK)
//  D14 PB9  (SDA)
//  D15 PB8  (SCL)
//  A0  PA4
//  A1  PA0  (UART4_TX)
//  A2  PA1  (UART4_RX)
//  A3  PB0
//  A4  PC1
//  A5  PC0

//-----------------------------------------------
// IOピンの名前定義
// 接続するリーフに合わせて定義する
//-----------------------------------------------
#define BLE_WAKEUP      PB12                // D7   PB12
#define BLE_RX          PA1                 // [A2] PA1
#define BLE_TX          PA0                 // [A1] PA0

//-----------------------------------------------
// プログラム内で使用する定数定義
//-----------------------------------------------
//------------------------------
// I2Cアドレス
//------------------------------
#define LIS2DH_ADDRESS          0x19        // Accelerometer (SD0/SA0 pin = VCC)
#define OPT3001_ADDRESS         0x45        // Ambient Light Sensor (ADDR pin = VCC)
#define LCD_I2C_EXPANDER_ADDR   0x1A        // LCD I2C Expander
#define BATT_ADC_ADDR           0x50        // Battery ADC

//------------------------------
// Loop interval
// HardwareTimerのタイマ割り込み発生間隔(us)
//------------------------------
#define LOOP_INTERVAL 125000                // 125000us = 125ms interval

//------------------------------
// BLE
//------------------------------
#define BLE_STATE_STANDBY               (0)
#define BLE_STATE_SCANNING              (1)
#define BLE_STATE_ADVERTISING           (2)
#define BLE_STATE_CONNECTING            (3)
#define BLE_STATE_CONNECTED_MASTER      (4)
#define BLE_STATE_CONNECTED_SLAVE       (5)

//---------------------------------------------------------------------
// object
//---------------------------------------------------------------------
//------------------------------
// LCD
//------------------------------
  ST7032 lcd;

//------------------------------
// Sensor
//------------------------------
Adafruit_LIS3DH accel = Adafruit_LIS3DH();
ClosedCube_OPT3001 light;

//------------------------------
// BLE
//------------------------------
HardwareSerial Serialble(BLE_RX, BLE_TX);
BGLib ble112((HardwareSerial *)&Serialble, 0, 0 );

//---------------------------------------------------------------------
// プログラムで使用する変数定義
//---------------------------------------------------------------------
//------------------------------
// LCD
//------------------------------
bool dispLCD = 0;                           // LCDに表示する場合1にする
int8_t lcdSendCount = 0;

//------------------------------
// Loop counter
//------------------------------
uint8_t iLoop1s = 0;
uint8_t iSendCounter = 0;

//------------------------------
// Event
//------------------------------
bool event1s = false;

//------------------------------
// interval Timer2 interrupt
//------------------------------
volatile bool bInterval = false;

//------------------------------
// LIS2DH : Accelerometer
//------------------------------
float dataX_g, dataY_g, dataZ_g;
float dataTilt = 0;
uint8_t dataPips;

//------------------------------
// HTS221 : Humidity and Temperature sensor
//------------------------------
Adafruit_HTS221 hts;

float dataTemp = 0;
float dataHumid = 0;

//--------------------
// 2点補正用データ
//--------------------
// 温度補正用データ0
float TL0 = 25.0;     // 4-Sensors温度測定値
float TM0 = 25.0;     // 温度計等測定値
// 温度補正用データ1
float TL1 = 40.0;     // 4-Sensors温度測定値
float TM1 = 40.0;     // 温度計等測定値

// 湿度補正用データ0
float HL0 = 60.0;     // 4-Sensors湿度測定値
float HM0 = 60.0;     // 湿度計等測定値
// 湿度補正用データ1
float HL1 = 80.0;     // 4-Sensors湿度測定値
float HM1 = 80.0;     // 湿度計等測定値

//------------------------------
// OPT3001 : Ambient Light Sensor
//------------------------------
float dataLight;

//------------------------------
// BLE
//------------------------------
bool bBLEconnect = false;
bool bBLEsendData = false;
volatile bool bSystemBootBle = false;

volatile uint8_t ble_state = BLE_STATE_STANDBY;
volatile uint8_t ble_encrypted = 0;         // 0 = not encrypted, otherwise = encrypted
volatile uint8_t ble_bonding = 0xFF;        // 0xFF = no bonding, otherwise = bonding handle

//------------------------------
// Battery
//------------------------------
float dataBatt = 0;

//=====================================================================
// setup
//=====================================================================
void setup(){
//  delay(1000);

  Serial.begin(115200);     // UART 115200bps
  Wire.begin();             // I2C 100kHz
#ifdef SERIAL_MONITOR
    Serial.println(F(""));
    Serial.println(F("========================================="));
    Serial.println(F("setup start"));
#endif

  if (dispLCD==1){
    i2c_write_byte(LCD_I2C_EXPANDER_ADDR, 0x03, 0xFE);
    i2c_write_byte(LCD_I2C_EXPANDER_ADDR, 0x01, 0x01);      // LCD 電源ON
    // LCD設定
    lcd.begin(8, 2);
    lcd.setContrast(30);
    lcd.clear();
    lcd.print("NOW");
    lcd.setCursor(0, 1);
    lcd.print("BOOTING!");
  }

  setupPort();
  setupSensor();
  setupBLE();

  ble112.ble_cmd_system_get_bt_address();
  while (ble112.checkActivity(1000));

  setupTimerInt();                            // Timer inverval start

#ifdef SERIAL_MONITOR
    Serial.println(F(""));
    Serial.println("=========================================");
    Serial.println(F("loop start"));
    Serial.println(F(""));
#endif
}

//-----------------------------------------------
// IOピンの入出力設定
// 接続するリーフに合わせて設定する
//-----------------------------------------------
void setupPort(){
  pinMode(BLE_WAKEUP, OUTPUT);           // [D7] : BLE Wakeup/Sleep
  digitalWrite(BLE_WAKEUP, HIGH);        // BLE Wakeup
}

//---------------------------------------------------------------------
// 割り込み処理初期設定
// timer2 interrupt (interval=125ms, int=overflow)
// メインループのタイマー割り込み設定
//---------------------------------------------------------------------
//-----------------------------------------------
// Timer interrupt (interval=125ms, int=overflow)
// メインループのタイマー割り込み設定
//-----------------------------------------------
void setupTimerInt(){
  HardwareTimer *timer2 = new HardwareTimer (TIM2);

  timer2->setOverflow(LOOP_INTERVAL, MICROSEC_FORMAT);       // 125ms
  timer2->attachInterrupt(intTimer);
  timer2->resume();
}

//---------------------------------------------------------------------
// 各デバイスの初期設定
//---------------------------------------------------------------------
//------------------------------
// Sensor
//------------------------------
void setupSensor(){
  //----------------------------
  // LIS2DH (accelerometer)
  //----------------------------
  accel.begin(LIS2DH_ADDRESS);

  accel.setClick(0, 0);                      // Disable Interrupt
  accel.setRange(LIS3DH_RANGE_2_G);          // Full scale +/- 2G
  accel.setDataRate(LIS3DH_DATARATE_10_HZ);  // Data rate = 10Hz

  //----------------------------
  // HTS221 (temperature /humidity)
  //----------------------------
  while (!hts.begin_I2C()) {
#ifdef SERIAL_MONITOR
    Serial.println("Failed to find HTS221");
#endif
    delay(10);
  }

  //----------------------------
  // OPT3001 (light)
  //----------------------------
  OPT3001_Config newConfig;
  OPT3001_ErrorCode errorConfig;

  light.begin(OPT3001_ADDRESS);

  newConfig.RangeNumber = B1100;                    // automatic full scale
  newConfig.ConvertionTime = B1;                    // convertion time = 800ms
  newConfig.ModeOfConversionOperation = B11;        // continous conversion
  newConfig.Latch = B0;                             // hysteresis-style

  errorConfig = light.writeConfig(newConfig);

  if(errorConfig != NO_ERROR){
    errorConfig = light.writeConfig(newConfig);     // retry
  }
}
//====================================================================
// Loop
//=====================================================================
//---------------------------------------------------------------------
// Main loop
//---------------------------------------------------------------------
void loop() {
  //-----------------------------------------------------
  // Timer interval　125ms で1回ループ
  //-----------------------------------------------------
  if (bInterval == true){
     bInterval = false;

    //--------------------------------------------
    loopCounter();                    // loop counter
    //--------------------------------------------
    // 1sに1回実行する
    //--------------------------------------------
    if(event1s == true){
      event1s = false;
      loopSensor();                   // sensor read
      bt_sendData();                  // Data send
    }
  }
  loopBleRcv();
}
//---------------------------------------------------------------------
// Counter
// メインループのループ回数をカウントし
// 1秒間隔でセンサーデータの取得とBLEの送信をONにする
//---------------------------------------------------------------------
void loopCounter(){
  iLoop1s += 1;

  //--------------------
  // 1s period
  //--------------------
  if (iLoop1s >=  8){                 // 125ms x 8 = 1s
    iLoop1s = 0;

    iSendCounter  += 1;
    if (iSendCounter >= SEND_INTERVAL){
      iSendCounter = 0;
      event1s = true;
    }
  }
}

//---------------------------------------------------------------------
// Sensor
// センサーデータ取得がONのとき、各センサーからデータを取得
// コンソール出力がONのときシリアルに測定値と計算結果を出力する
//---------------------------------------------------------------------
void loopSensor(){
  double temp_mv;
    //-------------------------
    // LIS2DH
    // 3軸センサーのデータ取得
    //-------------------------
    accel.read();
    dataX_g = accel.x_g;    //X軸
    dataY_g = accel.y_g;    //Y軸
    dataZ_g = accel.z_g;    //Z軸

    if(dataZ_g >= 1.0){
      dataZ_g = 1.00;
    } else if (dataZ_g <= -1.0){
      dataZ_g = -1.00;
    }

    dataTilt = acos(dataZ_g) / PI * 180;

    //サイコロの目の位置を計算
    //各目でセンサーは以下の値を取る
    //     X  Y  Z
    //  1  0  0  1
    //  2  0  1  0
    //  3 -1  0  0
    //  4  1  0  0
    //  5  0 -1  0
    //  6  0  0 -1
    if ((-0.5 <= dataX_g && dataX_g < 0.5) && (-0.5 <= dataY_g && dataY_g < 0.5) && (0.5 <= dataZ_g)){
      dataPips = 1;
    }
    else if ((-0.5 <= dataX_g && dataX_g < 0.5) && (0.5 <= dataY_g) && (-0.5 <= dataZ_g && dataZ_g < 0.5)){
      dataPips = 2;
    }
    else if ((dataX_g < -0.5) && (-0.5 <= dataY_g && dataY_g < 0.5) && (-0.5 <= dataZ_g && dataZ_g < 0.5)){
      dataPips = 3;
    }
    else if ((0.5 <= dataX_g) && (-0.5 <= dataY_g && dataY_g < 0.5) && (-0.5 <= dataZ_g && dataZ_g < 0.5)){
      dataPips = 4;
    }
    else if ((-0.5 <= dataX_g && dataX_g < 0.5) && (dataY_g < -0.5) && (-0.5 <= dataZ_g && dataZ_g < 0.5)){
      dataPips = 5;
    }
    else if ((-0.5 <= dataX_g && dataX_g < 0.5) && (-0.5 <= dataY_g && dataY_g < 0.5) && (dataZ_g < -0.5)){
      dataPips = 6;
    }
//    else{
//      dataPips = 0;
//    }

    //-------------------------
    // HTS221
    // 温湿度センサーデータ取得
    //-------------------------
    sensors_event_t humidity;
    sensors_event_t temp;

    hts.getEvent(&humidity, &temp);             // populate temp and humidity objects with fresh data
    dataTemp = temp.temperature;
    dataHumid =humidity.relative_humidity;

    //-------------------------
    // 温度と湿度の2点補正
    //-------------------------
    dataTemp=TM0+(TM1-TM0)*(dataTemp-TL0)/(TL1-TL0);      // 温度補正
    dataHumid=HM0+(HM1-HM0)*(dataHumid-HL0)/(HL1-HL0);    // 湿度補正

    //-------------------------
    // OPT3001
    // 照度センサーデータ取得
    //-------------------------
    OPT3001 result = light.readResult();

    if(result.error == NO_ERROR){
      dataLight = result.lux;
    }

  //-------------------------
  // ADC081C027（ADC)
  // 電池リーフ電池電圧取得
  //-------------------------
  uint8_t adcVal1 = 0;
  uint8_t adcVal2 = 0;

  Wire.beginTransmission(BATT_ADC_ADDR);
  Wire.write(0x00);
  Wire.endTransmission(false);
  Wire.requestFrom(BATT_ADC_ADDR,2);
  adcVal1 = Wire.read();
  adcVal2 = Wire.read();

  if (adcVal1 == 0xff && adcVal2 == 0xff) {
    //測定値がFFならバッテリリーフはつながっていない
    adcVal1 = adcVal2 = 0;
  }

  //電圧計算　ADC　* （(リファレンス電圧(3.3V)/ ADCの分解能(256)) * 分圧比（２倍））
  temp_mv = ((double)((adcVal1 << 4) | (adcVal2 >> 4)) * 3300 * 2) / 256;
  dataBatt = (float)(temp_mv / 1000);
}

//---------------------------------------------------------------------
// Send sensor data
// センサーデータをセントラルに送る文字列に変換してBLEリーフへデータを送る
//---------------------------------------------------------------------
void bt_sendData(){
  float value;
  char temp[7], humid[7], light[7], tilt[7],battVolt[7], pips[7];
  char sendData[40];
  uint8 sendLen;

  //-------------------------
  //センサーデータを文字列に変換
  //dtostrf(変換する数字,変換される文字数,小数点以下の桁数,変換した文字の格納先);
  //変換される文字数を-にすると変換される文字は左詰め、+なら右詰めとなる
  //-------------------------
  //-------------------------
  // Temperature (4Byte)
  //-------------------------
  value = dataTemp;
  if(value >= 100){
    value = 99.9;
  }
  else if(value <= -10){
    value = -9.9;
  }
  dtostrf(value,4,1,temp);

  //-------------------------
  // Humidity (4Byte)
  //-------------------------
  value = dataHumid;
  dtostrf(value,4,1,humid);

  //-------------------------
  // Ambient Light (5Byte)
  //-------------------------
  value = dataLight;
  if(value >= 100000){
    value = 99999;
  }
  dtostrf(value,5,0,light);

  //-------------------------
  // Tilt (4Byte)
  //-------------------------
  value = dataTilt;
  if(value < 3){
    value = 0;
  }
  dtostrf(value,4,0,tilt);

  //-------------------------
  // dice
  //-------------------------
 value = dataPips;
  if (value > 6){
    value = 0;
  }
  dtostrf(value,4,0,pips);
  
  //-------------------------
  // Battery Voltage (4Byte)
  //-------------------------
  value = dataBatt;

  if (value >= 10){
   value = 9.99;
  }
  dtostrf(value, 4, 2, battVolt);

  //-------------------------
  trim(temp);
  trim(humid);
  trim(light);
  trim(tilt);
  trim(battVolt);
  trim(pips);

  lcd.clear();

  if (dispLCD==1){
    switch (lcdSendCount){
      case 0:                 // BLE未接続
        lcd.print("Waiting");
        lcd.setCursor(0, 1);
        lcd.print("connect");
        break;
      case 1:                 // Tmp XX.X [degC]
        lcd.print("Temp");
        lcd.setCursor(0, 1);
        lcd.print( String(temp) +" C");
        break;
      case 2:                 // Hum xx.x [%]
        lcd.print("Humidity");
        lcd.setCursor(0, 1);
        lcd.print( String(humid) +" %");
        break;
      case 3:                 // Lum XXXXX [lx]
        lcd.print("Luminous");
        lcd.setCursor(0, 1);
        lcd.print( String(light) +" lx");
        break;
      case 4:                 // Ang XXXX [arc deg]
        lcd.print("Angle");
        lcd.setCursor(0, 1);
        lcd.print( String(tilt) +" deg");
        break;
      case 5:                 // Bat X.XX [V]
        lcd.print("Battery");
        lcd.setCursor(0, 1);
        lcd.print( String(battVolt) +" V");
        break;
      default:
        break;
    }
    if (lcdSendCount < 5){
      lcdSendCount++;
    }
    else{
        if( bBLEsendData == true ){                           // BLE送信中は1から開始
          lcdSendCount = 1;
        }
        else{
          lcdSendCount = 0;
        }
    }
  }

  //-------------------------
  // BLE Send Data
  //-------------------------
  if( bBLEsendData == true ){                                 // BLE送信
    // WebBluetoothアプリ用フォーマット
    sendLen = sprintf(sendData, "%04s,%04s,%04s,%04s,%04s,%01s\n", temp, humid, light, tilt, battVolt, pips);
    // BLEデバイスへの送信
    ble112.ble_cmd_gatt_server_send_characteristic_notification( 1, 0x000C, sendLen, (const uint8 *)sendData );
    while (ble112.checkActivity(1000));
  }

    //-------------------------
    // シリアルモニタ表示
    //-------------------------
#ifdef SERIAL_MONITOR
// 項目別に表示する場合
/*
  Serial.println("--- sensor data ---");    
  Serial.println("  Tmp[degC]     = " + String(dataTemp));
  Serial.println("  Hum[%]        = " + String(dataHumid));
  Serial.println("  Lum[lx]       = " + String(dataLight));
  Serial.println("  Ang[arc deg]  = " + String(dataTilt));
  Serial.println("  Bat[V]        = " + String(dataBatt));
*/
// 1行に表示する場合
  Serial.println("SensorData: Temp=" + String(temp) + ", Humid=" + String(humid) + ", Light=" + String(light) + ", Tilt=" + String(tilt) + ", Vbat=" + String(battVolt) + ", Dice=" + String(pips));
#endif
}
//====================================================================

//==============================================
// 割り込み処理
//==============================================

//----------------------------------------------
// Timer INT
// タイマー割り込み関数
//----------------------------------------------
//void intTimer(HardwareTimer*){      // STM32 Core 1.7.0
void intTimer(void){                  // STM32 Core 1.9.0
  bInterval = true;
}

//---------------------------------------
// trim
// 文字列配列からSPを削除する
//---------------------------------------
void trim(char * data){
  int i = 0, j = 0;

  while (*(data + i) != '\0'){
    if (*(data + i) != ' '){
      *(data + j) = *(data + i);
      j++;
    }
    i++;
  }
  *(data + j) = '\0';
}

//=====================================================================
// I2C　制御関数
//=====================================================================
//-----------------------------------------------
// I2C スレーブデバイスに1バイト書き込む
//-----------------------------------------------
void i2c_write_byte(int device_address, int reg_address, int write_data){
  Wire.beginTransmission(device_address);
  Wire.write(reg_address);
  Wire.write(write_data);
  Wire.endTransmission();
}

//-----------------------------------------------
// I2C スレーブデバイスから1バイト読み込む
//-----------------------------------------------
unsigned char i2c_read_byte(int device_address, int reg_address){
  int read_data = 0;

  Wire.beginTransmission(device_address);
  Wire.write(reg_address);
  Wire.endTransmission(false);

  Wire.requestFrom(device_address, 1);
  read_data = Wire.read();

  return read_data;
}

//=====================================================================
// BLE
//=====================================================================
//-----------------------------------------------
//  Setup BLE
//-----------------------------------------------
void setupBLE(){
    uint8  stLen;
    uint8 adv_data[31];

    // set up internal status handlers (these are technically optional)
    ble112.onBusy = onBusy;
    ble112.onIdle = onIdle;
    ble112.onTimeout = onTimeout;
    // ONLY enable these if you are using the <wakeup_pin> parameter in your firmware's hardware.xml file
    // BLE module must be woken up before sending any UART data

    // set up BGLib response handlers (called almost immediately after sending commands)
    // (these are also technicaly optional)

    // set up BGLib event handlers
    /* [gatt_server] */
    ble112.ble_evt_gatt_server_attribute_value = my_evt_gatt_server_attribute_value;    /* [BGLib] */
    /* [le_connection] */
    ble112.ble_evt_le_connection_opend = my_evt_le_connection_opend;                    /* [BGLib] */
    ble112.ble_evt_le_connection_closed = my_evt_le_connection_closed;                  /* [BGLib] */
    /* [system] */
    ble112.ble_evt_system_boot = my_evt_system_boot;                                    /* [BGLib] */

    ble112.ble_evt_system_awake = my_evt_system_awake;
    ble112.ble_rsp_system_get_bt_address = my_rsp_system_get_bt_address;

    uint8_t tm=0;
    Serialble.begin(9600);
    while (!Serialble && tm <150){                              // Serial起動待ち タイムアウト1.5s
      tm++;
      delay(10);
    }

    tm=0;
    while (!bSystemBootBle && tm <150){                         // BLE起動待ち
      ble112.checkActivity(100);
      tm++;
      delay(10);
    }

    /* setting */
    /* [set Advertising Data] */
    uint8 ad_data[21] = {
        (2),                                                    // field length
        BGLIB_GAP_AD_TYPE_FLAGS,                                // field type (0x01)
        (6),                                                    // data
        (1),                                                    // field length (1は仮の初期値)
        BGLIB_GAP_AD_TYPE_LOCALNAME_COMPLETE                    // field type (0x09)
    };

    /*  */
    size_t lenStr2 = strDeviceName.length();

    ad_data[3] = (lenStr2 + 1);                                 // field length
    uint8 u8Index;
    for( u8Index=0; u8Index < lenStr2; u8Index++){
      ad_data[5 + u8Index] = strDeviceName.charAt(u8Index);
    }

    /*   */
    stLen = (5 + lenStr2);

    //ble112.ble_cmd_le_gap_bt5_set_adv_data(0,SCAN_RSP_ADVERTISING_PACKETS, stLen, ad_data);
    ble112.ble_cmd_le_gap_set_adv_data(SCAN_RSP_ADVERTISING_PACKETS, stLen, ad_data);

    while (ble112.checkActivity(1000));                         /* 受信チェック */
    delay(20);

    /* interval_min :   40ms( =   64 x 0.625ms ) */
    //ble112.ble_cmd_le_gap_bt5_set_adv_parameters( 0, 64, 1600, 7, 0 );/* [BGLIB] <handle> <interval_min> <interval_max> <channel_map> <report_scan>*/
    /* interval_max : 1000ms( = 1600 x 0.625ms ) */
    ble112.ble_cmd_le_gap_set_adv_parameters( 64, 1600, 7 );    /* [BGLIB] <interval_min> <interval_max> <channel_map> */

    while (ble112.checkActivity(1000));                         /* [BGLIB] 受信チェック */

    /* start */
//    ble112.ble_cmd_le_gap_bt5_set_mode(0,LE_GAP_USER_DATA,LE_GAP_UNDIRECTED_CONNECTABLE,0,2);
//    ble112.ble_cmd_le_gap_set_mode(LE_GAP_USER_DATA,LE_GAP_UNDIRECTED_CONNECTABLE);
//    ble112.ble_cmd_le_gap_start_advertising(0, LE_GAP_GENERAL_DISCOVERABLE, LE_GAP_UNDIRECTED_CONNECTABLE);     // index = 0
    ble112.ble_cmd_le_gap_start_advertising(0, LE_GAP_USER_DATA, LE_GAP_UNDIRECTED_CONNECTABLE);                // index = 0
    while (ble112.checkActivity(1000));                         /* 受信チェック */
    /*  */
}

//-----------------------------------------
// BLEからデータが送信されていたらデータを取得し、取得データに従い
// 処理を実施
//-----------------------------------------
void loopBleRcv(void){
    // keep polling for new data from BLE
    ble112.checkActivity(0);                                    /* 受信チェック */

    /*  */
    if (ble_state == BLE_STATE_STANDBY) {
        bBLEconnect = false;                                    /* [BLE] 接続状態 */
    } else if (ble_state == BLE_STATE_ADVERTISING) {
        bBLEconnect = false;                                    /* [BLE] 接続状態 */
    } else if (ble_state == BLE_STATE_CONNECTED_SLAVE) {
        /*  */
        bBLEconnect = true;                                     /* [BLE] 接続状態 */
    }
}

//=====================================================================
// INTERNAL BGLIB CLASS CALLBACK FUNCTIONS
//=====================================================================
//-----------------------------------------------
// called when the module begins sending a command
void onBusy() {
    // turn LED on when we're busy
    //digitalWrite( D13_LED, HIGH );
}

//-----------------------------------------------
// called when the module receives a complete response or "system_boot" event
void onIdle() {
    // turn LED off when we're no longer busy
    //digitalWrite( D13_LED, LOW );
}

//-----------------------------------------------
// called when the parser does not read the expected response in the specified time limit
void onTimeout() {
    // set state to ADVERTISING
    ble_state = BLE_STATE_ADVERTISING;

    // clear "encrypted" and "bonding" info
    ble_encrypted = 0;
    ble_bonding = 0xFF;
    /*  */
    bBLEconnect = false;                                        /* [BLE] 接続状態 */
    bBLEsendData = false;
#ifdef DEBUG
     Serial.println(F("on time out"));
#endif
}

//-----------------------------------------------
// called immediately before beginning UART TX of a command
void onBeforeTXCommand() {
}

//-----------------------------------------------
// called immediately after finishing UART TX
void onTXCommandComplete() {
    // allow module to return to sleep (assuming here that digital pin 5 is connected to the BLE wake-up pin)
#ifdef DEBUG
    Serial.println(F("onTXCommandComplete"));
#endif
}
/*  */

//-----------------------------------------------
void my_evt_gatt_server_attribute_value( const struct ble_msg_gatt_server_attribute_value_evt_t *msg ) {
    uint16 attribute = (uint16)msg -> attribute;
    uint16 offset = 0;
    uint8 value_len = msg -> value.len;
    uint8 value_data[20];
    String rcv_data;
    rcv_data = "";
    for (uint8_t i = 0; i < value_len; i++) {
        rcv_data += (char)(msg -> value.data[i]);
    }

#ifdef DEBUG
        Serial.print(F("###\tgatt_server_attribute_value: { "));
        Serial.print(F("connection: ")); Serial.print(msg -> connection, HEX);
        Serial.print(F(", attribute: ")); Serial.print((uint16_t)msg -> attribute, HEX);
        Serial.print(F(", att_opcode: ")); Serial.print(msg -> att_opcode, HEX);

        Serial.print(", offset: "); Serial.print((uint16_t)msg -> offset, HEX);
        Serial.print(", value_len: "); Serial.print(msg -> value.len, HEX);
        Serial.print(", value_data: "); Serial.print(rcv_data);

        Serial.println(F(" }"));
#endif

    if( rcv_data.indexOf("SND") == 0 ){
        bBLEsendData = true;
    } else if( rcv_data.indexOf("STP") == 0 ){
        bBLEsendData = false;
    }
}
/*  */

//-----------------------------------------------
void my_evt_le_connection_opend( const ble_msg_le_connection_opend_evt_t *msg ) {
    #ifdef DEBUG
        Serial.print(F("###\tconnection_opend: { "));
        Serial.print(F("address: "));
        // this is a "bd_addr" data type, which is a 6-byte uint8_t array
        for (uint8_t i = 0; i < 6; i++) {
            if (msg -> address.addr[i] < 16) Serial.write('0');
            Serial.print(msg -> address.addr[i], HEX);
        }
        Serial.print(", address_type: "); Serial.print(msg -> address_type, HEX);
        Serial.print(", master: "); Serial.print(msg -> master, HEX);
        Serial.print(", connection: "); Serial.print(msg -> connection, HEX);
        Serial.print(", bonding: "); Serial.print(msg -> bonding, HEX);
        Serial.print(", advertiser: "); Serial.print(msg -> advertiser, HEX);
        Serial.println(" }");
    #endif
    /*  */
    ble_state = BLE_STATE_CONNECTED_SLAVE;
}
/*  */
//-----------------------------------------------
void my_evt_le_connection_closed( const struct ble_msg_le_connection_closed_evt_t *msg ) {
    #ifdef DEBUG
        Serial.print(F("###\tconnection_closed: { "));
        Serial.print(F("reason: ")); Serial.print((uint16_t)msg -> reason, HEX);
        Serial.print(F(", connection: ")); Serial.print(msg -> connection, HEX);
        Serial.println(F(" }"));
    #endif

    // after disconnection, resume advertising as discoverable/connectable (with user-defined advertisement data)
//    ble112.ble_cmd_le_gap_start_advertising(1, LE_GAP_GENERAL_DISCOVERABLE, LE_GAP_UNDIRECTED_CONNECTABLE);
//    ble112.ble_cmd_le_gap_start_advertising(0, LE_GAP_USER_DATA, LE_GAP_UNDIRECTED_CONNECTABLE);              // index = 0
//    ble112.ble_cmd_le_gap_start_advertising(0, LE_GAP_GENERAL_DISCOVERABLE, LE_GAP_UNDIRECTED_CONNECTABLE);   // index = 0
//     ble112.ble_cmd_le_gap_set_mode(LE_GAP_GENERAL_DISCOVERABLE, LE_GAP_UNDIRECTED_CONNECTABLE );
     ble112.ble_cmd_le_gap_set_mode(LE_GAP_USER_DATA,LE_GAP_UNDIRECTED_CONNECTABLE); 

    while (ble112.checkActivity(1000));

    // set state to ADVERTISING
    ble_state = BLE_STATE_ADVERTISING;

    // clear "encrypted" and "bonding" info
    ble_encrypted = 0;
    ble_bonding = 0xFF;
    /*  */
    bBLEconnect = false;                                        /* [BLE] 接続状態 */
    bBLEsendData = false;
}
/*  */

//-----------------------------------------------
void my_evt_system_boot( const ble_msg_system_boot_evt_t *msg ){
    #ifdef DEBUG
        Serial.print( "###\tsystem_boot: { " );
        Serial.print( "major: " ); Serial.print(msg -> major, HEX);
        Serial.print( ", minor: " ); Serial.print(msg -> minor, HEX);
        Serial.print( ", patch: " ); Serial.print(msg -> patch, HEX);
        Serial.print( ", build: " ); Serial.print(msg -> build, HEX);
        Serial.print( ", bootloader_version: " ); Serial.print( msg -> bootloader, HEX );           /*  */
        Serial.print( ", hw: " ); Serial.print( msg -> hw, HEX );
        Serial.println( " }" );
    #endif

     bSystemBootBle = true;

    // set state to ADVERTISING
    ble_state = BLE_STATE_ADVERTISING;
}

//-----------------------------------------------
void my_evt_system_awake(void){
  ble112.ble_cmd_system_halt( 0 );
  while (ble112.checkActivity(1000));
}

//-----------------------------------------------
void my_rsp_system_get_bt_address(const struct ble_msg_system_get_bt_address_rsp_t *msg ){
#ifdef DEBUG
  Serial.print( "###\tsystem_get_bt_address: { " );
  Serial.print( "address: " );
  for (int i = 0; i < 6 ;i++){
    Serial.print(msg->address.addr[i],HEX);
  }
  Serial.println( " }" );
#endif

#ifdef SERIAL_MONITOR
  unsigned short addr = 0;
  char cAddr[30];
  addr = msg->address.addr[0] + (msg->address.addr[1] *0x100);
  sprintf(cAddr, "Device name is Leaf_A_#%05d ",addr);
  Serial.println(cAddr);
#endif
}