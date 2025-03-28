let deviceStates = [false, false, false];
let sensorData1 = [];
let sensorData2 = [];
let sensorData3 = [];
let chart1, chart2, chart3;

function toggleDevice(device) {
    deviceStates[device - 1] = !deviceStates[device - 1];
    let statusElement = document.getElementById(`device-status-${device}`);
    let buttonElement = document.getElementById(`btn-toggle-${device}`);

    if (deviceStates[device - 1]) {
        statusElement.innerText = 'ON';
        buttonElement.innerText = 'Turn OFF';
    } else {
        statusElement.innerText = 'OFF';
        buttonElement.innerText = 'Turn ON';
    }
}

function updateChart(chart, sensorData) {
    if (sensorData.length > 10) sensorData.shift();
    sensorData.push({ x: new Date().toLocaleTimeString(), y: Math.random() * 100 });
    chart.update();
}

let chartMode = false;

function switchMode() {
    chartMode = !chartMode;
    const deviceSections = document.querySelectorAll(".section");

    deviceSections.forEach(section => {
        const status = section.querySelector("span");
        const button = section.querySelector("button");
        const sensorData = section.querySelector(".sensor-data");

        if (chartMode) {
            status.style.display = "none";
            button.style.display = "none";
            sensorData.style.display = "none";
        } else {
            status.style.display = "block";
            button.style.display = "block";
            sensorData.style.display = "block";
        }
    });

    document.getElementById("btn-switch-mode").innerText = chartMode ? "View Status" : "View Charts Only";
}

window.onload = function() {
    const ctx1 = document.getElementById('sensorChart1')?.getContext('2d');
    const ctx2 = document.getElementById('sensorChart2')?.getContext('2d');
    const ctx3 = document.getElementById('sensorChart3')?.getContext('2d');

    if (ctx1) {
        chart1 = new Chart(ctx1, {
            type: 'line',
            data: { datasets: [{ label: 'Sensor Data 1', data: [], borderColor: '#007bff', fill: false }] },
            options: { scales: { x: { type: 'time', time: { unit: 'second' } }, y: { beginAtZero: true } } }
        });
    }

    if (ctx2) {
        chart2 = new Chart(ctx2, {
            type: 'line',
            data: { datasets: [{ label: 'Sensor Data 2', data: [], borderColor: '#28a745', fill: false }] },
            options: { scales: { x: { type: 'time', time: { unit: 'second' } }, y: { beginAtZero: true } } }
        });
    }

    if (ctx3) {
        chart3 = new Chart(ctx3, {
            type: 'line',
            data: { datasets: [{ label: 'Sensor Data 3', data: [], borderColor: '#ff5733', fill: false }] },
            options: { scales: { x: { type: 'time', time: { unit: 'second' } }, y: { beginAtZero: true } } }
        });
    }

    setInterval(() => {
        if (chart1) updateChart(chart1, sensorData1);
        if (chart2) updateChart(chart2, sensorData2);
        if (chart3) updateChart(chart3, sensorData3);
    }, 2000);
};

function fetchSensorData() {
    fetch('http://localhost:5000/api/sensor/latest')
        .then(response => response.json())
        .then(data => {
            if (data.error || data.message) {
                console.log("Không có dữ liệu hợp lệ");
                return;
            }

            document.getElementById("temp-1").innerText = data.temperature + " °C";
            document.getElementById("humidity-1").innerText = data.humidity + " %";
            document.getElementById("light-1").innerText = data.light_intensity + " lux";
            document.getElementById("noise-1").innerText = data.noise_level + " dB";

            // Cập nhật trạng thái thiết bị
            let statusElement = document.getElementById("device-status-1");
            let buttonElement = document.getElementById("btn-toggle-1");

            if (data.status === "ON") {
                statusElement.innerText = "ON";
                buttonElement.innerText = "Turn OFF";
            } else {
                statusElement.innerText = "OFF";
                buttonElement.innerText = "Turn ON";
            }
        })
        .catch(error => console.error("Lỗi khi lấy dữ liệu từ API:", error));
}

// Gọi API mỗi 5 giây để cập nhật dữ liệu cảm biến
setInterval(fetchSensorData, 5000);

// Gọi ngay khi tải trang
fetchSensorData();
