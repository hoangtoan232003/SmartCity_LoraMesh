let deviceStates = {};
let sensorData = {};
let charts = {};

async function toggleDevice(deviceId) {
    const statusElement = document.getElementById(`device-status-${deviceId}`);
    const buttonElement = document.getElementById(`btn-toggle-${deviceId}`);
    const currentStatus = statusElement.innerText.trim();

    const newStatus = currentStatus === "OFF" ? "ON" : "OFF";
    const endpoint = `http://localhost:5000/api/device/${deviceId}/${newStatus.toLowerCase()}`;

    try {
        const response = await fetch(endpoint, { method: 'POST' });
        if (response.ok) {
            statusElement.innerText = newStatus;
            buttonElement.innerText = (newStatus === "ON") ? "Turn OFF" : "Turn ON";
        } else {
            const err = await response.json();
            alert("Lỗi điều khiển thiết bị: " + (err.error || "Không rõ lỗi"));
        }
    } catch (error) {
        console.error("Lỗi khi gửi yêu cầu điều khiển:", error);
        alert("Không thể điều khiển thiết bị.");
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

                if (tempElement && humidityElement && lightElement && noiseElement) {
                    tempElement.innerText = device.temperature + " °C";
                    humidityElement.innerText = device.humidity + " %";
                    lightElement.innerText = device.light_intensity + " lux";
                    noiseElement.innerText = device.noise_level + " dB";

                    // Không cập nhật status và nút ở đây nữa
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