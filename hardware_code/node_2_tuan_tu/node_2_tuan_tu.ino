#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <DHT.h>
#include <BH1750.h>

// =================== Cấu hình phần cứng ====================
#define LORA_SCK     23
#define LORA_MISO    19
#define LORA_MOSI    18
#define LORA_NSS     5
#define LORA_RST     27
#define LORA_DIO0    15

#define DHTPIN       4
#define DHTTYPE      DHT11
#define SOUND_SENSOR 13
#define LED          2

#define NODE_ID      2         // ID node hiện tại
#define MAX_TTL      3
#define MAX_NODE     10        // Tổng số node trong mạng

// =================== Biến toàn cục ====================
DHT dht(DHTPIN, DHTTYPE);
BH1750 lightMeter;

TaskHandle_t TaskReadSensor;
TaskHandle_t TaskSendLoRa;
TaskHandle_t TaskReceiveLoRa;

SemaphoreHandle_t xSensorDataReady;
SemaphoreHandle_t xLoRaMutex;

float nhietDo = 0.0;
float doAm = 0.0;
uint16_t anhSang = 0;
int amThanh = 0;

uint32_t packetID = 1;

struct PacketRecord {
  uint8_t senderID;
  uint32_t lastPacketID;
};
PacketRecord packetHistory[MAX_NODE] = {0};

bool isDuplicate(uint8_t senderID, uint32_t pid) {
  for (int i = 0; i < MAX_NODE; i++) {
    if (packetHistory[i].senderID == senderID) {
      if (packetHistory[i].lastPacketID == pid) return true;
      packetHistory[i].lastPacketID = pid;
      return false;
    }
    if (packetHistory[i].senderID == 0) {
      packetHistory[i].senderID = senderID;
      packetHistory[i].lastPacketID = pid;
      return false;
    }
  }
  return false;
}

// =================== Task đọc cảm biến ====================
void ReadSensorTask(void *pvParameters) {
  while (1) {
    nhietDo = dht.readTemperature();
    doAm = dht.readHumidity();
    anhSang = lightMeter.readLightLevel();
    amThanh = digitalRead(SOUND_SENSOR);

    if (!isnan(nhietDo) && !isnan(doAm)) {
      Serial.printf("[Sensor] Temp: %.1f°C | Humi: %.1f%% | Lux: %d | Sound: %d\n", nhietDo, doAm, anhSang, amThanh);
      xSemaphoreGive(xSensorDataReady);
    }

    vTaskDelay(pdMS_TO_TICKS(10000));  // mỗi 10s
  }
}

// =================== Task gửi LoRa ====================
void SendLoRaTask(void *pvParameters) {
  while (1) {
    if (xSemaphoreTake(xSensorDataReady, portMAX_DELAY) == pdTRUE) {
          String ledStatus = digitalRead(LED) == HIGH ? "ON" : "OFF";
          String data = String(packetID++) + "," + String(NODE_ID) + "," + ledStatus + "," +
                    String(nhietDo) + "," + String(doAm) + "," +
                    String(anhSang) + "," + String(amThanh) + "," + String(MAX_TTL);


      if (xSemaphoreTake(xLoRaMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        LoRa.beginPacket();
        LoRa.print(data);
        LoRa.endPacket();
        xSemaphoreGive(xLoRaMutex);
        Serial.println("[LoRa SEND] " + data);
      }
    }
  }
}

// =================== Task nhận và xử lý LoRa ====================
void ReceiveLoRaTask(void *pvParameters) {
  while (1) {
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      String received = "";
      while (LoRa.available()) {
        received += (char)LoRa.read();
      }

      Serial.println("[LoRa RECV] " + received);

      // ===== Xử lý gói lệnh LED =====
      if (received.startsWith("LED")) {
        int c1 = received.indexOf(',');
        int c2 = received.indexOf(',', c1 + 1);
        int c3 = received.indexOf(',', c2 + 1);

        int targetID = received.substring(c1 + 1, c2).toInt();
        String status = received.substring(c2 + 1, c3);
        int ttl = received.substring(c3 + 1).toInt();

        if (targetID == NODE_ID) {
          if (status == "ON") {
            digitalWrite(LED, HIGH);
            Serial.println("[LED] → BẬT");
          } else {
            digitalWrite(LED, LOW);
            Serial.println("[LED] → TẮT");
          }
        } else {
          // Không phải node đích → giảm TTL & chuyển tiếp
          ttl--;
          if (ttl > 0) {
            String forward = "LED," + String(targetID) + "," + status + "," + String(ttl);
            vTaskDelay(pdMS_TO_TICKS(random(100, 300)));
            if (xSemaphoreTake(xLoRaMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
              LoRa.beginPacket();
              LoRa.print(forward);
              LoRa.endPacket();
              xSemaphoreGive(xLoRaMutex);
              Serial.println("[Forward LED] → " + forward);
            }
          }
        }

      } else {
        // ===== Gói dữ liệu cảm biến =====
        int lastComma = received.lastIndexOf(',');
        int firstComma = received.indexOf(',');
        int secondComma = received.indexOf(',', firstComma + 1);

        uint32_t recvPacketID = received.substring(0, firstComma).toInt();
        int senderID = received.substring(firstComma + 1, secondComma).toInt();
        int ttl = received.substring(lastComma + 1).toInt();

        if (ttl <= 0 || senderID == NODE_ID || isDuplicate(senderID, recvPacketID)) continue;

        String forward = received.substring(0, lastComma) + "," + String(ttl - 1);
        vTaskDelay(pdMS_TO_TICKS(random(100, 300)));
        if (xSemaphoreTake(xLoRaMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
          LoRa.beginPacket();
          LoRa.print(forward);
          LoRa.endPacket();
          xSemaphoreGive(xLoRaMutex);
          Serial.println("[Forward Sensor] → " + forward);
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// =================== Setup ====================
void setup() {
  Serial.begin(115200);
  Wire.begin();
  dht.begin();
  lightMeter.begin();
  pinMode(LED, OUTPUT);
  pinMode(SOUND_SENSOR, INPUT);
  digitalWrite(LED, LOW);

  // Khởi tạo LoRa
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("❌ Lỗi khởi động LoRa!");
    while (1);
  }
  LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);
  LoRa.setSPIFrequency(500E3);
  Serial.println("✅ LoRa đã sẵn sàng");

  // Semaphore & Mutex
  xSensorDataReady = xSemaphoreCreateBinary();
  xLoRaMutex = xSemaphoreCreateMutex();

  // Tạo task
  xTaskCreatePinnedToCore(ReadSensorTask, "ReadSensor", 4096, NULL, 1, &TaskReadSensor, 0);
  xTaskCreatePinnedToCore(SendLoRaTask, "SendLoRa", 4096, NULL, 1, &TaskSendLoRa, 0);
  xTaskCreatePinnedToCore(ReceiveLoRaTask, "ReceiveLoRa", 8192, NULL, 1, &TaskReceiveLoRa, 1);
}

// =================== Loop trống ====================
void loop() {}
