// Shortcut key for Mute using M5ATOM (matrix/lite)
//  long press  : mode chang
//  short press : mute ON/OFF

// mode (color / key)
// mode=0 : green (110, 190,  75) : WebEx    : Ctrl+M
// mode=1 : blue  ( 49, 198, 237) : Zoom     : Alt+A
// mode=2 : purple( 70,  71, 117) : MS Teams : Ctrl+Shift+M

// ref:
//   https://qiita.com/poruruba/items/6e4e29068a28f5ee1711
//   https://qiita.com/poruruba/items/eff3fedb1d4a63cbe08d
//   http://okiraku-camera.tokyo/blog/?p=8333

#include "M5Atom.h"
#include <Preferences.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "BLE2902.h"
#include "BLEHIDDevice.h"
#include "HIDTypes.h"
#include "HIDKeyboardTypes.h"

// BLEデバイス処理
BLEHIDDevice* hid;
BLECharacteristic* input;
BLECharacteristic* output;

uint8_t DisBuff[2 + 5 * 5 * 3];
void setBuff(uint8_t Rdata, uint8_t Gdata, uint8_t Bdata)
{
  DisBuff[0] = 0x05;
  DisBuff[1] = 0x05;
  for (int i = 0; i < 25; i++)
  {
    DisBuff[2 + i * 3 + 0] = Rdata;
    DisBuff[2 + i * 3 + 1] = Gdata;
    DisBuff[2 + i * 3 + 2] = Bdata;
  }
  M5.dis.displaybuff(DisBuff);
}

uint8_t mode = 0;
void setDisplay(uint8_t mode)
{
  Serial.print("mode="); Serial.println(mode);
  if (mode == 0) setBuff(0, 50, 0);
  else if (mode == 1) setBuff(0, 0, 50);
  else if (mode == 2) setBuff(50, 0, 50);
  else setBuff(0, 0, 0);
}

bool connected = false;

class MyCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer){
    connected = true;
    BLE2902* desc = (BLE2902*)input->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
    desc->setNotifications(true);
  }

  void onDisconnect(BLEServer* pServer){
    connected = false;
    BLE2902* desc = (BLE2902*)input->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
    desc->setNotifications(false);
  }
};

// ペアリング処理用
class MySecurity : public BLESecurityCallbacks {
  bool onConfirmPIN(uint32_t pin){
    return false;
  }

  uint32_t onPassKeyRequest(){
    Serial.println("ONPassKeyRequest");
    return 123456;
  }

  void onPassKeyNotify(uint32_t pass_key){
    // ペアリング時のPINの表示
    Serial.println("onPassKeyNotify number");
    Serial.println(pass_key);
  }

  bool onSecurityRequest(){
    Serial.println("onSecurityRequest");
    return true;
  }

  void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl){
    Serial.println("onAuthenticationComplete");
    if(cmpl.success){
      // ペアリング完了
      Serial.println("auth success");
    }else{
      // ペアリング失敗
      Serial.println("auth failed");
    }
  }
};

// BLEデバイスの起動
void taskServer(void*){
  BLEDevice::init("ATOMmute");

  BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT_MITM);
  BLEDevice::setSecurityCallbacks(new MySecurity());  

  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyCallbacks());

  hid = new BLEHIDDevice(pServer);
  input = hid->inputReport(1); // <-- input REPORTID from report map
  output = hid->outputReport(1); // <-- output REPORTID from report map

  std::string name = "akita11";
  hid->manufacturer()->setValue(name);

  hid->pnp(0x02, 0xe502, 0xa111, 0x0210);
  hid->hidInfo(0x00,0x02);

  BLESecurity *pSecurity = new BLESecurity();
//  pSecurity->setKeySize();

