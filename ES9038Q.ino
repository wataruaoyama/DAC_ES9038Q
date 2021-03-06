/*
   Copyright (c) 2022/05/05 あおやまわたる

   以下に定める条件に従い、本ソフトウェアおよび関連文書のファイル（以下
   「ソフトウェア」）の複製を取得するすべての人に対し、ソフトウェアを無制
   限に扱うことを無償で許可します。これには、ソフトウェアの複製を使用、複
   写、変更、結合、掲載、頒布、サブライセンス、および/または販売する権利、
   およびソフトウェアを提供する相手に同じことを許可する権利も無制限に含ま
   れます。

   上記の著作権表示および本許諾表示を、ソフトウェアのすべての複製または重
   要な部分に記載するものとします。

   ソフトウェアは「現状のまま」で、明示であるか暗黙であるかを問わず、何ら
   の保証もなく提供されます。ここでいう保証とは、商品性、特定の目的への適
   合性、および権利非侵害についての保証も含みますが、それに限定されるもの
   ではありません。作者または著作権者は、契約行為、不法行為、またはそれ以
   外であろうと、ソフトウェアに起因または関連し、あるいはソフトウェアの使
   用またはその他の扱いによって生じる一切の請求、損害、その他の義務につい
   て何らの責任を負わないものとします。
*/
#include <Wire.h>
#include <avr/sleep.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define ES9038Q 0x48
#define master 2
#define LED 13
#define SLEEP 11
#define resetES9038Q A3
#define VOL A0

char noinput[] = "NO INPUT";
char i2s[] = "I2S ";
char dop[] = "DoP ";
char pcm[] = "PCM ";
char dsd[] = "DSD ";
char fs32[] = "32kHz";
char fs44[] = "44.1kHz";
char fs48[] = "48kHz";
char fs88[] = "88.2kHz";
char fs96[] = "96kHz";
char fs176[] = "176.4kHz";
char fs192[] = "192kHz";
char fs352[] = "352.8kHz";
char fs384[] = "384kHz";
char fs282[] = "2.8MHz";
char fs564[] = "5.6MHz";
char fs1128[] = "11.2MHz";
char fs2256[] = "22.4MHz";
char dsdfil[] = "DSD Filter";
char nofs[] = "";
char apodiz[] = "Apodizing Fast";
char brick[] = "Brick Wall";
char corrected[] = "Corrected Min. Phase";
char min_phase_slow[] = "Minimum Phase Slow";
char min_phase_fast[] = "Minimum Phase Fast";
char lin_phase_slow[] = "Linear Phase Slow";
char lin_phase_fast[] = "Linear Phase Fast";
char nofil[] = "";
char coa[] = "COA";
char opt[] = "OPT";
char locked[] = "Locked";
char unlocked[] = "Unlocked";

int vbuf = 0;
uint8_t jumperPins[] = {3, 4, 5, 6, 7, 9, 10};
uint8_t inputMode;
//126x64pixel SSD1306 OLED
Adafruit_SSD1306 display(128, 64, &Wire, -1);

void setup() {
  //
  int i;
  for (i = 0; i <= 6; i++)
    pinMode(jumperPins[i], INPUT_PULLUP);
  pinMode(SLEEP, INPUT_PULLUP);
  pinMode(resetES9038Q, OUTPUT);
  pinMode(VOL, INPUT);
  pinMode(master, OUTPUT);

  Wire.begin();
  Wire.setClock(400000);  // SCLクロックを400kHzに設定
  // SSD1306のスレーブアドレスは0x3C
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();
  Serial.begin(9600);
  delay(200);
  digitalWrite(resetES9038Q, HIGH); // リセット解除

  /*
     ES9038Q2Mをマスターモードで動作させる場合はHIGHにする.
     LVC126を介してMCLKが出力される。しかし、マスターモードでの動作は未確認^^;
  */
  digitalWrite(master, LOW);
//  Serial.println("master");
  initES9038Q();  // ES9038Q2Mを初期設定
//  Serial.println("initES9038Q");
//  delay(20);
  //digitalWrite(LED, 0);
  initOledDisplay();  // OLEDディスプレイの初期化
  delay(2000);
}

