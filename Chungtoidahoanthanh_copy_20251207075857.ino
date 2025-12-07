    /*
    * ĐỒ ÁN MÁY LỌC KHÔNG KHÍ (DATN) - PHIÊN BẢN ESP32 HOÀN CHỈNH
    */
    //  KHAI BÁO THƯ VIỆN
    // Core & WiFi & NTP
    #include <Arduino.h>
    #include <WiFi.h>
    #include <WiFiManager.h> // <-- Thêm WiFiManager
    #include <WiFiUdp.h>
    #include <NTPClient.h>

    // Firebase
    #include <Firebase_ESP_Client.h>
    #include "addons/TokenHelper.h"
    #include "addons/RTDBHelper.h"

    // JSON
    #include <ArduinoJson.h>

    // Cảm biến I2C (SDA: 21, SCL: 22)
    #include <Wire.h>
    #include "RTClib.h" // Thư viện RTC
    #include <Adafruit_SHT31.h>
    #include <SparkFunCCS811.h>

    // Cảm biến UART (SDS011)
    #include <SdsDustSensor.h>

    //  ĐỊNH NGHĨA CHÂN (PIN) VÀ CÀI ĐẶT
    // WiFi & Firebase
    #define API_KEY "AIzaSyA6e3mTRJTL2qnhaAE9EM--wNlKt5RYNUM"
    #define DTBU "https://test2-f5585-default-rtdb.asia-southeast1.firebasedatabase.app/"

    // Chân Nút Cấu Hình WiFi
    const int CONFIG_BUTTON_PIN = 23; // Chân IO0 (thường là nút "BOOT" trên mạch)

    // Chân I2C
    const int I2C_SDA = 21;
    const int I2C_SCL = 22;

    // Chân UART HMI (Sử dụng Serial1)
    const int HMI_RX = 16;
    const int HMI_TX = 17;

    // Chân UART SDS011 (Sử dụng Serial2)
    const int SDS_RX = 18;
    const int SDS_TX = 19;

    // Chân Điều khiển
    const int RELAY_PIN = 32;
    const int FAN_ENA = 25;
    const int FAN_IN1 = 26;
    const int FAN_IN2 = 27;
    const int BUZZER_PIN = 14;

    // Cài đặt PWM
    const int FAN_PWM_CHANNEL = 0;
    const int FAN_PWM_FREQ = 1000;
    const int FAN_PWM_RESOLUTION = 8;

    // Khởi tạo Cảm biến & RTC
    #define CCS811_ADDR 0x5A
    CCS811 mySensor(CCS811_ADDR);
    Adafruit_SHT31 sht31 = Adafruit_SHT31();
    SdsDustSensor sds(Serial2);
    RTC_DS3231 rtc; // Dùng DS3231 làm nguồn thời gian chính

    // Cài đặt NTP (Chỉ để đồng bộ)
    WiFiUDP ntpUDP;
    NTPClient timeClient(ntpUDP, "pool.ntp.org");

    // Cài đặt Firebase
    FirebaseData fbdo;
    FirebaseAuth auth;
    FirebaseConfig config;
    bool signUp = false;

    // =============================================================
    //  BIẾN TOÀN CỤC (GLOBALS)
    // =============================================================

    // Biến quản lý trạng thái Online/Offline
    bool isOnline = false;           // Cờ báo trạng thái Online
    unsigned long lastWifiCheck = 0; // Timer kiểm tra Wi-Fi
    bool configPortalActive = false; // Cờ báo cổng cấu hình đang chạy
    unsigned long lastRTCSync = 0;   // Timer đồng bộ RTC

    // Biến điều khiển
    int Mode = 1, Ion = 0, Speed = 0, OnOff = 0;
    int ssMode = 1, ssIon = 0, ssSpeed = 0, ssOnOff = 0;
    bool fbUpdateMode = false, fbUpdateIon = false, fbUpdateSpeed = false, fbUpdateOnOff = false;

    // Biến cảm biến
    float Temp = 0, Humid = 0;
    int CO2 = 0, TVOC = 0;
    float PM25 = 0, PM10 = 0;
    int AQI_h = 0;

    // Biến hẹn giờ (đọc từ Firebase)
    int ClockOnCheck = 0, HourOn = 0, MinuteOn = 0;
    int ClockOffCheck = 0, HourOff = 0, MinuteOff = 0;

    // Biến tính toán AQI (dùng RTC)
    float sum_sec_PM25 = 0, sum_sec_PM10 = 0, sum_min_PM25 = 0, sum_min_PM10 = 0;
    int pre_minute = 0, count_sec = 0, count_min = 0;
    float input_PM25[24] = { 20.2, 15.15, 16.38, 18.97, 18.72, 25.7, 19.9, 20.2, 20.83, 27.45, 33.2, 33.17, 27.9, 8.37, 9.8, 21.75, 16.12, 17.05, 13.52, 16.78, 18.83, 19.58, 25.4, 19.77 };
    float inputAQI_h[24] = { 0 };
    bool check = true;
    int dem = 0;

    // Timers
    unsigned long lastSensorRead = 0, lastFirebaseCheck = 0, lastHMITimer = 0;
    unsigned long lastWarningBeep = 0; // Bạn đã có dòng này

  // <-- THÊM 3 DÒNG NÀY VÀO
  unsigned long warningSnoozeUntil = 0; // 0 = chưa tắt. >0 = tắt đến thời điểm này
  bool snoozeBeepDone = false; // Cờ để đảm bảo chỉ kêu bíp (xác nhận snooze) 1 lần
  bool configBeepDone = false; // Cờ
  String lastWarningMessage = "";


    // =============================================================
    //  HÀM XỬ LÝ AQI (Giữ nguyên)
    // =============================================================
    void shiftArray(float arr[], float newValue) {
      for (int i = 0; i < 23; i++) { arr[i] = arr[i + 1]; }
      arr[23] = newValue;
    }
    float findMin(float arr[]) {
      float min = arr[0];
      for (int i = 1; i < 12; i++) { if (arr[i] < min) { min = arr[i]; } }
      return min;
    }
    float findMax(float arr[]) {
      float max = arr[0];
      for (int i = 1; i < 12; i++) { if (arr[i] > max) { max = arr[i]; } }
      return max;
    }
    float calculate_nowcast(float arr[]) {
      float w, nowcast = 0, sum1 = 0, sum2 = 0;
      float input_nowcast[12];
      for (int i = 0; i < 12; i++) { input_nowcast[i] = arr[i + 12]; }
      float cmax = findMax(input_nowcast);
      float cmin = findMin(input_nowcast);
      w = (float)cmin / cmax;
      if (w <= 0.5) {
        w = 0.5;
        for (int i = 0; i < 12; i++) { nowcast = nowcast + (pow(w, i + 1) * input_nowcast[12 - i - 1]); }
      }
      if (w > 0.5) {
        for (int i = 0; i < 12; i++) {
          sum1 += input_nowcast[12 - i - 1] * pow(w, (i + 1) - 1);
          sum2 += pow(w, (i + 1) - 1);
        }
        nowcast = sum1 / sum2;
      }
      return nowcast;
    }
  // Hàm tính AQI chuẩn US EPA 2024
  float calculateAQI(float value) {
    float aqi_val = 0;
    float a, b, ni, nj;

    // 1. Tốt (0-9.0)
    if (value >= 0.0 && value <= 9.0) {
      a = 0.0; b = 9.0; ni = 0.0; nj = 50.0;
    }
    // 2. Trung bình (9.1-35.4)
    else if (value > 9.0 && value <= 35.4) {
      a = 9.0; b = 35.4; ni = 50.0; nj = 100.0;
    }
    // 3. Kém cho người nhạy cảm (35.5-55.4)
    else if (value > 35.4 && value <= 55.4) {
      a = 35.4; b = 55.4; ni = 100.0; nj = 150.0;
    }
    // 4. Kém (55.5-125.4)
    else if (value > 55.4 && value <= 125.4) {
      a = 55.4; b = 125.4; ni = 150.0; nj = 200.0;
    }
    // 5. Rất Kém (125.5-225.4)
    else if (value > 125.4 && value <= 225.4) {
      a = 125.4; b = 225.4; ni = 200.0; nj = 300.0;
    }
    // 6. Nguy hại (>225.4)
    else {
      a = 225.4; b = 500.0; ni = 300.0; nj = 500.0;
    }

    aqi_val = (((nj - ni) / (b - a)) * (value - a)) + ni;
    return aqi_val;
  }

    // =============================================================
    //  HÀM ĐIỀU KHIỂN QUẠT VÀ RELAY (Giữ nguyên)
    // =============================================================
    void applyControlLogic() {
      if (OnOff == 0) {
        ledcWrite(FAN_PWM_CHANNEL, 0);
        digitalWrite(FAN_IN1, LOW);
        digitalWrite(RELAY_PIN, LOW);
        return;
      }
      digitalWrite(FAN_IN1, HIGH);
      digitalWrite(FAN_IN2, LOW);
      if (Mode == 1) { // Auto
        if (check) { digitalWrite(RELAY_PIN, HIGH); } 
        else { digitalWrite(RELAY_PIN, LOW); }
        if (PM25 >= 36 || PM10 >= 155 || TVOC >= 661) { ledcWrite(FAN_PWM_CHANNEL, 240);
        } else if ((PM25 >= 13 && PM25 <= 35) || (PM10 >= 55 && PM10 <= 154) || (TVOC >= 221 && TVOC <= 660)) { ledcWrite(FAN_PWM_CHANNEL, 150);
        } else { ledcWrite(FAN_PWM_CHANNEL, 110); }
      } else if (Mode == 2) { // Normal
        if (Speed == 0) { ledcWrite(FAN_PWM_CHANNEL, 0);
        } else if (Speed == 1) { ledcWrite(FAN_PWM_CHANNEL, 110);
        } else if (Speed == 2) { ledcWrite(FAN_PWM_CHANNEL, 150);
        } else if (Speed == 3) { ledcWrite(FAN_PWM_CHANNEL, 240); }
        if (Ion == 0) { digitalWrite(RELAY_PIN, LOW);
        } else { digitalWrite(RELAY_PIN, HIGH); }
      }
    }

  /**
  * @brief (PHIÊN BẢN NÂNG CẤP)
  * Kích hoạt còi cảnh báo (Pin 14) VÀ hỗ trợ Tắt Còi Tạm Thời (Snooze)
  */
  void checkAirQualityWarning() {
    // KIỂM TRA SNOOZE (MỚI)
    // Nếu đang trong thời gian "Snooze" (tắt còi)
    if (warningSnoozeUntil > 0) {
      if (millis() < warningSnoozeUntil) {
        // Vẫn còn thời gian snooze
        digitalWrite(BUZZER_PIN, LOW); // Đảm bảo còi tắt
        return; // Bỏ qua, không làm gì cả
      } else {
        // Hết thời gian snooze
        warningSnoozeUntil = 0; // Đặt lại cờ
        Serial.println("Thoi gian tat coi da het. Bat lai canh bao.");
      }
    }

    // 1. Định nghĩa ngưỡng "Xấu" (Đã thêm CO2)
    bool level_high = (PM25 >= 55 || PM10 >= 254 || TVOC >= 2200 || CO2 >= 5000);

    // 2. Nếu không khí TỐT, tắt còi và thoát
    if (!level_high) {
      digitalWrite(BUZZER_PIN, LOW); // Tắt còi 14
      return;
    }
    
    // 3. Nếu không khí XẤU (và không bị snooze), xử lý logic kêu bíp
    
    // Đã đến lúc bắt đầu một chu kỳ bíp mới (5 giây)
    if (millis() - lastWarningBeep > 1000) {
      lastWarningBeep = millis(); // Đặt lại mốc thời gian
      digitalWrite(BUZZER_PIN, HIGH); // Bật còi 14
    }
    
    // Đã hết thời gian bíp (200ms), tắt còi đi
    if (millis() - lastWarningBeep > 200) {
      digitalWrite(BUZZER_PIN, LOW); // Tắt còi 14
    }
  }
    // =============================================================
    //  HÀM GIAO TIẾP HMI (Giữ nguyên)
    // =============================================================
    void updateHMIValues() {
      // Dùng Serial1 cho HMI (thay vì Serial2)
      int pm25_int = PM25;
      Serial1.print("pm25V.val=");
      Serial1.print(pm25_int); // Gửi số nguyên
      Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);

      Serial1.print("tvocV.val=");
      Serial1.print(TVOC);
      Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);

      Serial1.print("co2V.val=");
      Serial1.print(CO2);
      Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);

      int TempV = Temp;
      Serial1.print("tempV.val=");
      Serial1.print(TempV);
      Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);

      int humV = Humid;
      Serial1.print("humV.val=");
      Serial1.print(humV);
      Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);

      // Thêm cập nhật AQI nếu bạn có ô hiển thị
      Serial1.print("aqiV.val="); // Giả sử tên ô là 'aqiV'
      Serial1.print(AQI_h);
      Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);
    }

    // Cập nhật giao diện (trang, nút) trên HMI
    void updateHMIInterface() {
      // Cập nhật trang và nút bấm dựa trên OnOff
      if (OnOff != ssOnOff) {
        if (OnOff == 0) {
          // Chuyển về trang Logo (Tắt)
          Serial1.print("page Logo");
          Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);
        } else {
          // Bật máy, kiểm tra Mode để vào trang
          if (Mode == 1) { // Auto
            Serial1.print("page Home");
            Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);
            // Cập nhật nút Mode
            Serial1.print("bt0.val=1"); // Auto
            Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);
            Serial1.print("bt1.val=0"); // Normal
            Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);
          } else if (Mode == 2) { // Normal
            Serial1.print("page Homemanual");
            Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);
            // Cập nhật nút Mode
            Serial1.print("bt0.val=0");
            Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);
            Serial1.print("bt1.val=1");
            Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);
            // Cập nhật Ion và Speed cho trang Normal
            Serial1.print("bt3.val=" + String(Ion)); // Nút Ion
            Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);
            int speedVal = 25 * (Speed + 1); // 25, 50, 75, 100
            if (Speed == 0) speedVal = 0; // Hoặc 25 nếu mức 0 là 25%
            Serial1.print("h0.val=" + String(speedVal)); // Thanh trượt Speed
            Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);
          }
        }
        ssOnOff = OnOff; // Lưu trạng thái
      }

      // Cập nhật thay đổi Mode khi máy đang Bật
      if (OnOff == 1 && Mode != ssMode) {
        if (Mode == 1) { // Chuyển sang Auto
          Serial1.print("page Home");
          Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);
          Serial1.print("bt0.val=1");
          Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);
          Serial1.print("bt1.val=0");
          Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);
        } else if (Mode == 2) { // Chuyển sang Normal
          Serial1.print("page Homemanual");
          Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);
          Serial1.print("bt0.val=0");
          Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);
          Serial1.print("bt1.val=1");
          Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);
          // Cập nhật lại trạng thái Ion/Speed
          Serial1.print("bt3.val=" + String(Ion)); // Nút Ion
          Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);
          int speedVal = 25 * (Speed + 1); 
          if (Speed == 0) speedVal = 0;
          Serial1.print("h0.val=" + String(speedVal)); // Thanh trượt Speed
          Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);
        }
        ssMode = Mode; // Lưu trạng thái
      }

      // Cập nhật thay đổi Ion/Speed khi ở trang Normal
      if (OnOff == 1 && Mode == 2) {
        if (Ion != ssIon) {
          Serial1.print("bt3.val=" + String(Ion)); // Nút Ion
          Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);
          ssIon = Ion;
        }
        if (Speed != ssSpeed) {
          int speedVal = 25 * (Speed + 1);
          if (Speed == 0) speedVal = 0;
          Serial1.print("h0.val=" + String(speedVal)); // Thanh trượt Speed
          Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);
          Serial1.print("n0.val=" + String(Speed)); // Speed là 0, 1, 2, 3
          Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);
          ssSpeed = Speed;
        }
      }
    }

    // Đọc lệnh từ HMI (thay thế DieuKhienHMI)
    void readHMI() {
      // Dùng Serial1
      while (Serial1.available()) {
        byte value = Serial1.read();
       
        // Thay vì gọi DataJson, chúng ta cập nhật biến toàn cục
        // và đặt cờ (flag) để đẩy lên Firebase
        
        switch (value) {
          // --- Nút Chế độ ---
          case 0x10: // Mode Auto
            Mode = 1;
            fbUpdateMode = true;
            beepFeedback();
            break;
          case 0x11: // Mode Normal
            Mode = 2;
            fbUpdateMode = true;
            beepFeedback();
            break;
          // Đã bỏ case 0x12 (Mode Sleep)

          // --- Nút Nguồn ---
          case 0x13: // Nút Bật (từ trang Logo)
            OnOff = 1;
            fbUpdateOnOff = true;
            beepFeedback();
            break;
          case 0x14: // Nút Tắt (từ trang Home)
            OnOff = 0;
            fbUpdateOnOff = true;
            beepFeedback();
            break;
          // Đã bỏ case 0x13, 0x14 cũ (liên quan đến Control/Lock)

          // --- Điều khiển trang Normal ---
          case 0x20: // Speed 0
            Speed = 0;
            fbUpdateSpeed = true;
            beepFeedback();
            break;
          case 0x21: // Speed 1
            Speed = 1;
            fbUpdateSpeed = true;
            beepFeedback();
            break;
          case 0x22: // Speed 2
            Speed = 2;
            fbUpdateSpeed = true;
            beepFeedback();
            break;
          case 0x23: // Speed 3
            Speed = 3;
            fbUpdateSpeed = true;
            beepFeedback();
            break;
          case 0x24: // Ion Off
            Ion = 0;
            fbUpdateIon = true;
            beepFeedback();
            break;
          case 0x25: // Ion On
            Ion = 1;
            fbUpdateIon = true;
            beepFeedback();
            break;
          // Nó sẽ chuyển Mode về 1 (Auto), hàm updateHMIInterface sẽ tự chuyển trang
          case 0x26: 
            Mode = 1; 
            fbUpdateMode = true;
            beepFeedback();
            break;
        }
        
        // Cập nhật HMI ngay lập tức để phản hồi người dùng
        updateHMIInterface();
      }
    }


    // =============================================================
    //  HÀM GIAO TIẾP FIREBASE (Chỉ gọi khi Online)
    // =============================================================
   // =============================================================
