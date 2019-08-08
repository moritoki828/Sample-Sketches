//=====================================================================
//  Leafony Platform sample sketch
//     Platform     : AVxx
//     Processor    : ATmega328P (3.3V /8MHz)
//     Application  : Battery voltage
//
//     Leaf configuration
//       (1) AP01 AVR MCU
//       (2) AV01 CR2032
//       (3) AZ01 USB
//
//		(c) 2019  Trillion-Node Study Group
//		Released under the MIT license
//		https://opensource.org/licenses/MIT
//
//      Rev.00 2019/08/01 First release
//=====================================================================
//use libraries
//=====================================================================

//=====================================================================
// difinition
//=====================================================================
#include <Wire.h>
#include <SoftwareSerial.h>
//=====================================================================

//=====================================================================
// IOピンの名前定義
// 接続するリーフに合わせて定義する
//=====================================================================
#define SDA       18
#define SCL       19

//=====================================================================
// プログラム内で使用する定数定義
// 
//=====================================================================
//-----------------------------------------------
// Batt ADC ADC081C027
//-----------------------------------------------
#define BATT_ADC_ADDR 0x50

//=====================================================================
// プログラムで使用する変数定義
// 
//=====================================================================
//=====================================================================
// RAM data
//=====================================================================
//---------------------------
// Batt
//---------------------------
float dataBatt = 0;

//=====================================================================
// setup
//=====================================================================
//-----------------------------------------------
// port
//-----------------------------------------------
//=====================================================================
// IOピンの入出力設定
// 接続するリーフに合わせて設定する
//=====================================================================
void setupPort(){

}

//====================================================================
// setup
//====================================================================
void setup() {
  Serial.begin(115200);     // Debug UART 115200bps
  Wire.begin();              // I2C 100kHz
 
  setupPort();
  delay(10);
}
//-----------------------------------------
// main loop
// コンソールに測定値を出力する
//-----------------------------------------
void loop(){
  double temp_mv;
  
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

    Serial.println("");
    Serial.println("  Batt[V]  = " + String(dataBatt));

  delay(1000);                       // wait for a second
}

//=====================================================================
// I2C　制御関数
// 
//=====================================================================
//-----------------------------------------------
//I2C スレーブデバイスに1バイト書き込む
//-----------------------------------------------
void i2c_write_byte(int device_address, int reg_address, int write_data){
  Wire.beginTransmission(device_address);
  Wire.write(reg_address);
  Wire.write(write_data);
  Wire.endTransmission();
}
//-----------------------------------------------
//I2C スレーブデバイスから1バイト読み込む
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

//---------------------------------------
// trim 
// 文字列配列からSPを削除する
//---------------------------------------
void trim(char * data)
{
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