void loop() {
  uint8_t jumperValue = jumperState();
  uint8_t inputMode = i2cReadRegister(ES9038Q, 96);
  uint8_t chipID = i2cReadRegister(ES9038Q, 64);
  int fs = detectFS();

  inputSelect(jumperValue);
  filterSelect(jumperValue);
  volumeCtrl();
  messageOut(jumperValue, inputMode, fs, chipID);

  /*
     Atmega328Pをパワーダウンモードに設定
     復帰はリセット（電源再投入）のみで、その際SLEEPピンをオープンにしないと
     再度パワーダウンモードに入る。
  */
  if (digitalRead(SLEEP) == 0) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 10);
    display.println("SLEEP MODE");
    display.display();
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);  // スリープモードに設定
    sleep_enable(); // スリープ機能の有効化
    sleep_cpu();  // スリープ実行
  }

  delay(50);

}
/* ES9038Q2Mのレジスタの初期化 */
void initES9038Q() {
  Wire.beginTransmission(ES9038Q);
  // Register 0: System Registers レジスタ
  Wire.write(0);
  Wire.write(0x00); // MCLK = XI(100MHz)に設定
  // Register 1: Iput Selection レジスタ
  Wire.write(0xCC); // 
  // Register 2: Mixing,Serial Data and Automute Configurationレジスタ
  Wire.write(0x34); // default
  // Register 3: SPDIF Configuration レジスタ
  Wire.write(0x40); // default
  // Register 4: Automute Time レジスタ
  Wire.write(0x00); // default
  // Register 5: Automute Level レジスタ
  Wire.write(0x68); // default
  // Register 6: De-emphasisi,Dop and Voumu Ramp Rate
  Wire.write(0x4A); // Dopを有効化
  // Register 7: Filter Bandwidth and System Mute
  Wire.write(0x80); // default
  // Register 8: GPIO1-2 Configuration レジスタ
  Wire.write(0x99); // GPIO1,2ともInput Select
  // Register 9: Reserved
  Wire.write(0x22); // default
  // Register 10: Master Mode and Sync Configuration
  Wire.write(0x02); // default
  // Register 11: SPDIF Select レジスタ
  Wire.write(0x30); // デフォルトではCOAを選択
  // Register 12: ASRC/DPLL Bndwidth レジスタ
  Wire.write(0x5F); // ノーマル
  // Register 13: THD Bypass レジスタ
  Wire.write(0x40); // default
  // Register 14: Soft Start Cnfiguration レジスタ
  Wire.write(0x8A);   // soft_start ビットを'1'に設定
  // Register 15: Volume Control レジスタ
  Wire.write(0x00);   // Volume1 レジスタを"0x00"（出力レベル最大）に設定
  // Register 16: Volume control レジスタ
  Wire.write(0x00);   // Volume2 レジスタを"0x00"（出力レベル最大）に設定
  Wire.endTransmission();

//  Wire.beginTransmission(ES9038Q);
//  Wire.write(21);   // GPIO Input Selection レジスタを指定
//  Wire.write(0x50); // GPIO1,GPIO2ともSPDIFを選択
//  Wire.endTransmission();
  i2cWriteRegister(ES9038Q, 21, 0x50);

  /*
     JP8,JP9で歪補正係数とDPLL bandwidhtを変更する。
     DAC-ES9038Q基板では歪補正に関しては特に補正する必要がないのでデフォルト値
     を設定している。
     DPLL bandwidhはデフォルト（JP9オープン）ではノーマルとし、PCMにおいて
     最も高いパフォーマンスが発揮できる設定値を選択可能(JP9ショート)。
  */
  uint8_t thdAndDpllBandwidth = jumperState();
  thdAndDpllBandwidth &= 0x0E;
  if ((thdAndDpllBandwidth == 0x00 or thdAndDpllBandwidth == 0x04)) {
    thdCompensation(0x0000, 0x0000, 0x00);
  }
  else if ((thdAndDpllBandwidth == 0x08) or (thdAndDpllBandwidth == 0x00)) {
    i2cWriteRegister(ES9038Q, 12, 0x1F);
  }
}

