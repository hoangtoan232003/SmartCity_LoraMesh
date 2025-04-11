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
Mạng LoRa Mesh bao gồm nhiều node cảm biến (Node) có khả năng chuyển tiếp dữ liệu qua nhau (multi-hop) để gửi về Gateway. Gateway sẽ thu thập dữ liệu từ các node và chuyển tiếp đến MQTT Broker để xử lý tiếp theo.

#### ⚙️ Kiến trúc mạng:
```plaintext
 [Node 1] --------\
    |              \
    |               \
    |                [Node 3] ------ [Gateway] ------ [MQTT Broker]
    |               /
    |              /
 [Node 2] --------/
 ```


Node 1, 2, 3 là các node chứa cảm biến (gồm: nhiệt độ, ánh sáng, âm thanh). Có thể giao tiếp với nhau để chuyển tiếp dữ liệu.
Gateway: Thiết bị trung tâm thu thập dữ liệu từ các node và gửi đến MQTT Broker.
MQTT Broker: Nơi lưu trữ và phân phối dữ liệu đến hệ thống backend.

🔁 Đặc điểm mạng:
Giao tiếp theo kiểu Mesh, đảm bảo độ tin cậy cao và mở rộng linh hoạt.
Dữ liệu được truyền nhiều bước nếu node không nằm trong phạm vi của gateway.
Mỗi node có thể vừa là nguồn dữ liệu, vừa là trạm chuyển tiếp.

## Giao tiếp MQTT

- MQTT Broker: Mosquitto (chạy trên server hoặc public broker)
- Gateway gửi dữ liệu lên topic: `city/data/<node_id>`
- Dữ liệu dạng JSON:
```json
{
  "node_id": "node1",
  "temperature": 30.5,
  "humidity": 65,
  "light": 200,
  "sound": 35,
  "timestamp": "2025-04-11 08:00:00"
}
```

- Backend Flask subscribe và lưu dữ liệu vào MySQL

## Cơ sở dữ liệu MySQL

```sql
CREATE TABLE sensor_data (
  id INT AUTO_INCREMENT PRIMARY KEY,
  node_id VARCHAR(50),
  temperature FLOAT,
  humidity FLOAT,
  light INT,
  sound INT,
  timestamp DATETIME
);
```

## Dashboard (gợi ý)
- Flask hoặc Node.js cung cấp REST API hoặc WebSocket
- Frontend: hiển thị bảng, biểu đồ theo node, vẽ bản đồ vị trí node (nếu có GPS)

## Cài đặt nhanh

### MQTT Broker (Mosquitto)
```bash
sudo apt update
sudo apt install mosquitto mosquitto-clients
```

### Flask Backend
```bash
pip install flask paho-mqtt mysql-connector-python
```

### Cấu hình kết nối MQTT trong Flask
```python
client.connect("mqtt_broker_address", 1883)
client.subscribe("city/data/#")
```

## Demo test MQTT
```bash
mosquitto_pub -h localhost -t city/data/test -m '{"node_id": "test", "temperature": 25}'
```

## Ghi chú thêm
- Nên có watchdog / reconnect cho Gateway
- Dữ liệu có thể mã hóa AES trước khi gửi
- Có thể mở rộng thêm điều khiển actuator qua MQTT

---

## Tác giả
- Hoàng Quốc Toàn - B21DCDT221 - Nhóm trưởng
- Đào Bá Thọ - B21DCDT217
- Tạ Quang Trường - B21DCDT026
- Vương Tuấn Minh - B21DCDT153
