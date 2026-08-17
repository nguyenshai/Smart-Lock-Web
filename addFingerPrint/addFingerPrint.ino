#include <Adafruit_Fingerprint.h>

#define FINGER_RX 16
#define FINGER_TX 17

HardwareSerial mySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

uint8_t id;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  delay(100);
  Serial.println("\n\n=== CHUONG TRINH THEM VAN TAY (AS608) ===");

  mySerial.begin(57600, SERIAL_8N1, FINGER_RX, FINGER_TX);
  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("✅ Da tim thay cam bien van tay!");
  } else {
    Serial.println("❌ Khong tim thay cam bien. Hay kiem tra lai day P16, P17.");
    while (1) { delay(1); }
  }
}

uint8_t readnumber(void) {
  uint8_t num = 0;
  while (num == 0) {
    while (! Serial.available());
    num = Serial.parseInt();
  }
  return num;
}

void loop() {
  Serial.println("\n------------------------------------------------");
  Serial.println("1. NHAP ID (TU 1 DEN 127) VAO O TEXT ROI BAM ENTER:");
  id = readnumber();
  if (id == 0) {
     return;
  }
  Serial.print("Dang chuan bi luu van tay vao ID #");
  Serial.println(id);
  
  while (!getFingerprintEnroll() );
}

uint8_t getFingerprintEnroll() {

  int p = -1;
  Serial.println(">> 2. DAT NGON TAY CAN THEM VAO CAM BIEN...");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Da chup duoc anh lan 1!");
      break;
    case FINGERPRINT_NOFINGER:
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Loi anh");
      break;
    default:
      break;
    }
  }

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    Serial.println("❌ Anh bi mo hoac loi. Vui long thu lai.");
    return false;
  }

  Serial.println(">> 3. NHAC NGON TAY RA.");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  
  p = -1;
  Serial.println(">> 4. DAT LAI CHINH XAC NGON TAY DO VAO LAN NUA...");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Da chup duoc anh lan 2!");
      break;
    case FINGERPRINT_NOFINGER:
      break;
    default:
      break;
    }
  }

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    Serial.println("❌ Anh bi mo hoac loi. Vui long thu lai.");
    return false;
  }

  Serial.print("Dang xu ly tao khuon mau cho ID #");  Serial.println(id);
  
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Da tao xong Khuon mau!");
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("❌ HAI LAN QUET KHONG KHOP NHAU! Vui long lam lai.");
    return false;
  } else {
    Serial.println("Loi khong xac dinh");
    return false;
  }   
  
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("\n🎉 HOAN TAT! LUU VAN TAY THANH CONG VAO ID: " + String(id));
    Serial.println("------------------------------------------------");
    return true;
  } else {
    Serial.println("❌ Loi khi luu vao bo nho cam bien.");
    return false;
  }   
}
