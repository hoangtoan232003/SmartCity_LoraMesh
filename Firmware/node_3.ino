# Import các thư viện cần thiết
#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <DHT.h>
#include <BH1750.h>

// Chân kết nối SX1278 (Module LoRa)
#define LORA_SCK  23   // Chân SCK của LoRa
#define LORA_MISO 19   // Chân MISO của LoRa
#define LORA_MOSI 18   // Chân MOSI của LoRa
#define LORA_NSS  5    // Chân NSS của LoRa
#define LORA_RST  27   // Chân RESET của LoRa
#define LORA_DIO0 15   // Chân DIO0 của LoRa

// Cảm biến DHT và âm thanh
#define DHTPIN 4          // Chân dữ liệu của DHT11
#define DHTTYPE DHT11     // Loại cảm biến DHT sử dụng
#define SOUND_SENSOR 13   // Chân cảm biến âm thanh

// Định danh node
#define NODE_ID 3         // ID của node hiện tại
#define GATEWAY_ID 99     // ID của Gateway
#define RELAY_NODE 2      // ID của node trung gian
#define MAX_RETRIES 3     // Số lần thử gửi lại khi thất bại
#define ACK_TIMEOUT 3000  // Thời gian chờ phản hồi ACK (ms)

// Khai báo cảm biến
DHT dht(DHTPIN, DHTTYPE);
BH1750 lightMeter;

// Khai báo các task trong FreeRTOS
TaskHandle_t TaskReadSensor;
TaskHandle_t TaskSendLoRa;
SemaphoreHandle_t xSemaphore;

// Biến lưu trữ giá trị cảm biến
float nhietDo = 0.0;
float doAm = 0.0;
uint16_t anhSang = 0;
int amThanh = 0;

// Task đọc dữ liệu cảm biến
void ReadSensorTask(void *pvParameters) {
    while (1) {
        nhietDo = dht.readTemperature();  // Đọc nhiệt độ từ cảm biến DHT
        doAm = dht.readHumidity();        // Đọc độ ẩm từ cảm biến DHT
        anhSang = lightMeter.readLightLevel(); // Đọc cường độ ánh sáng từ BH1750
        amThanh = digitalRead(SOUND_SENSOR);   // Đọc tín hiệu âm thanh

        if (!isnan(nhietDo) && !isnan(doAm)) { // Kiểm tra nếu dữ liệu hợp lệ
            Serial.printf("[ReadSensorTask] Nhiệt độ: %.2f°C, Độ ẩm: %.2f%%, Ánh sáng: %d lux, Âm thanh: %d\n", nhietDo, doAm, anhSang, amThanh);
            xSemaphoreGive(xSemaphore); // Báo hiệu dữ liệu đã sẵn sàng để gửi
        }
        vTaskDelay(pdMS_TO_TICKS(10000)); // Chờ 10 giây trước khi đọc lại
    }
}

// Hàm chờ phản hồi ACK từ Gateway
bool waitForAck() {
    unsigned long startTime = millis();
    while (millis() - startTime < ACK_TIMEOUT) {
        int packetSize = LoRa.parsePacket();
        if (packetSize) {
            String receivedData = "";
            while (LoRa.available()) {
                receivedData += (char)LoRa.read();
            }
            if (receivedData == "ACK") { // Kiểm tra nếu phản hồi là ACK
                Serial.println("[waitForAck] Nhận được ACK từ Gateway!");
                return true;
            }
        }
    }
    return false;
}

// Hàm gửi dữ liệu qua LoRa
bool sendLoRaData(int targetNode, String data) {
    for (int i = 0; i < MAX_RETRIES; i++) {
        LoRa.beginPacket();
        LoRa.printf("%d,%s", targetNode, data.c_str()); // Gửi dữ liệu kèm theo ID node đích
        LoRa.endPacket();

        Serial.printf("[SendLoRaTask] Đã gửi đến %d, lần thử %d\n", targetNode, i + 1);
        if (waitForAck()) return true; // Nếu nhận được ACK thì dừng gửi lại

        vTaskDelay(pdMS_TO_TICKS(2000)); // Chờ 2 giây trước khi thử lại
    }
    return false;
}

// Task gửi dữ liệu LoRa
void SendLoRaTask(void *pvParameters) {
    while (1) {
        if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE) { // Chờ tín hiệu từ Task đọc cảm biến
            String dataToSend = String(NODE_ID) + "," + String(nhietDo) + "," + String(doAm) + "," + String(anhSang) + "," + String(amThanh);
            if (!sendLoRaData(GATEWAY_ID, dataToSend)) { // Thử gửi trực tiếp đến Gateway
                Serial.println("[SendLoRaTask] Gửi trực tiếp thất bại, thử gửi qua node trung gian...");
                sendLoRaData(RELAY_NODE, dataToSend); // Nếu thất bại, gửi qua node trung gian
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

    // Cấu hình LoRa
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
    LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);

    if (!LoRa.begin(433E6)) {
        Serial.println("Lỗi khởi tạo LoRa!");
        while (1);
    }

    LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN); // Thiết lập công suất phát
    LoRa.setSPIFrequency(500E3); // Đặt tần số SPI cho LoRa
    Serial.println("LoRa hoạt động tốt!");

    // Tạo Semaphore để đồng bộ dữ liệu giữa các task
    xSemaphore = xSemaphoreCreateBinary();

    // Tạo các task trong FreeRTOS
    xTaskCreatePinnedToCore(ReadSensorTask, "ReadSensor", 2048, NULL, 1, &TaskReadSensor, 0);
    xTaskCreatePinnedToCore(SendLoRaTask, "SendLoRa", 2048, NULL, 1, &TaskSendLoRa, 1);
}

void loop() {}