//  pSecurity->setAuthenticationMode(ESP_LE_AUTH_NO_BOND); // NO Bond
// AndroidではうまくPIN入力が機能しない場合有り
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);
  pSecurity->setCapability(ESP_IO_CAP_OUT);
  pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

  uint32_t pass_key = 123456;
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &pass_key, sizeof(uint32_t));

    const uint8_t report[] = {
      USAGE_PAGE(1),      0x01,       // Generic Desktop Ctrls
      USAGE(1),           0x06,       // Keyboard
      COLLECTION(1),      0x01,       // Application
      REPORT_ID(1),       0x01,        //   Report ID (1)
      USAGE_PAGE(1),      0x07,       //   Kbrd/Keypad
      USAGE_MINIMUM(1),   0xE0,
      USAGE_MAXIMUM(1),   0xE7,
      LOGICAL_MINIMUM(1), 0x00,
      LOGICAL_MAXIMUM(1), 0x01,
      REPORT_SIZE(1),     0x01,       //   1 byte (Modifier)
      REPORT_COUNT(1),    0x08,
      HIDINPUT(1),           0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position
      REPORT_COUNT(1),    0x01,       //   1 byte (Reserved)
      REPORT_SIZE(1),     0x08,
      HIDINPUT(1),           0x01,       //   Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position
      REPORT_COUNT(1),    0x06,       //   6 bytes (Keys)
      REPORT_SIZE(1),     0x08,
      LOGICAL_MINIMUM(1), 0x00,
      LOGICAL_MAXIMUM(1), 0x65,       //   101 keys
      USAGE_MINIMUM(1),   0x00,
      USAGE_MAXIMUM(1),   0x65,
      HIDINPUT(1),           0x00,       //   Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position
      REPORT_COUNT(1),    0x05,       //   5 bits (Num lock, Caps lock, Scroll lock, Compose, Kana)
      REPORT_SIZE(1),     0x01,
      USAGE_PAGE(1),      0x08,       //   LEDs
      USAGE_MINIMUM(1),   0x01,       //   Num Lock
      USAGE_MAXIMUM(1),   0x05,       //   Kana
      HIDOUTPUT(1),          0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile
      REPORT_COUNT(1),    0x01,       //   3 bits (Padding)
      REPORT_SIZE(1),     0x03,
      HIDOUTPUT(1),          0x01,       //   Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile
      END_COLLECTION(0)
    };

  hid->reportMap((uint8_t*)report, sizeof(report));
  hid->startServices();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->setAppearance(HID_KEYBOARD);
  pAdvertising->addServiceUUID(hid->hidService()->getUUID());
  pAdvertising->start();
  hid->setBatteryLevel(7);

  Serial.println("Advertising started");
  delay(portMAX_DELAY);
};

enum KEY_MODIFIER_MASK {
  KEY_MASK_CTRL = 0x01,
  KEY_MASK_SHIFT = 0x02,
  KEY_MASK_ALT = 0x04,
  KEY_MASK_WIN = 0x08
};

void setup() {
  M5.begin(true, false, true); // enable serial & display, disable I2C

  Serial.begin(115200);
  Serial.println("Start");
  setDisplay(mode);
  // BLEデバイスの起動処理の開始
  xTaskCreate(taskServer, "server", 20000, NULL, 5, NULL);

}

void loop() {
  M5.update();

  if (M5.Btn.pressedFor(1000)){
    Serial.println("long press");
    mode = (mode + 1) % 3;
    Serial.print("mode is switched to "); Serial.println(mode);
    setDisplay(mode);
    while(M5.Btn.read() == 1) delay(10);
  }
  else if( M5.Btn.wasPressed() ){
    Serial.println("Btn pressed");
    if(connected){
      // BLEキーボードとしてPCに接続されている状態の場合
      Serial.println("KBD");
      // Keycode (USB HID Usage) 
      // http://hp.vector.co.jp/authors/VA003720/lpproj/others/kbdjpn.htm
      // 1st arg: modifier; KEY_MASK_CTRL etc.

      Serial.println("press");
      uint8_t msg[] = {0x00, 0x00, 0x00, 0x0,0x0,0x0,0x0,0x0};
      if (mode == 0){msg[0] = KEY_MASK_CTRL; msg[2] = 0x10; }// Ctrl+M for WebEx
      else if (mode == 1){msg[0] = KEY_MASK_ALT; msg[2] = 0x04; }// Alt+A for Zoom
      else if (mode == 2){msg[0] = KEY_MASK_CTRL | KEY_MASK_SHIFT; msg[2] = 0x10; }// Ctrl+Shift+M for MS Teams
      input->setValue(msg, sizeof(msg));
      input->notify();
      Serial.println("release (key)");
      msg[2] = 0; input->setValue(msg, sizeof(msg)); input->notify();
      Serial.println("release (key2)");
      msg[0] = 0; input->setValue(msg, sizeof(msg)); input->notify();
    }
  }
}
