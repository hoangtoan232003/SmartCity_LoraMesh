#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <DHT.h>
#include <BH1750.h>

// Định nghĩa chân SPI cho LoRa SX1278
#define LORA_SCK  23   
#define LORA_MISO 19   
#define LORA_MOSI 18   
#define LORA_NSS  5    
#define LORA_RST  27   
#define LORA_DIO0 15   

// Định nghĩa chân cảm biến
#define DHTPIN 4          // Chân DHT11
#define DHTTYPE DHT11     // Loại cảm biến DHT
#define SOUND_SENSOR 13   // Cảm biến âm thanh LM393
#define NODE_ID 3  // Node 1

DHT dht(DHTPIN, DHTTYPE);
BH1750 lightMeter;

TaskHandle_t TaskReadSensor;
TaskHandle_t TaskSendLoRa;
SemaphoreHandle_t xSemaphore;  // Tạo semaphore

float nhietDo = 0.0;
float doAm = 0.0;
uint16_t anhSang = 0;
int amThanh = 0;

void ReadSensorTask(void *pvParameters) {
    while (1) {
        nhietDo = dht.readTemperature();
        doAm = dht.readHumidity();
        anhSang = lightMeter.readLightLevel();
        amThanh = digitalRead(SOUND_SENSOR);

        // Kiểm tra lỗi đọc cảm biến DHT11
        if (isnan(nhietDo) || isnan(doAm)) {
            Serial.println("[ReadSensorTask] Lỗi đọc cảm biến DHT11!");
        } else {
            Serial.print("[ReadSensorTask] Nhiệt độ: ");
            Serial.print(nhietDo);
            Serial.print("°C, Độ ẩm: ");
            Serial.print(doAm);
            Serial.print("%, Ánh sáng: ");
            Serial.print(anhSang);
            Serial.print(" lux, Âm thanh: ");
            Serial.println(amThanh);
        }

        xSemaphoreGive(xSemaphore);  // Cấp quyền cho task gửi LoRa
        Serial.println("[ReadSensorTask] Đã cấp quyền cho SendLoRaTask");

        vTaskDelay(pdMS_TO_TICKS(10000));  // Đọc lại sau 5 giây
    }
}

void SendLoRaTask(void *pvParameters) {
    while (1) {
        Serial.println("[SendLoRaTask] Đang chờ semaphore...");
        if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE) {  // Chờ tín hiệu từ ReadSensorTask
            Serial.println("[SendLoRaTask] Nhận được semaphore!");

            // Chuẩn bị dữ liệu
            String dataToSend = String(NODE_ID) + "," + String(nhietDo) + "," + String(doAm) + "," + String(anhSang) + "," + String(amThanh);


            // Gửi qua LoRa
            LoRa.beginPacket();
            LoRa.print(dataToSend);
            LoRa.endPacket();

            Serial.print("[SendLoRaTask] Gửi dữ liệu LoRa: ");
            Serial.println(dataToSend);
        }
    }
}

void setup() {
    Serial.begin(115200);
    Wire.begin(); // Khởi động I2C cho cảm biến BH1750

    // Khởi tạo cảm biến
    dht.begin();
    lightMeter.begin();
    pinMode(SOUND_SENSOR, INPUT);

    // Khởi tạo LoRa
    Serial.println("Đang khởi tạo LoRa...");
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
    LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);

    if (!LoRa.begin(433E6)) {
        Serial.println("Lỗi khởi tạo LoRa!");
        while (1);
    }

    LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);
    LoRa.setSPIFrequency(500E3);

    Serial.println("LoRa hoạt động tốt!");

    xSemaphore = xSemaphoreCreateBinary();  // Khởi tạo semaphore

  xTaskCreatePinnedToCore(ReadSensorTask, "ReadSensor", 2048, NULL, 1, &TaskReadSensor, 0);
  xTaskCreatePinnedToCore(SendLoRaTask, "SendLoRa", 2048, NULL, 1, &TaskSendLoRa, 1);

}

void loop() {}
