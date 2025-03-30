function createChart(canvasId) {
    const ctx = document.getElementById(canvasId).getContext('2d');
    return new Chart(ctx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [
                { label: 'Temperature (°C)', data: [], borderColor: 'red', fill: false },
                { label: 'Humidity (%)', data: [], borderColor: 'blue', fill: false },
                { label: 'Light (lux)', data: [], borderColor: 'yellow', fill: false },
                { label: 'Noise (dB)', data: [], borderColor: 'green', fill: false }
            ]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                x: { title: { display: true, text: 'Time' } },
                y: { beginAtZero: true }
            }
        }
    });
}

let charts = {
    1: createChart('sensorChart1'),
    2: createChart('sensorChart2'),
    3: createChart('sensorChart3')
};

function fetchSensorData() {
    fetch('http://localhost:5000/api/sensor/latest')
        .then(response => response.json())
        .then(data => {
            if (!Array.isArray(data) || data.length === 0) {
                console.error('No valid data received');
                return;
            }

            let now = new Date().toLocaleTimeString();
            data.forEach(device => {
                let chart = charts[device.device_id];
                if (chart) {
                    if (chart.data.labels.length > 10) chart.data.labels.shift();
                    chart.data.labels.push(now);

                    chart.data.datasets[0].data.push(device.temperature);
                    chart.data.datasets[1].data.push(device.humidity);
                    chart.data.datasets[2].data.push(device.light_intensity);
                    chart.data.datasets[3].data.push(device.noise_level);

                    chart.data.datasets.forEach(dataset => {
                        if (dataset.data.length > 10) dataset.data.shift();
                    });
                    chart.update();
                }
            });
        })
        .catch(error => console.error('Error fetching sensor data:', error));
}

setInterval(fetchSensorData, 5000);
fetchSensorData();
