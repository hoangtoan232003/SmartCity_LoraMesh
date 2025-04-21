#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// ===== CẤU HÌNH WiFi & MQTT =====
#define WIFI_SSID       "TP-Link_4E28"
#define WIFI_PASSWORD   "43624799"
#define MQTT_BROKER     "192.168.0.107"
#define MQTT_PORT       1883
#define MQTT_TOPIC_UP   "sensor/data"
#define MQTT_TOPIC_DOWN "device/+/led"

// ===== CẤU HÌNH CHÂN LoRa SX1278 =====
#define LORA_SCK   23
#define LORA_MISO  19
#define LORA_MOSI  18
#define LORA_NSS   5
#define LORA_RST   15
#define LORA_DIO0  22

// ===== FreeRTOS =====
TaskHandle_t TaskReceiveLoRa;
TaskHandle_t TaskSendMQTT;
TaskHandle_t TaskControlLED;

WiFiClient espClient;
PubSubClient client(espClient);

// ===== Biến toàn cục =====
SemaphoreHandle_t xDataSemaphore;

String receivedData = "";
String jsonToSend = "";
String lastCommand = "";
int nodeID = 0;
String status = "";
float temp = 0.0, humid = 0.0;
uint16_t lux = 0;
int sound = 0;
uint32_t packetID = 0;
uint32_t lastPacketID = 0;
int lastNodeID = -1;

// ===== KẾT NỐI WiFi =====
void connectWiFi() {
    Serial.print("[WiFi] Đang kết nối");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println("\n[WiFi] Đã kết nối!");
    Serial.print("[WiFi] IP: ");
    Serial.println(WiFi.localIP());
}

// ===== KẾT NỐI MQTT =====
void connectMQTT() {
    while (!client.connected()) {
        Serial.print("[MQTT] Kết nối...");
        if (client.connect("ESP32_Gateway")) {
            Serial.println("Thành công!");
            client.subscribe(MQTT_TOPIC_DOWN);
            Serial.println("[MQTT] Đã subscribe topic: device/+/led");
        } else {
            Serial.print("Lỗi: ");
            Serial.println(client.state());
            delay(2000);
        }
    }
}

// ===== CALLBACK MQTT =====
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.println("[MQTT] Có bản tin mới từ Frontend!");

    String payloadStr;
    for (unsigned int i = 0; i < length; i++) {
        payloadStr += (char)payload[i];
    }

    Serial.println("[MQTT] Payload: " + payloadStr);

    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, payloadStr);
    if (error) {
        Serial.println("[MQTT] Lỗi parse JSON!");
        return;
    }

    int targetID = doc["device_id"];
    String ledStatus = doc["status"];  // "ON" hoặc "OFF"

    // Lưu lệnh điều khiển để gửi xuống node
    lastCommand = "LED," + String(targetID) + "," + ledStatus;

    // Gửi ngay lệnh điều khiển
    LoRa.beginPacket();
    LoRa.print(lastCommand);
    LoRa.endPacket();
    Serial.println("[LoRa] Đã gửi lệnh điều khiển LED: " + lastCommand);
}

// ===== TASK: Gửi lại lệnh điều khiển LED định kỳ =====
void ControlLEDTask(void *pvParameters) {
    while (1) {
        if (lastCommand != "") {
            LoRa.beginPacket();
            LoRa.print(lastCommand);
            LoRa.endPacket();
            Serial.println("[LoRa] Đã gửi lại lệnh điều khiển LED: " + lastCommand);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));  // Mỗi 1 giây
    }
}

