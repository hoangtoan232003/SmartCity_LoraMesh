# 🔌 Hệ Thống Đô Thị Thông Minh Sử Dụng LoRa Mesh

> Hệ thống giám sát và thu thập dữ liệu môi trường trong khu đô thị thông minh, sử dụng mạng cảm biến không dây LoRa Mesh để truyền dữ liệu từ các node cảm biến đến Gateway. Gateway sẽ gửi dữ liệu lên backend thông qua MQTT, backend lưu vào cơ sở dữ liệu MySQL và cung cấp API phục vụ giao diện dashboard.

---

## 👋 Giới Thiệu

Hệ thống giám sát môi trường ứng dụng trong đô thị thông minh, sử dụng LoRa Mesh để truyền tải dữ liệu từ nhiều node cảm biến đến Gateway. Dữ liệu sau đó được gửi đến backend thông qua MQTT, lưu trữ tại MySQL và phục vụ dashboard qua API.

**Mục tiêu:** Nghiên cứu và phát triển hệ thống IoT có khả năng mở rộng, tiết kiệm năng lượng, ứng dụng trong đô thị thông minh.

**Đối tượng sử dụng:** Sinh viên, nhà nghiên cứu IoT, các dự án giáo dục và phát triển đô thị thông minh.

---

## Cấu Trúc Dự Án
- backend/        # Mã nguồn backend
- frontend/       # Mã nguồn frontend
- hardware_code/  # Code điều khiển phần cứng
- docs/           # Tài liệu dự án
- results/        # Kết quả test (.csv,...)


## 📐 Thông Số Kỹ Thuật

| Thành phần      | Mô tả                                |
|----------------|----------------------------------------|
| Node cảm biến  | ESP32 + DHT11, BH1750, cảm biến âm thanh |
| Gateway        | ESP32 + LoRa, kết nối WiFi             |
| Giao tiếp      | LoRa Mesh, MQTT                       |
| Backend        | Flask + MQTT + MySQL                   |
| CSDL           | MySQL                                  |

---

## 🧰 Danh Sách Linh Kiện

| Tên linh kiện            | Số lượng | Ghi chú                         |
|--------------------------|----------|---------------------------------|
| ESP32 DevKit             | 2+       | Dùng cho cả node và gateway    |
| SX1278 LoRa Module       | 2+       | Giao tiếp không dây            |
| Cảm biến DHT11           | 1        | Nhiệt độ, độ ẩm                 |
| Cảm biến ánh sáng BH1750 | 1        | Đo cường độ ánh sáng           |
| Micro âm thanh           | 1        | Đo độ ồn                        |


---


### 📎 Kết nối LoRa với ESP32 (SPI)

| ESP32 | LoRa SX1278 |
|-------|--------------|
| 18    | SCK          |
| 19    | MISO         |
| 23    | MOSI         |
| 5     | NSS (CS)     |
| 14    | RESET        |
| 2     | DIO0         |

---

## 🔩 Hướng Dẫn Lắp Ráp

1. Kết nối các cảm biến với ESP32 như sơ đồ mạch.
2. Kết nối module LoRa với ESP32 theo chuẩn SPI.
3. Cấu hình mã nguồn firmware cho Node và Gateway.
4. Kiểm tra kết nối giữa các node bằng Serial monitor.
5. Lắp thử với nguồn 5V hoặc pin lithium nếu muốn di động.

---

## 💻 Lập Trình Firmware

- **Ngôn ngữ:** C++ (Arduino)
- **Thư viện cần thiết:**
  - `LoRaMesh` hoặc `RadioHead`
  - `PubSubClient` (MQTT)
  - `WiFi.h`, `DHT.h`, `BH1750.h`
- **Nạp mã nguồn:**
  - Node: `firmware/node_main.ino`
  - Gateway: `firmware/gateway_main.ino`

```bash
# Sử dụng PlatformIO
platformio run --target upload
```

---

## ⚙️ Cách Sử Dụng

### Mô hình mạng Mesh:

```plaintext
 [Node 1] --------\
    |              \
    |               \
    |                [Node 3] ------ [Gateway] ------ [MQTT Broker]
    |               /
    |              /
 [Node 2] --------/
```

- Node cảm biến đo nhiệt độ, độ ẩm, ánh sáng, tiếng ồn.
- Các node có thể chuyển tiếp dữ liệu để đảm bảo kết nối đến Gateway.
- Gateway thu thập và gửi dữ liệu lên MQTT Broker.

### Dữ liệu MQTT

- **Topic:** `city/data/<node_id>`
- **Payload mẫu:**

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

---

## ✅ Kiểm Thử

- Kiểm tra từng node với Serial monitor.
- Gửi dữ liệu thử lên Mosquitto bằng MQTT client như MQTTX hoặc MQTT Explorer.
- Kiểm tra dữ liệu có được ghi vào MySQL qua backend Flask.

---


## 🤝 Đóng Góp

- Fork repository, tạo nhánh mới và gửi pull request.
- Góp ý thêm chức năng hoặc cải tiến.
- Báo lỗi hoặc câu hỏi qua tab Issues.

---

## 👨‍💻 Tác Giả

- **Hoàng Quốc Toàn** - B21DCDT221 (Nhóm trưởng)  
- **Đào Bá Thọ** - B21DCDT217  
- **Tạ Quang Trường** - B21DCDT026  
- **Vương Tuấn Minh** - B21DCDT153

---
