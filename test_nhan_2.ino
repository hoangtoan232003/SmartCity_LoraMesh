#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <PubSubClient.h>

// WiFi & MQTT Broker
#define WIFI_SSID "TP-Link_4E28"
#define WIFI_PASSWORD "43624799"
#define MQTT_BROKER "192.168.11.173"  // i thành địa chỉ MQTT Broker của bạn
#define MQTT_PORT 1883
#define MQTT_TOPIC "sensor/data"

// Chân SPI LoRa SX1278
#define LORA_SCK  23   
#define LORA_MISO 19   
#define LORA_MOSI 18   
#define LORA_NSS  5    
#define LORA_RST  15   
#define LORA_DIO0 22   

TaskHandle_t TaskReceiveLoRa;
TaskHandle_t TaskSendMQTT;
SemaphoreHandle_t xSemaphore;

String receivedData = "";
int id = 0;
float nhietDo = 0.0, doAm = 0.0;
uint16_t anhSang = 0;
int amThanh = 0;

// WiFi & MQTT
WiFiClient espClient;
PubSubClient client(espClient);

void connectWiFi() {
    Serial.print("Dang ket noi WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println("\nKet noi WiFi thanh cong!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
}

void connectMQTT() {
    while (!client.connected()) {
        Serial.print("Ket noi MQTT...");
        if (client.connect("ESP32_Gateway")) {
            Serial.println("Ket noi MQTT thanh cong!");
        } else {
            Serial.print("Loi ket noi MQTT: ");
            Serial.println(client.state());
            delay(2000);
        }
    }
}

void ReceiveLoRaTask(void *pvParameters) {
    while (1) {
        int packetSize = LoRa.parsePacket();
        if (packetSize) {
            Serial.println("[ReceiveLoRaTask] Nhan duoc goi tin LoRa!");

            receivedData = "";
            while (LoRa.available()) {
                receivedData += (char)LoRa.read();
            }

            Serial.print("[ReceiveLoRaTask] Du lieu nhan duoc: ");
            Serial.println(receivedData);

            sscanf(receivedData.c_str(), "%d,%f,%f,%hu,%d", &id, &nhietDo, &doAm, &anhSang, &amThanh);

            Serial.printf("[ReceiveLoRaTask] ID: %d, Nhiet do: %.2f C, Do am: %.2f %%, Anh sang: %u lux, Am thanh: %d\n",
                          id, nhietDo, doAm, anhSang, amThanh);

            xSemaphoreGive(xSemaphore);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void SendMQTTTask(void *pvParameters) {
    while (1) {
        if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE) {
            if (!client.connected()) {
                connectMQTT();
            }

            String jsonData = "{";
            jsonData += "\"device_id\": " + String(id) + ",";
            jsonData += "\"status\": \"ON\",";
            jsonData += "\"temperature\": " + String(nhietDo, 2) + ",";
            jsonData += "\"humidity\": " + String(doAm, 2) + ",";
            jsonData += "\"light_intensity\": " + String(anhSang) + ",";
            jsonData += "\"noise_level\": " + String(amThanh);
            jsonData += "}";

            Serial.print("Gui MQTT: ");
            Serial.println(jsonData);

            if (client.publish(MQTT_TOPIC, jsonData.c_str())) {
                Serial.println("Gui du lieu thanh cong!");
            } else {
                Serial.println("Gui du lieu that bai!");
            }
        }
    }
}
 
void setup() {
    Serial.begin(115200);
    
    connectWiFi();
    client.setServer(MQTT_BROKER, MQTT_PORT);

    Serial.println("Dang khoi tao LoRa...");
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
    LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);

    if (!LoRa.begin(433E6)) {
        Serial.println("Loi khoi tao LoRa!");
        while (1);
    }

    Serial.println("LoRa hoat dong tot!");

    xSemaphore = xSemaphoreCreateBinary();

    xTaskCreatePinnedToCore(ReceiveLoRaTask, "ReceiveLoRa", 8192, NULL, 2, &TaskReceiveLoRa, 1);
    xTaskCreatePinnedToCore(SendMQTTTask, "SendMQTT", 4096, NULL, 1, &TaskSendMQTT, 0);
}

void loop() {
    client.loop();
}
