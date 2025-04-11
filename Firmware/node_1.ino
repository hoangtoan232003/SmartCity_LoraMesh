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

#define NODE_ID 1         // ID của Node (có thể thay đổi thành 2 hoặc 3)
#define GATEWAY_ID 4      // ID của Gateway
#define RETRY_NODE 2      // Node trung gian thử gửi nếu gửi trực tiếp thất bại

DHT dht(DHTPIN, DHTTYPE);
BH1750 lightMeter;

TaskHandle_t TaskReadSensor;
TaskHandle_t TaskSendLoRa;
SemaphoreHandle_t xSemaphore;

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

        if (isnan(nhietDo) || isnan(doAm)) {
            Serial.println("[ReadSensorTask] Lỗi đọc cảm biến DHT11!");
        } else {
            Serial.printf("[ReadSensorTask] Dữ liệu: %.1f°C, %.1f%%, %d lux, Âm thanh: %d\n", nhietDo, doAm, anhSang, amThanh);
        }

        xSemaphoreGive(xSemaphore);
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

bool sendLoRaMessage(int targetID, String message) {
    String dataToSend = String(NODE_ID) + "," + message;
    LoRa.beginPacket();
    LoRa.print(targetID);
    LoRa.print(",");
    LoRa.print(dataToSend);
    LoRa.endPacket();
    Serial.printf("[SendLoRaTask] Gửi đến %d: %s\n", targetID, dataToSend.c_str());
    return true;
}

void SendLoRaTask(void *pvParameters) {
    while (1) {
        if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE) {
            String message = String(nhietDo) + "," + String(doAm) + "," + String(anhSang) + "," + String(amThanh);
            if (!sendLoRaMessage(GATEWAY_ID, message)) {
                Serial.println("[SendLoRaTask] Gửi trực tiếp thất bại, thử gửi qua node trung gian...");
                sendLoRaMessage(RETRY_NODE, message);
            }
        }
    }
}

void setup() {
    Serial.begin(115200);
    Wire.begin();
    dht.begin();
    lightMeter.begin();
    pinMode(SOUND_SENSOR, INPUT);

    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
    LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);
    if (!LoRa.begin(433E6)) {
        Serial.println("Lỗi khởi tạo LoRa!");
        while (1);
    }
    LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);
    LoRa.setSPIFrequency(500E3);
    Serial.println("LoRa hoạt động tốt!");

    xSemaphore = xSemaphoreCreateBinary();
    xTaskCreatePinnedToCore(ReadSensorTask, "ReadSensor", 2048, NULL, 1, &TaskReadSensor, 0);
    xTaskCreatePinnedToCore(SendLoRaTask, "SendLoRa", 2048, NULL, 1, &TaskSendLoRa, 1);
}

void loop() {}