/* OLEDの初期化 */
void initOledDisplay() {
  // Adafruitのロゴ表示データを消去
  display.clearDisplay();
  display.display();
  /* SSD1306 OLEDディスプレイに表示 */
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(10, 10);
  display.println("ES9038Q2M DAC Board");
  display.setCursor(30, 30);
  display.println("Desgined by");
  display.setTextSize(2);
  display.setCursor(20, 50);
  display.println("LINUXCOM");
  display.display();
}

void inputSelect(uint8_t input) {
  input &= 0x03;
//    Serial.print("Input =  ");
//    Serial.println(input,HEX);
  if (input == 0x01) {
    i2cWriteRegister(ES9038Q, 11, 0x30);
    i2cWriteRegister(ES9038Q, 1, 0xC1);
  }
  else if (input == 0x02) {
    i2cWriteRegister(ES9038Q, 11, 0x40);
    i2cWriteRegister(ES9038Q, 1, 0xC1);
  }
  else if (input == 0x03) {
    i2cWriteRegister(ES9038Q, 11, 0x00);
    i2cWriteRegister(ES9038Q, 1, 0xCC);
  }
}

/*
   サンプリング周波数の検出
   dpllNumberは各サンプルレート再生時のRegister68,69を読んで決定しているだけ。
*/
int detectFS() {
  int FSR;
  int dpllNum[4];
  uint32_t dpllNumber;
  int i;

  Wire.beginTransmission(ES9038Q);
  Wire.write(66);
  Wire.endTransmission();
  Wire.requestFrom(ES9038Q, 4);
  for (i = 0; i <= 3; i++) {
    dpllNum[i] = Wire.read();
    //    Serial.print("Dpll Number");
    //    Serial.print(i+1);
    //    Serial.print(" = ");
    //    Serial.println(dpllNum[i],HEX);
  }
  dpllNum[3] <<= 8;
  dpllNumber = dpllNum[3] | dpllNum[2];

  if (dpllNumber == 20)
    FSR = 32;
  else if (dpllNumber == 28)
    FSR = 44;
  else if (dpllNumber == 31)
    FSR = 48;
  else if (dpllNumber == 57)
    FSR = 88;
  else if (dpllNumber == 62)
    FSR = 96;
  else if (dpllNumber == 115)
    FSR = 176;
  else if (dpllNumber == 125)
    FSR = 192;
  else if (dpllNumber == 230)
    FSR = 352;
  else if (dpllNumber == 231)
    FSR = 352;
  else if (dpllNumber == 251)
    FSR = 384;
  else if (dpllNumber == 1849)
    FSR = 282;
  else if (dpllNumber == 3699)
    FSR = 564;
  else if (dpllNumber == 7398)
    FSR = 1128;
  else if (dpllNumber == 14797)
    FSR = 2256;
  else FSR = 0;

  //  dpllNum[1] <<= 8;
  //  dpllNum[2] <<= 16;
  //  dpllNum[3] <<= 24;
  //  dpllNumber = dpllNum[0] | dpllNum[1] | dpllNum[2] | dpllNum[3];
  //  FS = dpllNumber * 25 / 4294967296;

  return (FSR);
}

/* ジャンパーピンの接続状態を取得する */
uint8_t jumperState() {
  int i = 0;
  uint8_t j = 0;
  // Get the jumpers state
  for (i = 0; i <= 6; i++) {
    j = (j << 1) | digitalRead(jumperPins[i]);
  }
//    Serial.print("j=");     // SCLを100kHzにした場合、何故かコメントにすると
//    Serial.println(j,HEX);  // おかしな動きになる
  return j;
}