//  HÀM ĐỌC FIREBASE (ĐÃ SỬA LỖI MẤT TIẾNG BÍP)
// =============================================================
void readFirebase() {
  if (Firebase.ready() && signUp) {
    
    bool coThayDoi = false; // Cờ đánh dấu

    // 1. Đọc Mode
    if (Firebase.RTDB.getInt(&fbdo, "/Control/Mode")) {
      if (fbdo.dataType() == "int") {
        int newMode = fbdo.intData();
        if (newMode != Mode) { 
          Mode = newMode;
          coThayDoi = true; // Có thay đổi!
        }
      }
    }

    // 2. Đọc Ion
    if (Firebase.RTDB.getInt(&fbdo, "/Control/Ion")) {
      if (fbdo.dataType() == "int") {
        int newIon = fbdo.intData();
        if (newIon != Ion) {
          Ion = newIon;
          coThayDoi = true; // Có thay đổi!
        }
      }
    }

    // 3. Đọc Speed
    if (Firebase.RTDB.getInt(&fbdo, "/Control/Speed")) {
      if (fbdo.dataType() == "int") {
        int newSpeed = fbdo.intData();
        if (newSpeed != Speed) {
          Speed = newSpeed;
          coThayDoi = true; // Có thay đổi!
        }
      }
    }

    // 4. Đọc OnOff
    if (Firebase.RTDB.getInt(&fbdo, "/Control/OnOff")) {
      if (fbdo.dataType() == "int") {
        int newOnOff = fbdo.intData();
        if (newOnOff != OnOff) {
          OnOff = newOnOff;
          coThayDoi = true; // Có thay đổi!
        }
      }
    }
        
    // --- Phần đọc Hẹn Giờ (Giữ nguyên) ---
    // (Lưu ý: Hẹn giờ mình không gắn cờ 'coThayDoi' để tránh kêu bíp khi hệ thống tự update giờ)
    if (Firebase.RTDB.getInt(&fbdo, "/ClockOn/OnOff")) {
      if (fbdo.dataType() == "int") ClockOnCheck = fbdo.intData();
    }
    if (Firebase.RTDB.getInt(&fbdo, "/ClockOn/Hour")) {
      if (fbdo.dataType() == "int") HourOn = fbdo.intData();
    }
    if (Firebase.RTDB.getInt(&fbdo, "/ClockOn/Minute")) {
      if (fbdo.dataType() == "int") MinuteOn = fbdo.intData();
    }

    if (Firebase.RTDB.getInt(&fbdo, "/ClockOff/OnOff")) {
      if (fbdo.dataType() == "int") ClockOffCheck = fbdo.intData();
    }
    if (Firebase.RTDB.getInt(&fbdo, "/ClockOff/Hour")) {
      if (fbdo.dataType() == "int") HourOff = fbdo.intData();
    }
    if (Firebase.RTDB.getInt(&fbdo, "/ClockOff/Minute")) {
      if (fbdo.dataType() == "int") MinuteOff = fbdo.intData();
    }

    // ============================================================
    // ĐOẠN QUAN TRỌNG BẠN BỊ THIẾU LÀ Ở ĐÂY:
    // ============================================================
    
    // Kiểm tra: Nếu nãy giờ có bất kỳ thay đổi nào (coThayDoi == true)
    if (coThayDoi) {
       beepFeedback();       // <--- Kêu Bíp 1 cái để báo hiệu
       updateHMIInterface(); // Cập nhật màn hình ngay lập tức
       Serial.println("Da cap nhat lenh tu Firebase!"); 
    }
    
  }
}

    // Gửi dữ liệu cảm biến lên Firebase
    void sendSensorDataToFirebase() {
      if (Firebase.ready() && signUp) {
        PM25 = round(PM25 * 100.0) / 100.0; 
      PM10 = round(PM10 * 100.0) / 100.0;
        Firebase.RTDB.setFloat(&fbdo, "/Sensor/Temp", Temp);
        Firebase.RTDB.setFloat(&fbdo, "/Sensor/Humid", Humid);
        Firebase.RTDB.setInt(&fbdo, "/Sensor/CO2", CO2);
        Firebase.RTDB.setInt(&fbdo, "/Sensor/TVOC", TVOC);
        Firebase.RTDB.setFloat(&fbdo, "/Sensor/PM25", PM25);
        Firebase.RTDB.setFloat(&fbdo, "/Sensor/PM10", PM10);
        Firebase.RTDB.setInt(&fbdo, "/Sensor/AQI_h", AQI_h);
        // Đã bỏ PM1
      }
    }

    // Gửi dữ liệu điều khiển (từ HMI) lên Firebase
    void sendControlDataToFirebase() {
      if (Firebase.ready() && signUp) {
        if (fbUpdateMode) {
          Firebase.RTDB.setInt(&fbdo, "/Control/Mode", Mode);
          fbUpdateMode = false;
        }
        if (fbUpdateIon) {
          Firebase.RTDB.setInt(&fbdo, "/Control/Ion", Ion);
          fbUpdateIon = false;
        }
        if (fbUpdateSpeed) {
          Firebase.RTDB.setInt(&fbdo, "/Control/Speed", Speed);
          fbUpdateSpeed = false;
        }
        if (fbUpdateOnOff) {
          Firebase.RTDB.setInt(&fbdo, "/Control/OnOff", OnOff);
          fbUpdateOnOff = false;
        }
      }
    }

    // =============================================================
    //  HÀM XỬ LÝ CẢM BIẾN VÀ THỜI GIAN
    // =============================================================

    // Đọc tất cả cảm biến (An toàn khi offline)
  // =============================================================
  //  HÀM ĐỌC CẢM BIẾN (ĐÃ THÊM BỘ LỌC LÀM MỀM 70/30)
  // =============================================================
  void readAllSensors() {
    // 1. Đọc SHT31
    float t = sht31.readTemperature();
    float h = sht31.readHumidity();
    if (!isnan(t)) Temp = t;
    if (!isnan(h)) Humid = h;

    // 2. Đọc CCS811 (Có lọc nhẹ)
    if (mySensor.dataAvailable()) {
      mySensor.readAlgorithmResults();
      int co2_raw = mySensor.getCO2();
      int tvoc_raw = mySensor.getTVOC();
      
      // Lọc nhẹ cho CO2 và TVOC nếu giá trị hợp lệ
      if (co2_raw > 0) CO2 = (CO2 * 0.7) + (co2_raw * 0.3);
      if (tvoc_raw > 0) TVOC = (TVOC * 0.7) + (tvoc_raw * 0.3);
    }

    // 3. Đọc SDS011 (QUAN TRỌNG: BỘ LỌC BỤI)
    PmResult pm = sds.readPm();
    if (pm.isOk()) {
      // Nếu là lần đầu khởi động (PM25 đang = 0), lấy luôn giá trị mới để không bị delay
      if (PM25 == 0) { 
        PM25 = pm.pm25;
        PM10 = pm.pm10;
      } else {
        // Công thức: Giá trị mới = (Cũ * 0.7) + (Mới đọc * 0.3)
        PM25 = (PM25 * 0.7) + (pm.pm25 * 0.3);
        PM10 = (PM10 * 0.7) + (pm.pm10 * 0.3);
      }
    }
  }
  /**
  * @brief Gửi cảnh báo thông minh lên HMI (Trang Manual)
  * Logic dựa trên chuẩn AQI Mới (US EPA 2024)
  */
  void updateHMIWarnings() {
    String currentMessage = "";

    // 1. Chỉ chạy khi ở chế độ Manual và máy đang Bật
    if (Mode != 2 || OnOff == 0) {
      currentMessage = "";
    } else {
      // 2. Định nghĩa mức độ (Theo chuẩn mới 2024: 9.0 và 35.4)
      bool level_high = (PM25 > 35.4 || PM10 > 154 || TVOC >= 661 || CO2 >= 1500);
      bool level_medium = ((PM25 > 12.0 && PM25 <= 35.4) || 
                          (PM10 > 54 && PM10 <= 154) || 
                          (TVOC >= 221 && TVOC < 661));

      // 3. Logic Khuyến nghị
      if (level_high) {
        // Mức Đỏ: Cần Quạt 3 + Ion
        if (Speed < 3 || Ion == 0) {
          currentMessage = "AQI Poor! Recommend: Fan Lvl 3 + Ion ON";
        } else {
          currentMessage = ""; // Đã tuân thủ -> Ẩn
        }
      } 
      else if (level_medium) {
        // Mức Vàng: Cần Quạt 2
        if (Speed < 2) {
          currentMessage = "AQI Medium. Recommend: Fan Lvl 2";
        } else {
          currentMessage = ""; // Đã tuân thủ -> Ẩn
        }
      } 
      else {
        // Mức Xanh
        currentMessage = "Air Quality: Good";
      }
    }

    // 4. Gửi lệnh ra HMI (chỉ khi tin nhắn thay đổi)
    if (currentMessage != lastWarningMessage) {
      // Giả sử tên đối tượng Marquee trên HMI là "t_warn"
      Serial1.print("t_warn.txt=\"" + currentMessage + "\"");
      Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);
      
      lastWarningMessage = currentMessage;
    }
  }
    // <-- THAY ĐỔI: HÀM HẸN GIỜ (DÙNG RTC)
    // Hàm này giờ AN TOÀN TUYỆT ĐỐI khi Offline
    void checkTimers() {
      // Luôn lấy thời gian từ RTC
      DateTime now = rtc.now();
      int currentHour = now.hour();
      int currentMinute = now.minute();
      int currentSecond = now.second();

      // Bật thiết bị
      if (ClockOnCheck == 1) {
        if (HourOn == currentHour && MinuteOn == currentMinute && currentSecond == 0) {
          if (OnOff == 0) { // Chỉ bật nếu đang tắt
            OnOff = 1;
            fbUpdateOnOff = true; // Báo để cập nhật Firebase (nếu online)
            updateHMIInterface(); // Cập nhật HMI ngay
          }
        }
      }

      // Tắt thiết bị
      if (ClockOffCheck == 1) {
        if (HourOff == currentHour && MinuteOff == currentMinute && currentSecond == 0) {
          if (OnOff == 1) { // Chỉ tắt nếu đang bật
            OnOff = 0;
            fbUpdateOnOff = true; // Báo để cập nhật Firebase (nếu online)
            updateHMIInterface(); // Cập nhật HMI ngay
          }
        }
      }
    }

    // Xử lý tính toán AQI (An toàn khi offline - Dùng RTC)
    void processAQIData() {
      DateTime now = rtc.now(); // Lấy thời gian từ RTC
      int sec = now.second();
      int minute = now.minute();

      count_sec += 1;
      sum_sec_PM25 += PM25;
      sum_sec_PM10 += PM10;

      if (pre_minute != minute) {
        pre_minute = minute;
        count_min += 1;

        sum_sec_PM25 = sum_sec_PM25 / count_sec;
        sum_sec_PM10 = sum_sec_PM10 / count_sec;
        sum_min_PM25 += sum_sec_PM25;
        sum_min_PM10 += sum_sec_PM10;
        count_sec = 0;
        sum_sec_PM25 = 0;
        sum_sec_PM10 = 0;

        if (minute == 0) { // Mỗi giờ một lần
          if (Mode == 1) {
            dem++;
            if (dem == 3) {
              dem = 0;
              check = !check;
            }
          }
          sum_min_PM25 /= count_min;
          sum_min_PM10 /= count_min;

          shiftArray(input_PM25, sum_min_PM25);
          AQI_h = calculateAQI(calculate_nowcast(input_PM25));
          shiftArray(inputAQI_h, AQI_h);
          
          count_min = 0;
          sum_min_PM25 = 0;
          sum_min_PM10 = 0;
        }
      }
    }

    // =============================================================
    //  CÁC HÀM QUẢN LÝ KẾT NỐI (MỚI)
    // =============================================================

    /**
    * @brief Đồng bộ thời gian từ NTP vào RTC.
    * Chỉ được gọi khi đang Online.
    */
    void syncRTCToNTP() {
      Serial.println("Dang dong bo RTC voi NTP...");
      if (timeClient.update()) {
        unsigned long epochTime = timeClient.getEpochTime();
        rtc.adjust(DateTime(epochTime)); // Chỉnh lại giờ cho RTC
        Serial.println("Dong bo RTC thanh cong!");
      } else {
        Serial.println("Loi dong bo NTP.");
      }
      lastRTCSync = millis();
    }

    /**
    * @brief Khởi động các dịch vụ online (Firebase, NTP)
    * Chỉ gọi hàm này MỘT LẦN sau khi đã kết nối WiFi.
    */
  void startOnlineServices() {
    Serial.println("WiFi da ket noi! Dang khoi dong dich vu online...");
    
    // 1. Khởi động NTP TRƯỚC (ĐÚNG)
    Serial.println("Dang khoi dong NTP de lay gio...");
    timeClient.begin();
    timeClient.setTimeOffset(25200); // +7 UTC
    
    // Phải đảm bảo lấy được giờ
    // Chúng ta ép nó đồng bộ ngay lập tức
    syncRTCToNTP(); 
    
    // 2. Khởi động Firebase SAU (ĐÚNG)
    // Giờ ESP32 đã có thời gian chính xác
    Serial.println("Da co gio. Dang khoi dong Firebase...");
    config.api_key = API_KEY;
    config.database_url = DTBU;
    if (Firebase.signUp(&config, &auth, "", "")) {
      signUp = true;
      Serial.println("Dang nhap Firebase thanh cong");
    } else {
      Serial.printf("Loi dang nhap Firebase: %s\n", config.signer.signupError.message.c_str());
    }
    config.token_status_callback = tokenStatusCallback;
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);

    // Gửi trạng thái hiện tại lên Firebase
    Firebase.RTDB.setInt(&fbdo, "/Control/OnOff", OnOff);
    Firebase.RTDB.setInt(&fbdo, "/Control/Mode", Mode);

    isOnline = true; // ĐẶT CỜ ONLINE
    Serial.println("He thong da Online.");
  }

    /**
    * @brief Quản lý kết nối WiFi và kích hoạt Cổng cấu hình.
    */
  /**
  * @brief (PHIÊN BẢN NÂNG CẤP)
  * Quản lý kết nối WiFi VÀ xử lý nút bấm đa chức năng (2s Tắt còi, 5s Mở Config)
  */
  void handleWifiAndServices() {
    // 1. Kiểm tra nút bấm Config (Ưu tiên cao nhất)
    // Logic mới: Kiểm tra TRONG KHI đang nhấn
    if (digitalRead(CONFIG_BUTTON_PIN) == HIGH) { // Bắt đầu nhấn nút
      unsigned long pressStartTime = millis();
      snoozeBeepDone = false; // Reset cờ
      configBeepDone = false;

      // Vòng lặp WHILE (chừng nào còn giữ nút)
      while (digitalRead(CONFIG_BUTTON_PIN) == HIGH) {
        unsigned long holdTime = millis() - pressStartTime;

        // CHỨC NĂNG 1: TẮT CÒI (Giữ 2 giây)
        if (holdTime > 2000 && !snoozeBeepDone) {
          warningSnoozeUntil = millis() + 3600000; // Tắt còi trong 1 giờ (3600000ms)
          Serial.println("Da giu 2s. Tat canh bao trong 1 gio.");
          
          // Kêu bíp 1 tiếng NGẮN để xác nhận
          digitalWrite(BUZZER_PIN, HIGH);
          delay(100); // Bíp ngắn
          digitalWrite(BUZZER_PIN, LOW);
          
          snoozeBeepDone = true; // Đã kêu bíp, không làm lại
        }

        // CHỨC NĂNG 2: MỞ CONFIG (Giữ 5 giây)
        if (holdTime > 5000 && !configBeepDone) {
          Serial.println("Da giu 5s. Chuan bi mo Config Portal...");
          
          // Kêu bíp 1 tiếng DÀI để xác nhận
          digitalWrite(BUZZER_PIN, HIGH);
          delay(300); // Bíp dài
          digitalWrite(BUZZER_PIN, LOW);
          
          configBeepDone = true; // Đã kêu bíp, không làm lại
        }
        
        delay(50); // Chờ 1 chút
      }
      // KẾT THÚC VÒNG LẶP (Người dùng đã thả nút)


      // XỬ LÝ SAU KHI THẢ NÚT
      unsigned long totalHoldTime = millis() - pressStartTime;

      // Quyết định hành động: Nếu giữ LÂU HƠN 5s -> Mở Config
      if (totalHoldTime > 5000) {
        Serial.println("Bat dau Config Portal...");
        WiFiManager wm;
        wm.setConfigPortalBlocking(true); // Chặn code cho đến khi cấu hình xong
        configPortalActive = true; 
        
        if (wm.startConfigPortal("MayLocKhongKhi-Setup")) {
          Serial.println("Cau hinh WiFi moi thanh cong! Dang khoi dong lai...");
          ESP.restart(); // Khởi động lại
        } else {
          Serial.println("Dong Config Portal.");
          configPortalActive = false;
        }
      } 
      // Nếu chỉ giữ (2s < time < 5s) -> Chức năng Tắt Còi đã được làm bên trên rồi
      
    } // Kết thúc if (digitalRead...)

    // Nếu cổng cấu hình đang chạy, không làm gì cả
    if (configPortalActive) return;

    // 2. Kiểm tra kết nối (Mỗi 10 giây) - Giữ nguyên logic này
    if (millis() - lastWifiCheck >= 10000) {
      lastWifiCheck = millis();

      if (WiFi.status() == WL_CONNECTED) {
        if (!isOnline) {
          startOnlineServices(); // Vừa mới Online
        }
      } else {
        if (isOnline) {
          Serial.println("Mat ket noi WiFi. Chuyen sang che do Offline.");
          isOnline = false; // Vừa mới Offline
        }
        Serial.println("Dang thu ket noi lai...");
        WiFi.reconnect(); // Thêm dòng này để chủ động kết nối lại
      }
    }
  }
  // Hàm kêu bíp ngắn để phản hồi thao tác
