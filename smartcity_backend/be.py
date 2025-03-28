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
    'password': '12345',
    'database': 'sensor'
}

# Kết nối MySQL
conn = mysql.connector.connect(**db_config)
cursor = conn.cursor()

# Hàm xử lý khi nhận dữ liệu từ MQTT
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

        if temperature is None or humidity is None or light_intensity is None or noise_level is None:
            print("Lỗi: Dữ liệu nhận được không đầy đủ.")
            return

        sql = """
            INSERT INTO device_data (device_id, status, temperature, humidity, light_intensity, noise_level)
            VALUES (%s, %s, %s, %s, %s, %s)
        """
        cursor.execute(sql, (device_id, status, temperature, humidity, light_intensity, noise_level))
        conn.commit()

        print("Đã chèn dữ liệu vào MySQL.")
    except Exception as e:
        print("Lỗi khi xử lý message:", e)

# Hàm xử lý khi kết nối MQTT Broker
def on_connect(client, userdata, flags, reason_code, properties=None):
    if reason_code == 0:
        print("Kết nối MQTT Broker thành công!")
        client.subscribe("sensor/data")
    else:
        print(f"Kết nối MQTT thất bại, mã lỗi: {reason_code}")


# API để lấy dữ liệu mới nhất từ MySQL
@app.route('/api/sensor/latest', methods=['GET'])
def get_latest_sensor_data():
    try:
        cursor.execute("SELECT * FROM device_data ORDER BY id DESC LIMIT 1")
        row = cursor.fetchone()
        if row:
            data = {
                "id": row[0],
                "device_id": row[1],
                "status": row[2],
                "temperature": row[3],
                "humidity": row[4],
                "light_intensity": row[5],
                "noise_level": row[6],
                "timestamp": row[7]
            }
            return jsonify(data)
        else:
            return jsonify({"message": "Không có dữ liệu"}), 404
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# Thiết lập client MQTT
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.username_pw_set(username="tho", password="1")

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

