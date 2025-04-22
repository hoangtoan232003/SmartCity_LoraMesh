# 🔌 Hệ Thống Đô Thị Thông Minh Sử Dụng LoRa Mesh

> Hệ thống giám sát môi trường khu đô thị thông minh bằng mạng LoRa Mesh. Thu thập dữ liệu từ các node cảm biến, gửi về Gateway, chuyển tiếp lên server qua MQTT để lưu trữ và phân tích.  
> Phục vụ mục đích nghiên cứu, giáo dục và ứng dụng thực tế.

---

## 📑 Mục Lục

- [Giới thiệu](#giới-thiệu)
- [Thông số kỹ thuật](#thông-số-kỹ-thuật)
- [Danh sách linh kiện](#danh-sách-linh-kiện)
- [Sơ đồ nguyên lý và PCB](#sơ-đồ-nguyên-lý-và-pcb)
- [Hướng dẫn lắp ráp](#hướng-dẫn-lắp-ráp)
- [Lập trình firmware](#lập-trình-firmware)
- [Cách sử dụng](#cách-sử-dụng)
- [Kiểm thử](#kiểm-thử)
- [Ảnh/Video demo](#ảnhvideo-demo)
- [Đóng góp](#đóng-góp)
- [Giấy phép](#giấy-phép)

---

## 👋 Giới Thiệu

Dự án xây dựng hệ thống thu thập dữ liệu môi trường trong khu đô thị bằng mạng cảm biến không dây LoRa Mesh. Các node cảm biến đo nhiệt độ, độ ẩm, ánh sáng và âm thanh, sau đó chuyển tiếp dữ liệu về Gateway.  
Gateway gửi dữ liệu qua MQTT lên server Flask để lưu vào cơ sở dữ liệu MySQL và hiển thị qua dashboard.  
**Đối tượng sử dụng:** Sinh viên, kỹ sư nghiên cứu về IoT, đô thị thông minh.

---

## 📐 Thông Số Kỹ Thuật

| Thành phần     | Thông tin                          |
|----------------|------------------------------------|
| MCU            | ESP32-WROOM-32                     |
| Giao tiếp LoRa | SX1278 (SPI)                       |
| Cảm biến       | DHT11 (nhiệt độ, độ ẩm), BH1750 (ánh sáng), microphone (âm thanh) |
| Gateway        | ESP32 kết nối WiFi và LoRa          |
| Giao tiếp Server | MQTT Broker (Mosquitto), Flask Backend |
| CSDL           | MySQL                               |

---

## 🧰 Danh Sách Linh Kiện

| Tên linh kiện           | Số lượng | Ghi chú                     |
|--------------------------|----------|-----------------------------|
| ESP32 DevKit v1           | 4        | 3 node + 1 gateway          |
| Module LoRa SX1278        | 4        | 3 node + 1 gateway          |
| Cảm biến DHT11            | 3        | Đo nhiệt độ, độ ẩm          |
| Cảm biến ánh sáng BH1750  | 3        | Đo cường độ ánh sáng        |
| Microphone analog         | 3        | Đo mức âm thanh môi trường  |
| Nguồn 5V (hoặc pin Li-ion) | 4        | Cho ESP32 và LoRa Module    |

---

## 🔧 Sơ Đồ Nguyên Lý và PCB

- 📎 [Schematic (PDF)](docs/schematic.pdf) *(cập nhật sau)*
- 📎 [PCB Layout (Gerber)](docs/gerber.zip) *(cập nhật sau)*
- 📎 [File thiết kế (KiCad/Eagle)](docs/project.kicad_pcb) *(cập nhật sau)*

### Kết nối phần cứng LoRa SPI:

| ESP32 | LoRa SX1278 |
|-------|-------------|
| 18    | SCK         |
| 19    | MISO        |
| 23    | MOSI        |
| 5     | NSS (CS)    |
| 14    | RESET       |
| 2     | DIO0        |

---

## 🔩 Hướng Dẫn Lắp Ráp

1. Kết nối ESP32 với module LoRa SX1278 theo sơ đồ trên.
2. Gắn cảm biến DHT11, BH1750 và microphone vào ESP32 node.
3. Hàn chắc các chân, kiểm tra ngắn mạch.
4. Cấp nguồn 5V hoặc dùng pin.
5. Nạp firmware cho các node và Gateway.

---

## 💻 Lập Trình Firmware

- **Ngôn ngữ:** C++ (Arduino IDE)
- **Firmware node:** Thu thập dữ liệu, gửi qua LoRa
- **Firmware gateway:** Nhận dữ liệu LoRa, gửi lên MQTT Broker
- **Cách nạp firmware:**
  ```bash
  pio run --target upload
Cách Sử Dụng
Cài đặt MQTT Broker (Mosquitto):
sudo apt update
sudo apt install mosquitto mosquitto-clients
Chạy Flask Backend:
bash
Sao chép
Chỉnh sửa
pip install flask paho-mqtt mysql-connector-python
Ví dụ code nhận dữ liệu MQTT bằng Python:

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
Topic dữ liệu: city/data/<node_id>

Dữ liệu mẫu:

json
Sao chép
Chỉnh sửa
{
  "node_id": "node1",
  "temperature": 30.5,
  "humidity": 65,
  "light": 200,
  "sound": 35
}
🛠️ Kiểm Thử
Test MQTT Gateway:

bash
Sao chép
Chỉnh sửa
mosquitto_pub -h localhost -t city/data/test -m '{"node_id": "test", "temperature": 25}'
Giám sát log nhận/gửi dữ liệu tại Gateway và Backend.

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