/* 7種類の8倍オーバーサンプリング・デジタルフィルタの選択 */
void filterSelect(uint8_t filterShape) {
  filterShape &= 0x70;
  Wire.beginTransmission(ES9038Q);
  Wire.write(7);
  if (filterShape == 0x70)      // apodizing fast roll-off filter
    Wire.write(0x80);
  else if (filterShape == 0x60) // brick wall filter
    Wire.write(0xE0);
  else if (filterShape == 0x50) // corrected minimum phase fast roll-off filter
    Wire.write(0xC0);
  else if (filterShape == 0x40) // minimum phase slow roll-off filter
    Wire.write(0x60);
  else if (filterShape == 0x30) // minimum phase fast roll-off filter
    Wire.write(0x40);
  else if (filterShape == 0x20) // linear phase slow roll-off filter
    Wire.write(0x20);
  else if (filterShape == 0x10) // linear phase fast roll-off filter
    Wire.write(0x00);
  else
    Wire.write(0x80);           // apodizing fast roll-off filter
  Wire.endTransmission();
  //  Serial.print("Filter Switch = ");
  //  Serial.println(filterShape, HEX);
}

/* 歪補正係数の設定 */
void thdCompensation(int16_t c2, int16_t c3, int8_t en) {
  int8_t c2lower = c2;
  int8_t c2uper = c2 >> 8;
  int8_t c3lower = c3;
  int8_t c3uper = c3 >> 8;
  Wire.beginTransmission(ES9038Q);
  Wire.write(13);   // THD Bypass レジスタを指定
  Wire.write(en);   // THD Compensation の有効化
  Wire.endTransmission();
  Wire.beginTransmission(ES9038Q);
  Wire.write(22);       // THD Compensation レジスタを指定
  Wire.write(c2lower);  // Second harmonic distortionの係数下位8bit
  Wire.write(c2uper);   // Second harmonic distortionの係数上位8bit
  Wire.write(c3lower);  // Third harmonic distortionの係数下位8bit
  Wire.write(c3uper);   // Third harmonic distortionの係数上位8bit
  Wire.endTransmission();
}

/* アッテネータ・レベルの設定 */
void volumeCtrl() {
  int vin = analogRead(VOL);
  vin = vin >> 3;
  if ( vin != vbuf ) {
    Wire.beginTransmission(ES9038Q);
    Wire.write(15);   // Volume Control レジスタを指定    
    if ( vin >= 127 ) {
      Wire.write(0xFF);
      Wire.write(0xFF);
    } else {
    Wire.write(uint8_t(vin));  //
    Wire.write(uint8_t(vin));
    }
    Wire.endTransmission();
  }
  vbuf = vin;
//  Serial.print("vin = ");
//  Serial.println(vin);
}

/* 表示器への出力　*/
void messageOut(uint8_t jumperValue, uint8_t inputMode, int fs, uint8_t chipID) {
  //  Serial.print("Input Mode = ");
  //  Serial.println(inputMode,HEX);
  display.clearDisplay();
  displayOledFSR(fs, inputMode);
  displayOledLockStatus(chipID);
  displayOledFilter(jumperValue, inputMode);
  displayOledPlayMode(inputMode);
  displayOledInput(inputMode, jumperValue);
  uint8_t vol = i2cReadRegister(ES9038Q, 15);//readVolumeValue();
//  Serial.print("vol = ");
//  Serial.println(vol);
  displayOledVolume(vol);
  display.display();
}

