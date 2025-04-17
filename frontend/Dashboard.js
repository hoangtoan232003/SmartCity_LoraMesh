let deviceStates = {};
let sensorData = {};
let charts = {};

function toggleDevice(deviceId) {
    deviceStates[deviceId] = !deviceStates[deviceId];
    let statusElement = document.getElementById(`device-status-${deviceId}`);
    let buttonElement = document.getElementById(`btn-toggle-${deviceId}`);

    if (deviceStates[deviceId]) {
        statusElement.innerText = 'ON';
        buttonElement.innerText = 'Turn OFF';
        // Gửi yêu cầu bật LED đến ESP32
        fetch(`http://localhost:5000/api/device/${deviceId}/on`, { method: 'POST' });
    } else {
        statusElement.innerText = 'OFF';
        buttonElement.innerText = 'Turn ON';
        // Gửi yêu cầu tắt LED đến ESP32
        fetch(`http://localhost:5000/api/device/${deviceId}/off`, { method: 'POST' });
    }
}


function updateChart(deviceId, newData) {
    if (!charts[deviceId]) return;
    
    let chart = charts[deviceId];
    let dataset = chart.data.datasets[0].data;
    if (dataset.length > 10) dataset.shift(); // Giới hạn 10 điểm dữ liệu
    dataset.push({ x: new Date(), y: newData });
    chart.update();
}

function initializeCharts() {
    document.querySelectorAll("canvas").forEach(canvas => {
        let deviceId = canvas.id.split('-')[1];
        let ctx = canvas.getContext('2d');
        charts[deviceId] = new Chart(ctx, {
            type: 'line',
            data: { datasets: [{ label: `Device ${deviceId} Data`, data: [], borderColor: '#007bff', fill: false }] },
            options: { scales: { x: { type: 'time', time: { unit: 'second' } }, y: { beginAtZero: true } } }
        });
    });
}

// Hàm để lấy và cập nhật dữ liệu cảm biến
function fetchSensorData() {
    fetch('http://localhost:5000/api/sensor/latest')
        .then(response => response.json())
        .then(data => {
            if (!Array.isArray(data) || data.length === 0) {
                console.log("Không có dữ liệu hợp lệ");
                return;
            }

            data.forEach(device => {
                let deviceId = device.device_id;
                let tempElement = document.getElementById(`temp-${deviceId}`);
                let humidityElement = document.getElementById(`humidity-${deviceId}`);
                let lightElement = document.getElementById(`light-${deviceId}`);
                let noiseElement = document.getElementById(`noise-${deviceId}`);
                let statusElement = document.getElementById(`device-status-${deviceId}`);
                let buttonElement = document.getElementById(`btn-toggle-${deviceId}`);

                if (tempElement && humidityElement && lightElement && noiseElement && statusElement && buttonElement) {
                    tempElement.innerText = device.temperature + " °C";
                    humidityElement.innerText = device.humidity + " %";
                    lightElement.innerText = device.light_intensity + " lux";
noiseElement.innerText = device.noise_level + " dB";

                    if (device.status === "ON") {
                        statusElement.innerText = "ON";
                        buttonElement.innerText = "Turn OFF";
                    } else {
                        statusElement.innerText = "OFF";
                        buttonElement.innerText = "Turn ON";
                    }

                    // Cập nhật biểu đồ nhiệt độ cho thiết bị
                    updateChart(deviceId, device.temperature);
                }
            });
        })
        .catch(error => {
            console.error("Lỗi khi lấy dữ liệu từ API:", error);
            alert("Không thể lấy dữ liệu cảm biến. Vui lòng thử lại sau.");
        });
}

window.onload = function() {
    initializeCharts();
    fetchSensorData(); // Lấy dữ liệu ngay khi trang tải
    setInterval(fetchSensorData, 5000); // Lấy dữ liệu mỗi 5 giây
};