void beepFeedback() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(50); // 50ms là đủ nghe tiếng "Tí" nhẹ nhàng
  digitalWrite(BUZZER_PIN, LOW);
}
    // =============================================================
    //  SETUP (Hàm cài đặt chính ĐÃ SỬA)
    // =============================================================
    void setup() {
      Serial.begin(115200);
      Serial.println("\n\nBat dau chuong trinh ESP32 DATN (Voi WiFiManager)...");

      pinMode(CONFIG_BUTTON_PIN, INPUT_PULLDOWN);

      // Khởi động I2C
      Wire.begin(I2C_SDA, I2C_SCL);
      // Khởi động Serial HMI
      Serial1.begin(9600, SERIAL_8N1, HMI_RX, HMI_TX);
      // Khởi động Serial SDS011
      Serial2.begin(9600, SERIAL_8N1, SDS_RX, SDS_TX);
      sds.begin();

      // Khởi động Cảm biến & RTC
      if (!rtc.begin()) {
        Serial.println("Loi: Khong tim thay RTC DS3231! Chuc nang Hen gio/AQI se khong chay.");
        // Không dừng, vẫn cho chạy
      }
      if (!sht31.begin(0x44)) Serial.println("Loi SHT31!");
      if (!mySensor.begin()) Serial.println("Loi CCS811!");

      // Khởi động Chân Output & PWM
      pinMode(RELAY_PIN, OUTPUT);
      pinMode(FAN_IN1, OUTPUT);
      pinMode(FAN_IN2, OUTPUT);
      pinMode(BUZZER_PIN, OUTPUT);
      ledcSetup(FAN_PWM_CHANNEL, FAN_PWM_FREQ, FAN_PWM_RESOLUTION);
      ledcAttachPin(FAN_ENA, FAN_PWM_CHANNEL);
      digitalWrite(RELAY_PIN, LOW);
      digitalWrite(FAN_IN1, LOW);
      digitalWrite(FAN_IN2, LOW);
      ledcWrite(FAN_PWM_CHANNEL, 0);

      // Gửi lệnh khởi động HMI
      Serial1.print("page Logo");
      Serial1.write(0xff); Serial1.write(0xff); Serial1.write(0xff);
      ssOnOff = 0; // Đặt trạng thái ban đầu là Tắt

      // Khởi động WiFi (Không chặn)
      WiFi.mode(WIFI_STA);
      WiFi.begin(); // Thử kết nối với mạng đã lưu
      Serial.println("Dang thu ket noi voi WiFi da luu...");
      
      // KHÔNG khởi động Firebase/NTP ở đây
      
      // Lấy thời gian ban đầu cho AQI từ RTC
      DateTime now = rtc.now();
      pre_minute = now.minute();
      
      Serial.println("Setup hoan tat! May dang chay.");
    }

    // =============================================================
    //  LOOP (Hàm lặp chính ĐÃ SỬA)
    // =============================================================
     // =============================================================
  //  LOOP (Hàm lặp chính - ĐÃ SỬA CHUẨN)
  // =============================================================
  void loop() {
    
    // 1. Quản lý kết nối (Kiểm tra nút config & trạng thái WiFi)
    handleWifiAndServices();

    // Nếu cổng cấu hình đang mở, dừng mọi thứ khác
    if (configPortalActive) return;

    // ===================================
    // CÁC TÁC VỤ OFFLINE (Luôn luôn chạy)
    // ===================================
    
    // 2. Đọc HMI (luôn luôn để nhận lệnh nút bấm nhanh nhạy)
    readHMI();

    // 3. Đọc cảm biến & Gửi Firebase (Mỗi 2 giây)
    // <--- NOTE 1: Đây là khối lệnh quan trọng nhất
    if (millis() - lastSensorRead >= 2000) {
      lastSensorRead = millis(); // Đặt lại timer
      
      // --- Bước A: Đọc & Tính toán ---
      readAllSensors();     // Đọc SHT31, CCS811, SDS011
      processAQIData();     // Tính toán AQI (dùng RTC)
      
      // --- Bước B: Cập nhật màn hình ---
      if (OnOff == 1) {
        updateHMIValues();
      }
      updateHMIWarnings();

      // --- Bước C: Gửi Firebase (SỬA LỖI TẠI ĐÂY) ---
      
      // Đảm bảo không bị spam mạng và dữ liệu luôn mới nhất.
      if (isOnline) {
         sendSensorDataToFirebase(); 
      }
    }

    // 4. Kiểm tra Hẹn giờ (Mỗi 1 giây)
    if (millis() - lastHMITimer >= 1000) {
      lastHMITimer = millis();
      checkTimers(); // Kiểm tra hẹn giờ Bật/Tắt (đọc từ RTC)
    }

    // 5. Áp dụng logic điều khiển (luôn luôn)
    applyControlLogic();
    
    // 6. KIỂM TRA CẢNH BÁO (Luôn luôn chạy)
    checkAirQualityWarning();

    // ===================================
    // CÁC TÁC VỤ ONLINE KHÁC (Chỉ chạy khi có mạng)
    // ===================================
    
    if (isOnline) {
      
      // 7. Gửi cập nhật ĐIỀU KHIỂN từ HMI lên Firebase (nếu có bấm nút)
      if (fbUpdateMode || fbUpdateIon || fbUpdateSpeed || fbUpdateOnOff) {
        sendControlDataToFirebase();
      }

      // 8. Đọc điều khiển TỪ APP về máy (Mỗi 1 giây)
      // <--- NOTE 3: Tách riêng timer này để app phản hồi nhanh hơn
      if (millis() - lastFirebaseCheck >= 1000) {
        lastFirebaseCheck = millis();
        readFirebase();
      }

      // (Đã bỏ phần gửi Sensor ở đây vì đã đưa lên mục số 3 rồi)
        
      // 9. Đồng bộ RTC với NTP (Mỗi 1 giờ)
      if (millis() - lastRTCSync >= 3600000) { // 3600000ms = 1 giờ
        syncRTCToNTP();
      }
    }
  } // <--- Kết thúc loop() ở tận cùng này mới đúng