#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ===== CẤU HÌNH WiFi & MQTT =====
#define WIFI_SSID       "TP-Link_4E28"
#define WIFI_PASSWORD   "43624799"
#define MQTT_BROKER     "192.168.240.173"
#define MQTT_PORT       1883
#define MQTT_TOPIC      "sensor/data"

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
SemaphoreHandle_t xSemaphore;

// ===== BIẾN DỮ LIỆU =====
String receivedData = "";
int nodeID = 0;
String status = "";
float temp = 0.0, humid = 0.0;
uint16_t lux = 0;
int sound = 0;
uint32_t packetID = 0;
uint32_t lastPacketID = 0;
int lastNodeID = -1;

// ===== WiFi & MQTT =====
WiFiClient espClient;
PubSubClient client(espClient);

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
        } else {
            Serial.print("Lỗi: ");
            Serial.println(client.state());
            delay(2000);
        }
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

            // Tách dữ liệu
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

            // Kiểm tra gói tin trùng
            if (packetID == lastPacketID && nodeID == lastNodeID) {
                Serial.println("[LoRa] Gói tin trùng. Bỏ qua.");
                vTaskDelay(pdMS_TO_TICKS(100));
                continue;
            }

            lastPacketID = packetID;
            lastNodeID = nodeID;

            Serial.printf("[LoRa] Node %d | Temp: %.2f°C | Humid: %.2f%% | Lux: %d | Sound: %s\n",
                          nodeID, temp, humid, lux, sound == 0 ? "Phát hiện" : "Không");

            // Cho phép gửi MQTT
            xSemaphoreGive(xSemaphore);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ===== TASK: Gửi dữ liệu lên MQTT (GỒM packetID) =====
void SendMQTTTask(void *pvParameters) {
    while (1) {
        if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE) {
            if (!client.connected()) {
                connectMQTT();
            }

            String jsonData = "{";
            jsonData += "\"device_id\": " + String(nodeID) + ",";
            jsonData += "\"packet_id\": " + String(packetID) + ",";
            jsonData += "\"status\": \"" + status + "\",";
            jsonData += "\"temperature\": " + String(temp, 2) + ",";
            jsonData += "\"humidity\": " + String(humid, 2) + ",";
            jsonData += "\"light_intensity\": " + String(lux) + ",";
            jsonData += "\"noise_level\": " + String(sound);
            jsonData += "}";

            Serial.println("[MQTT] Gửi dữ liệu: " + jsonData);

            if (client.publish(MQTT_TOPIC, jsonData.c_str())) {
                Serial.println("[MQTT] Gửi thành công!");
            } else {
                Serial.println("[MQTT] Gửi thất bại!");
            }
        }
    }
}

// ===== SETUP =====
void setup() {
    Serial.begin(115200);

    connectWiFi();
    client.setServer(MQTT_BROKER, MQTT_PORT);

    // Khởi tạo LoRa
    Serial.println("[LoRa] Đang khởi tạo...");
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
    LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);
    if (!LoRa.begin(433E6)) {
        Serial.println("[LoRa] Lỗi khởi tạo!");
        while (1);
    }
    Serial.println("[LoRa] Đã sẵn sàng!");

    // Khởi tạo Semaphore và Task
    xSemaphore = xSemaphoreCreateBinary();

    xTaskCreatePinnedToCore(ReceiveLoRaTask, "ReceiveLoRa", 8192, NULL, 2, &TaskReceiveLoRa, 1);
    xTaskCreatePinnedToCore(SendMQTTTask, "SendMQTT", 4096, NULL, 1, &TaskSendMQTT, 0);
}

// ===== LOOP =====
void loop() {
    client.loop();  // Duy trì kết nối MQTT
}
