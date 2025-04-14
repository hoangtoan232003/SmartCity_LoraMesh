#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <DHT.h>
#include <BH1750.h>

#define LORA_SCK  23
#define LORA_MISO 19
#define LORA_MOSI 18
#define LORA_NSS  5
#define LORA_RST  27
#define LORA_DIO0 15

#define DHTPIN 4
#define DHTTYPE DHT11
#define SOUND_SENSOR 13
#define NODE_ID 3
#define GATEWAY_ID 4
#define MAX_TTL 3

#define LED 2

DHT dht(DHTPIN, DHTTYPE);
BH1750 lightMeter;
TaskHandle_t TaskReadSensor;
TaskHandle_t TaskSendLoRa;
SemaphoreHandle_t xSemaphore;

float nhietDo = 0.0;
float doAm = 0.0;
uint16_t anhSang = 0;
int amThanh = 0;
uint32_t lastPacketID = 0;
uint32_t currentPacketID = 1;  // Tạo packetID tuần tự

void ReadSensorTask(void *pvParameters) {
    while (1) {
        nhietDo = dht.readTemperature();
        doAm = dht.readHumidity();
        anhSang = lightMeter.readLightLevel();
        amThanh = digitalRead(SOUND_SENSOR);

        if (anhSang < 30) {
            digitalWrite(LED, HIGH);
        } else {
            digitalWrite(LED, LOW);
        }

        if (!isnan(nhietDo) && !isnan(doAm)) {
            Serial.printf("[ReadSensorTask] Nhiệt độ: %.2f°C, Độ ẩm: %.2f%%, Ánh sáng: %d lux, Âm thanh: %d\n",
                          nhietDo, doAm, anhSang, amThanh);
            xSemaphoreGive(xSemaphore);
        }

        vTaskDelay(pdMS_TO_TICKS(10000));  // Đọc cảm biến mỗi 10 giây
    }
}

void SendLoRaTask(void *pvParameters) {
    while (1) {
        if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE) {
            uint32_t packetID = currentPacketID++;  // Tăng tuần tự
            String status = "ON";
            String dataToSend = String(packetID) + "," + String(NODE_ID) + "," + status + "," +
                                String(nhietDo) + "," + String(doAm) + "," + String(anhSang) + "," +
                                String(amThanh) + "," + String(MAX_TTL);

            LoRa.beginPacket();
            LoRa.print(dataToSend);
            LoRa.endPacket();
            Serial.println("[SendLoRaTask] Đã gửi: " + dataToSend);
        }
    }
}

void ReceiveLoRaTask(void *pvParameters) {
    while (1) {
        int packetSize = LoRa.parsePacket();
        if (packetSize) {
            String receivedData = "";
            while (LoRa.available()) {
                receivedData += (char)LoRa.read();
            }
            Serial.println("[ReceiveLoRaTask] Nhận: " + receivedData);

            int firstComma = receivedData.indexOf(',');
            int secondComma = receivedData.indexOf(',', firstComma + 1);
            int lastComma = receivedData.lastIndexOf(',');

            uint32_t packetID = receivedData.substring(0, firstComma).toInt();
            int senderID = receivedData.substring(firstComma + 1, secondComma).toInt();
            int ttl = receivedData.substring(lastComma + 1).toInt();

            if (packetID == lastPacketID || senderID == NODE_ID || ttl <= 0) {
                return;  // Bỏ qua nếu trùng packetID, là node mình, hoặc TTL <= 0
            }

            lastPacketID = packetID;
            ttl--;  // Giảm TTL trước khi chuyển tiếp
            String newData = receivedData.substring(0, lastComma) + "," + String(ttl);

            vTaskDelay(pdMS_TO_TICKS(random(100, 500)));  // Delay ngẫu nhiên
            LoRa.beginPacket();
            LoRa.print(newData);
            LoRa.endPacket();
            Serial.println("[ReceiveLoRaTask] Chuyển tiếp: " + newData);
        }

        vTaskDelay(pdMS_TO_TICKS(100));  // Kiểm tra dữ liệu LoRa mỗi 100ms
    }
}

void setup() {
    Serial.begin(115200);
    Wire.begin();
    dht.begin();
    lightMeter.begin();
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);
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
    xTaskCreatePinnedToCore(ReceiveLoRaTask, "ReceiveLoRa", 2048, NULL, 1, NULL, 1);
}

void loop() {
    // Không làm gì ở loop vì dùng FreeRTOS task
}
