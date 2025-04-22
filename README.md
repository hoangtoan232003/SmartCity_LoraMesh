# 🔌 Hệ Thống Đô Thị Thông Minh Sử Dụng LoRa Mesh

> Mô tả ngắn gọn: Hệ thống giám sát và thu thập dữ liệu môi trường trong khu đô thị thông minh, sử dụng mạng cảm biến không dây LoRa Mesh để truyền dữ liệu từ các node cảm biến đến Gateway. Gateway sẽ gửi dữ liệu lên backend thông qua MQTT, backend lưu vào cơ sở dữ liệu MySQL và cung cấp API phục vụ giao diện dashboard.

---

## 📑 Mục Lục

- [Giới thiệu](#giới-thiệu)
- [Cấu tạo phần cứng](#cấu-tạo-phần-cứng)
- [Giao tiếp LoRa Mesh](#giao-tiếp-lora-mesh)
- [Giao tiếp MQTT](#giao-tiếp-mqtt)
- [Cơ sở dữ liệu MySQL](#cơ-sở-dữ-liệu-mysql)
- [Cài đặt nhanh](#cài-đặt-nhanh)
- [Cài đặt MQTT Broker (Mosquitto)](#cài-đặt-mqtt-broker-mosquitto)
- [Cài đặt Flask Backend](#cài-đặt-flask-backend)
- [Cài đặt cơ sở dữ liệu MySQL](#cài-đặt-cơ-sở-dữ-liệu-mysql)
- [Ví dụ code nhận dữ liệu MQTT](#ví-dụ-code-nhận-dữ-liệu-mqtt)
- [Demo test MQTT](#demo-test-mqtt)
- [Kiểm thử](#kiểm-thử)
- [Ảnh/Video demo](#ảnhvideo-demo)
- [Đóng góp](#đóng-góp)
- [Giấy phép](#giấy-phép)

---

## 👋 Giới Thiệu

Hệ thống giám sát và thu thập dữ liệu môi trường trong khu đô thị thông minh. Dự án sử dụng mạng cảm biến không dây LoRa Mesh để truyền tải dữ liệu giữa các node cảm biến và Gateway. Dữ liệu thu thập được gửi lên server backend thông qua giao thức MQTT, sau đó được lưu vào cơ sở dữ liệu MySQL và cung cấp API cho giao diện dashboard.

---

## 📐 Cấu Tạo Phần Cứng

| Thành phần      | Mô tả                                |
|----------------|----------------------------------------|
| Node cảm biến  | ESP32 + Cảm biến DHT11, BH1750, âm thanh |
| Gateway        | ESP32 có LoRa, kết nối WiFi            |
| Backend        | Server Flask, kết nối MQTT broker       |
| CSDL           | MySQL                                  |

---

## 🔧 Giao Tiếp LoRa Mesh

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
Node 1, 2, 3 là các node chứa cảm biến (gồm: nhiệt độ, ánh sáng, âm thanh). Có thể giao tiếp với nhau để chuyển tiếp dữ liệu.
Gateway: Thiết bị trung tâm thu thập dữ liệu từ các node và gửi đến MQTT Broker.
MQTT Broker: Nơi lưu trữ và phân phối dữ liệu đến hệ thống backend.

🔁 Đặc điểm mạng:
Giao tiếp theo kiểu Mesh, đảm bảo độ tin cậy cao và mở rộng linh hoạt.
Dữ liệu được truyền nhiều bước nếu node không nằm trong phạm vi của gateway.
Mỗi node có thể vừa là nguồn dữ liệu, vừa là trạm chuyển tiếp.

📡 Giao Tiếp MQTT
MQTT Broker: Mosquitto (chạy trên server hoặc public broker)

Gateway gửi dữ liệu lên topic: city/data/<node_id>

Dữ liệu dạng JSON:

json
Sao chép
Chỉnh sửa
{
  "node_id": "node1",
  "temperature": 30.5,
  "humidity": 65,
  "light": 200,
  "sound": 35,
  "timestamp": "2025-04-11 08:00:00"
}
Backend Flask subscribe và lưu dữ liệu vào MySQL.

🗃️ Cơ Sở Dữ Liệu MySQL
sql
Sao chép
Chỉnh sửa
CREATE TABLE sensor_data (
  id INT AUTO_INCREMENT PRIMARY KEY,
  node_id VARCHAR(50),
  temperature FLOAT,
  humidity FLOAT,
  light INT,
  sound INT,
  timestamp DATETIME
);
Cài Đặt Nhanh
Cài Đặt MQTT Broker (Mosquitto)
bash
Sao chép
Chỉnh sửa
sudo apt update
sudo apt install mosquitto mosquitto-clients
Cài Đặt Flask Backend
bash
Sao chép
Chỉnh sửa
pip install flask paho-mqtt mysql-connector-python
Cài Đặt Cơ Sở Dữ Liệu MySQL
bash
Sao chép
Chỉnh sửa
sudo apt install mysql-server
sudo mysql_secure_installation
Tạo database và bảng:

sql
Sao chép
Chỉnh sửa
CREATE DATABASE iot;
USE iot;
-- Execute the sensor_data table creation script
💻 Ví Dụ Code Nhận Dữ Liệu MQTT
python
Sao chép
Chỉnh sửa
import paho.mqtt.client as mqtt
import mysql.connector
import json

def on_message(client, userdata, msg):
    data = json.loads(msg.payload)
    print(f"Received data: {data}")
    # Lưu vào MySQL
    conn = mysql.connector.connect(host="localhost", user="root", password="password", database="iot")
    cursor = conn.cursor()
    sql = "INSERT INTO sensor_data (node_id, temperature, humidity, light, sound, timestamp) VALUES (%s, %s, %s, %s, %s, NOW())"
    val = (data['node_id'], data['temperature'], data['humidity'], data['light'], data['sound'])
    cursor.execute(sql, val)
    conn.commit()
    cursor.close()
    conn.close()

client = mqtt.Client()
client.connect("localhost", 1883)
client.subscribe("city/data/#")
client.on_message = on_message
client.loop_forever()
🛠️ Demo Test MQTT
bash
Sao chép
Chỉnh sửa
mosquitto_pub -h localhost -t city/data/test -m '{"node_id": "test", "temperature": 25}'
Kiểm tra dữ liệu lưu trữ trong MySQL:

sql
Sao chép
Chỉnh sửa
SELECT * FROM sensor_data;
📸 Ảnh/Video Demo
Sẽ cập nhật sau khi hoàn thành hệ thống thực tế.

🤝 Đóng Góp
Fork repo và gửi pull request.

Góp ý cải tiến thêm chức năng mới.

Báo lỗi tại phần Issues.

📜 Giấy Phép
Dự án phát hành dưới giấy phép MIT License.
Thoải mái sử dụng cho mục đích cá nhân, giáo dục hoặc thương mại.
Yêu cầu giữ nguyên tên tác giả gốc khi phát hành lại.

👨‍💻 Tác Giả
Hoàng Quốc Toàn - B21DCDT221 - Nhóm trưởng
Đào Bá Thọ - B21DCDT217
Tạ Quang Trường - B21DCDT026
Vương Tuấn Minh - B21DCDT153