/* サンプリング周波数の表示 */
void displayOledFSR(int FSR, uint8_t pm) {
  display.setTextSize(2);
  display.setCursor(30, 26);
  if (FSR == 32) display.println(fs32);
  else if (FSR == 44) display.println(fs44);
  else if (FSR == 48) display.println(fs48);
  else if (FSR == 88) display.println(fs88);
  else if (FSR == 96) display.println(fs96);
  else if (FSR == 176) {
    if (pm == 0x0A) display.println(fs282);
    else display.println(fs176);
  }
  else if (FSR == 192) display.println(fs192);
  else if (FSR == 352) {
    if (pm == 0x0A) display.println(fs564);
    else display.println(fs352);
  }
  else if (FSR == 384) display.println(fs384);
  else if (FSR == 282) display.println(fs282);
  else if (FSR == 564) display.println(fs564);
  else if (FSR == 1128) display.println(fs1128);
  else if (FSR == 2256) display.println(fs2256);
  else display.println(nofs);
}

/* デジタル・フィルタ特性の表示 */
void displayOledFilter(uint8_t fil, uint8_t pm) {
  fil &= 0x70;
  display.setTextSize(1);
  display.setCursor(0, 56);
  if ((pm == 0x02) || (pm == 0x04)) {
    if ((fil == 0x70) || (fil == 0x00)) display.println(apodiz);
    else if (fil == 0x60) display.println(brick);
    else if (fil == 0x50) display.println(corrected);
    else if (fil == 0x40) display.println(min_phase_slow);
    else if (fil == 0x30) display.println(min_phase_fast);
    else if (fil == 0x20) display.println(lin_phase_slow);
    else if (fil == 0x10) display.println(lin_phase_fast);  // SCLを100kHzにした場合、何故かコメントアウト
                                                            // しないとおかしな動きになる
    else display.println(apodiz);
  }
  else if ((pm == 0x01) || (pm == 0x0A) || (pm == 0x0C)) display.println(dsdfil);
}

/* 再生モードの表示 */
void displayOledPlayMode(uint8_t pm) {
  display.setTextSize(2);
  display.setCursor(0, 0);
  if (pm == 0x0A) display.println(dop);
  else if (pm == 0x0C) display.println(dop);
  else if (pm == 0x02) display.println(pcm);
  else if (pm == 0x01) display.println(dsd);
  else if (pm == 0x04) display.println(pcm);
}

void displayOledVolume(uint8_t vol) {
  uint8_t value = vol / 2;
  uint8_t decimalPart = vol % 2;
  //  Serial.print("VLEVEL=");
  //  Serial.println(value);
  char buf[8];
  snprintf(buf, 8, "%d", value);
  display.setTextSize(1);
  display.setCursor(85, 4);
  display.print(buf);
  if (decimalPart == 0) {
    display.print(".0");
  } else {
    display.print(".5");    
  }
  display.println("dB");
}

void displayOledInput(uint8_t inputSelection, uint8_t input) {
  inputSelection &= 0x06;
  input &= 0x03;
  display.setTextSize(1);
  display.setCursor(52, 4);
  if (input == 0x03) display.println(i2s);
  if ( inputSelection == 0x04 ) {
    if (input == 0x01) display.println(coa);
    else if (input == 0x02) display.println(opt);
  }
}

/* DPLLのアンロック状態の表示 */
void displayOledLockStatus(uint8_t lockStatus) {
  lockStatus &= 0x01;
  display.setTextSize(2);
  display.setCursor(20, 26);
  if (lockStatus == 0x00) display.println(unlocked);
  //  else if (lockStatus == 0x01) display.println(locked);
}

uint8_t i2cReadRegister(uint8_t sladr, uint8_t regadr){
  Wire.beginTransmission(sladr);
  Wire.write(regadr);
  Wire.endTransmission();
  Wire.requestFrom(sladr, 1);
  return Wire.read();
}

uint8_t i2cWriteRegister(uint8_t sladr, uint8_t regadr, uint8_t wdata){
  Wire.beginTransmission(sladr);
  Wire.write(regadr);
  Wire.write(wdata);
  Wire.endTransmission();
}
