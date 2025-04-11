# He_Thong_Nhung
# Đồ Án: Hệ thống đô thị thông minh sử dụng LoRa Mesh

## Mô tả dự án
Hệ thống giám sát và thu thập dữ liệu môi trường trong khu đô thị thông minh, sử dụng mạng cảm biến không dây LoRa Mesh để truyền dữ liệu từ các node cảm biến đến Gateway. Gateway sẽ gửi dữ liệu lên backend thông qua MQTT, backend lưu vào cơ sở dữ liệu MySQL và cung cấp API phục vụ giao diện dashboard.

## Cấu tạo phần cứng

| Thành phần      | Mô tả                                |
|----------------|----------------------------------------|
| Node cảm biến  | ESP32 + Cảm biến DHT11, BH1750, âm thanh |
| Gateway        | ESP32 có LoRa, kết nối WiFi            |
| Backend        | Server Flask, kết nối MQTT broker       |
| CSDL           | MySQL                                  |

## Giao tiếp LoRa Mesh

### 1. Kết nối phần cứng LoRa SPI

| ESP32 | LoRa SX1278 |
|-------|--------------|
| 18    | SCK          |
| 19    | MISO         |
| 23    | MOSI         |
| 5     | NSS (CS)     |
| 14    | RESET        |
| 2     | DIO0         |

### 2. Mô hình mạng Mesh
Các node cảm biến gửi dữ liệu đến Gateway theo dạng truyền tin mesh (ví dụ dùng thư viện RadioHead RHMesh).

## Giao tiếp MQTT

- MQTT Broker: Mosquitto (chạy trên server hoặc public broker)
- Gateway gửi dữ liệu lên topic: `city/data/<node_id>`
- Dữ liệu dạng JSON:

```jB21DCDT153
