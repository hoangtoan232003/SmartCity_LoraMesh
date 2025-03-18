import paho.mqtt.client as mqtt
import mysql.connector
import json

# Cấu hình kết nối MySQL
db_config = {
    'host': 'localhost',         
    'user': 'root',              
    'password': '12345',        
    'database': 'sensor'   # Đổi từ "test" thành "sensor"
}

# Kết nối MySQL
conn = mysql.connector.connect(**db_config)
cursor = conn.cursor()

# Hàm xử lý khi nhận dữ liệu từ MQTT
def on_message(client, userdata, msg):
    print(f"Nhận dữ liệu từ topic {msg.topic}: {msg.payload.decode()}")
    try:
        # Parse JSON
        data = json.loads(msg.payload.decode())
        
        temperature = data.get("temperature")
        humidity = data.get("humidity")

        # Câu lệnh chèn dữ liệu vào bảng sensor.sensor_data
        sql = "INSERT INTO sensor_data (temperature, humidity) VALUES (%s, %s)"
        cursor.execute(sql, (temperature, humidity))
        conn.commit()
        
        print("Đã chèn dữ liệu vào MySQL.")
    except Exception as e:
        print("Lỗi khi xử lý message:", e)
# Hàm xử lý khi kết nối MQTT Broker
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Kết nối MQTT Broker thành công!")
        client.subscribe("sensor/data")  # Đăng ký topic
    else:
        print(f"Kết nối MQTT thất bại, mã lỗi: {rc}")

# Thiết lập client MQTT
client = mqtt.Client()
client.username_pw_set(username="tho", password="1")  # Nếu cần username & password

client.on_connect = on_connect
client.on_message = on_message

# Kết nối tới Mosquitto MQTT Broker (localhost, port 1883)
client.connect("localhost", 1883, 60)

# Chạy vòng lặp MQTT
client.loop_forever()