// ===== TASK: Nhận dữ liệu từ LoRa =====
void ReceiveLoRaTask(void *pvParameters) {
    while (1) {
        int packetSize = LoRa.parsePacket();
        if (packetSize) {
            Serial.println("[LoRa] Gói tin mới nhận!");

            receivedData = "";
            while (LoRa.available()) {
                receivedData += (char)LoRa.read();
            }

            Serial.println("[LoRa] Dữ liệu nhận: " + receivedData);

            // Tách chuỗi
            int idx1 = receivedData.indexOf(",");
            int idx2 = receivedData.indexOf(",", idx1 + 1);
            int idx3 = receivedData.indexOf(",", idx2 + 1);
            int idx4 = receivedData.indexOf(",", idx3 + 1);
            int idx5 = receivedData.indexOf(",", idx4 + 1);
            int idx6 = receivedData.indexOf(",", idx5 + 1);
            int idx7 = receivedData.indexOf(",", idx6 + 1);

            if (idx7 == -1) {
                Serial.println("[LoRa] Lỗi định dạng gói tin!");
                vTaskDelay(pdMS_TO_TICKS(100));
                continue;
            }

            packetID = receivedData.substring(0, idx1).toInt();
            nodeID = receivedData.substring(idx1 + 1, idx2).toInt();
            status = receivedData.substring(idx2 + 1, idx3);
            temp = receivedData.substring(idx3 + 1, idx4).toFloat();
            humid = receivedData.substring(idx4 + 1, idx5).toFloat();
            lux = receivedData.substring(idx5 + 1, idx6).toInt();
            sound = receivedData.substring(idx6 + 1, idx7).toInt();
            int ttl = receivedData.substring(idx7 + 1).toInt();

            // Tránh trùng lặp
            if (packetID == lastPacketID && nodeID == lastNodeID) {
                Serial.println("[LoRa] Gói tin trùng. Bỏ qua.");
                vTaskDelay(pdMS_TO_TICKS(100));
                continue;
            }
            if (lastCommand.startsWith("LED," + String(nodeID) + ",")) {
                String expectedStatus = lastCommand.substring(lastCommand.lastIndexOf(",") + 1); // "ON" hoặc "OFF"
                if (status == expectedStatus) {
                    Serial.println("[LoRa] Node " + String(nodeID) + " đã thực thi lệnh: " + expectedStatus);
                    lastCommand = "";  // Xóa lệnh để không gửi lại nữa
                }
            }
            lastPacketID = packetID;
            lastNodeID = nodeID;

            // Nếu status là ACK => xóa lệnh


            // Tạo chuỗi JSON để gửi lên MQTT
            jsonToSend = "{";
            jsonToSend += "\"device_id\": " + String(nodeID) + ",";
            jsonToSend += "\"status\": \"" + status + "\",";
            jsonToSend += "\"temperature\": " + String(temp, 2) + ",";
            jsonToSend += "\"humidity\": " + String(humid, 2) + ",";
            jsonToSend += "\"light_intensity\": " + String(lux) + ",";
            jsonToSend += "\"noise_level\": " + String(sound) + ",";
            jsonToSend += "\"count\": " + String(packetID);
            jsonToSend += "}";

            Serial.println("[LoRa] Dữ liệu đã sẵn sàng gửi MQTT");

            // Gửi tín hiệu cho task gửi
            xSemaphoreGive(xDataSemaphore);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ===== TASK: Gửi dữ liệu lên MQTT =====
void SendMQTTTask(void *pvParameters) {
    while (1) {
        if (xSemaphoreTake(xDataSemaphore, portMAX_DELAY) == pdTRUE) {
            if (!client.connected()) {
                connectMQTT();
            }

            if (client.publish(MQTT_TOPIC_UP, jsonToSend.c_str())) {
                Serial.println("[MQTT] Gửi thành công: " + jsonToSend);
            } else {
                Serial.println("[MQTT] Gửi thất bại!");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ===== SETUP =====
void setup() {
    Serial.begin(115200);
    connectWiFi();
    client.setServer(MQTT_BROKER, MQTT_PORT);
    client.setCallback(mqttCallback);

    // Khởi tạo LoRa
    Serial.println("[LoRa] Đang khởi tạo...");
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
    LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);
    if (!LoRa.begin(433E6)) {
        Serial.println("[LoRa] Lỗi khởi tạo!");
        while (1);
    }
    Serial.println("[LoRa] Khởi tạo thành công!");

    // Semaphore
    xDataSemaphore = xSemaphoreCreateBinary();

    // Task FreeRTOS
    xTaskCreate(ReceiveLoRaTask, "ReceiveLoRaTask", 4096, NULL, 1, &TaskReceiveLoRa);
    xTaskCreate(SendMQTTTask, "SendMQTTTask", 4096, NULL, 1, &TaskSendMQTT);
    xTaskCreate(ControlLEDTask, "ControlLEDTask", 4096, NULL, 1, &TaskControlLED);
}

// ===== LOOP =====
void loop() {
    client.loop();  // Duy trì kết nối MQTT
}
