#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <DHT.h>
#include <BH1750.h>

// Định nghĩa chân SPI cho LoRa SX1278 (Gửi)
#define LORA_SCK  23   // Chân SCK của LoRa
#define LORA_MISO 19   // Chân MISO của LoRa
#define LORA_MOSI 18   // Chân MOSI của LoRa
#define LORA_CS   5    // Chân NSS (CS) của LoRa
#define LORA_RST  27   // Chân Reset của LoRa (đã đổi sang IO27)
#define LORA_IRQ  15   // Chân IRQ (DIO0) của LoRa (đã đổi sang IO15)

// Định nghĩa chân cảm biến
#define DHTPIN 4         // Chân DHT11
#define DHTTYPE DHT11    // Loại cảm biến DHT
#define SOUND_SENSOR 13  // Chân cảm biến âm thanh (LM393, đã đổi sang IO13)
DHT dht(DHTPIN, DHTTYPE);
BH1750 lightMeter;

void setup() {
    Serial.begin(115200);
    Wire.begin();
    
    Serial.println("Đang khởi tạo LoRa...");
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
    LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ);
    
    if (!LoRa.begin(433E6)) { // Tần số 433MHz cho SX1278
        Serial.println("LoRa init failed!");
        while (1);
    }
    
    Serial.println("LoRa init success!");
    Serial.println("LoRa Sender Initialized!");
    
    dht.begin();
    lightMeter.begin();
    pinMode(SOUND_SENSOR, INPUT);
}

void loop() {
    float temp = dht.readTemperature();
    float humid = dht.readHumidity();
    uint16_t lux = lightMeter.readLightLevel();
    int sound = digitalRead(SOUND_SENSOR);

    // Kiểm tra lỗi đọc cảm biến
    if (isnan(temp) || isnan(humid)) {
        Serial.println("Lỗi đọc cảm biến DHT11!");
        return;
    }

    // Kiểm tra xem LoRa có sẵn sàng gửi dữ liệu không
    if (LoRa.beginPacket() == 0) {
        Serial.println("Lỗi khi bắt đầu gói dữ liệu LoRa!");
        return;
    }

    // Chuẩn bị dữ liệu
    String dataToSend = String(temp) + "," + String(humid) + "," + String(lux) + "," + String(sound);

    // Gửi qua LoRa
    LoRa.print(dataToSend);
    LoRa.endPacket();

    Serial.print("Gửi: ");
    Serial.println(dataToSend);
    Serial.println("------------------------");

    delay(5000); // Gửi mỗi 5 giây
}
