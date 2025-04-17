import paho.mqtt.client as mqtt
import mysql.connector
import json
from flask import Flask, jsonify
from flask_cors import CORS  # Thêm dòng này

# Cấu hình Flask
app = Flask(__name__)
CORS(app)  # Cho phép CORS với tất cả các domain
# Cấu hình kết nối MySQL
db_config = {
    'host': 'localhost',
    'user': 'root',
    'password': '123456789',
    'database': 'test'
}

# Kết nối MySQL
conn = mysql.connector.connect(**db_config)
cursor = conn.cursor()

# Xử lý dữ liệu khi có bản tin MQTT đến
def on_message(client, userdata, msg):
    print(f"Nhận dữ liệu từ topic {msg.topic}: {msg.payload.decode()}")

    try:
        data = json.loads(msg.payload.decode())

        device_id = data.get("device_id", 1)
        status = data.get("status", "OFF")
        temperature = data.get("temperature")
        humidity = data.get("humidity")
        light_intensity = data.get("light_intensity")
        noise_level = data.get("noise_level")
        count = data.get("count")  # Count do thiết bị tự tăng

        if None in (temperature, humidity, light_intensity, noise_level, count):
            print("Dữ liệu không đầy đủ. Bỏ qua.")
            return

        sql = """
            INSERT INTO device_data (
                device_id, status, temperature, humidity,
                light_intensity, noise_level, count
            ) VALUES (%s, %s, %s, %s, %s, %s, %s)
        """
        cursor.execute(sql, (
            device_id, status, temperature, humidity,
            light_intensity, noise_level, count
        ))
        conn.commit()
        print("✅ Đã lưu dữ liệu vào MySQL.")

    except Exception as e:
        print("❌ Lỗi xử lý dữ liệu MQTT:", e)


# Hàm xử lý khi kết nối MQTT Broker
def on_connect(client, userdata, flags, reason_code, properties=None):
    if reason_code == 0:
        print("Kết nối MQTT Broker thành công!")
        client.subscribe("sensor/data")
    else:
        print(f"Kết nối MQTT thất bại, mã lỗi: {reason_code}")


# API để lấy dữ liệu mới nhất từ MySQL
# API để lấy dữ liệu mới nhất từ MySQL
@app.route('/api/sensor/latest', methods=['GET'])
def get_latest_sensor_data():
    try:
        cursor.execute("""
            SELECT t1.* FROM device_data t1
            JOIN (
                SELECT device_id, MAX(id) AS max_id
                FROM device_data
                GROUP BY device_id
            ) t2 ON t1.device_id = t2.device_id AND t1.id = t2.max_id
        """)
        rows = cursor.fetchall()

        if rows:
            data = [{
                "id": row[0],
                "device_id": row[1],
                "status": row[2],
                "temperature": row[3],
                "humidity": row[4],
                "light_intensity": row[5],
                "noise_level": row[6],
                "count": row[7],  # Thêm cột count vào dữ liệu trả về
                "timestamp": row[8]
            } for row in rows]

            return jsonify(data)
        else:
            return jsonify([]), 404  # Trả về danh sách rỗng nếu không có dữ liệu
    except Exception as e:
        return jsonify({"error": str(e)}), 500
    
# API để bật LED
@app.route('/api/device/<int:device_id>/on', methods=['POST'])
def turn_on_led(device_id):
    try:
        # Gửi thông điệp MQTT để bật LED
        message = json.dumps({"device_id": device_id, "status": "ON"})
        client.publish(f"device/{device_id}/led", message)
        return jsonify({"message": "LED turned ON"}), 200
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# API để tắt LED
@app.route('/api/device/<int:device_id>/off', methods=['POST'])
def turn_off_led(device_id):
    try:
        # Gửi thông điệp MQTT để tắt LED
        message = json.dumps({"device_id": device_id, "status": "OFF"})
        client.publish(f"device/{device_id}/led", message)
        return jsonify({"message": "LED turned OFF"}), 200
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# Thiết lập client MQTT
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

client.on_connect = on_connect
client.on_message = on_message

client.connect("localhost", 1883, 60)

# Chạy Flask và MQTT song song
if __name__ == '__main__':
    from threading import Thread

    # Chạy MQTT trong một luồng riêng
    mqtt_thread = Thread(target=client.loop_forever)
    mqtt_thread.daemon = True
    mqtt_thread.start()

    # Chạy Flask API
    app.run(host='0.0.0.0', port=5000, debug=False)

