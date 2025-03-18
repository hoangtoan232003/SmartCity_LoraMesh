#include <SPI.h>
#include <LoRa.h>

// Định nghĩa chân SPI cho LoRa SX1278 (Nhận)
#define LORA_SCK  23   // Chân SCK của LoRa
#define LORA_MISO 19   // Chân MISO của LoRa
#define LORA_MOSI 18   // Chân MOSI của LoRa
#define LORA_CS   5    // Chân NSS (CS) của LoRa
#define LORA_RST  15   // Chân Reset của LoRa (đã đổi sang IO15)
#define LORA_IRQ  22   // Chân IRQ (DIO0) của LoRa (đã đổi sang IO22)

void setup() {
    Serial.begin(115200);
    Serial.println("Đang khởi tạo LoRa Receiver...");
    
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
    LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ);
    
    if (!LoRa.begin(433E6)) { // Tần số 433MHz cho SX1278
        Serial.println("LoRa init failed!");
        while (1);
    }
    
    Serial.println("LoRa Receiver Initialized!");
}

void loop() {
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
        Serial.println("\nNhận gói tin từ LoRa...");
        
        String receivedData = "";
        while (LoRa.available()) {
            receivedData += (char)LoRa.read();
        }
        
        Serial.print("Dữ liệu nhận: ");
        Serial.println(receivedData);

        // Giải mã dữ liệu từ gói tin
        int tempIndex = receivedData.indexOf(",");
        int humidIndex = receivedData.indexOf(",", tempIndex + 1);
        int luxIndex = receivedData.indexOf(",", humidIndex + 1);
        
        float temp = receivedData.substring(0, tempIndex).toFloat();
        float humid = receivedData.substring(tempIndex + 1, humidIndex).toFloat();
        int lux = receivedData.substring(humidIndex + 1, luxIndex).toInt();
        int sound = receivedData.substring(luxIndex + 1).toInt();
        
        Serial.print("Nhiệt độ: "); Serial.print(temp); Serial.println("°C");
        Serial.print("Độ ẩm: "); Serial.print(humid); Serial.println("%");
        Serial.print("Ánh sáng: "); Serial.print(lux); Serial.println(" lx");
        Serial.print("Âm thanh: "); Serial.println(sound == 0 ? "Phát hiện" : "Không có");
        
        Serial.println("-----------------------------");
    }
}
