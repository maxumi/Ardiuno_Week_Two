var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
var touchChart;
var chartData = {
  labels: [],
  touchCounts: [],
  touchRates: []
};

window.addEventListener('load', onLoad);

function onLoad() {
  initWebSocket();
  fetchChartData();
}

function initWebSocket() {
  websocket = new WebSocket(gateway);
  websocket.onmessage = event => {
    if (event.data === "reset") {
      handleChartReset();
    } else if (event.data.startsWith("New_Data:")) {
      const jsonData = event.data.slice(9);
      try {
        const dataPoint = JSON.parse(jsonData);
        updateChart(dataPoint);
        updateTouchCounter(dataPoint.touchCount);
      } catch (e) {
        console.error("Error parsing JSON data:", e);
      }
    } else {
      updateTouchCounter(event.data);
    }
  };
  
  websocket.onerror = error => console.error("WebSocket Error:", error);
  websocket.onclose = () => setTimeout(initWebSocket, 5000);
}

function fetchChartData() {
  fetch('/data')
    .then(response => response.json())
    .then(data => {
      initializeChartData(data);
      displayChart();
    })
    .catch(error => console.error('Error fetching chart data:', error));
}

function initializeChartData(data) {
  chartData.labels = data.map(entry => new Date(entry.timestamp).toLocaleTimeString());
  chartData.touchCounts = data.map(entry => entry.touchCount);
  chartData.touchRates = data.map(entry => entry.touchRate);
}

function displayChart() {
  const ctx = document.getElementById('touchChart').getContext('2d');

  if (touchChart) touchChart.destroy();

  touchChart = new Chart(ctx, {
    type: 'line',
    data: {
      labels: chartData.labels,
      datasets: [
        {
          label: 'Total Touch Count',
          data: chartData.touchCounts,
          borderColor: 'rgba(75, 192, 192, 1)',
          backgroundColor: 'rgba(75, 192, 192, 0.2)',
          yAxisID: 'y'
        },
        {
          label: 'Touch Rate (per minute)',
          data: chartData.touchRates,
          borderColor: 'rgba(255, 99, 132, 1)',
          backgroundColor: 'rgba(255, 99, 132, 0.2)',
          yAxisID: 'y1'
        }
      ]
    },
    options: {
      responsive: true,
      interaction: { mode: 'index', intersect: false },
      stacked: false,
      plugins: { title: { display: true, text: 'Touch Data Chart' } },
      scales: {
        y: { type: 'linear', position: 'left', title: { display: true, text: 'Total Touch Count' } },
        y1: { type: 'linear', position: 'right', grid: { drawOnChartArea: false }, title: { display: true, text: 'Touch Rate (per minute)' } },
        x: { title: { display: true, text: 'Time' } }
      }
    }
  });
}

function updateChart(dataPoint) {
  const timeLabel = new Date(dataPoint.timestamp).toLocaleTimeString();
  chartData.labels.push(timeLabel);
  chartData.touchCounts.push(dataPoint.touchCount);
  chartData.touchRates.push(dataPoint.touchRate);

  // Limit the data arrays to the last 100 entries
  if (chartData.labels.length > 100) {
    chartData.labels.shift();
    chartData.touchCounts.shift();
    chartData.touchRates.shift();
  }

  // Update the chart with new data
  touchChart.data.labels = chartData.labels;
  touchChart.data.datasets[0].data = chartData.touchCounts;
  touchChart.data.datasets[1].data = chartData.touchRates;
  touchChart.update();
}

function updateTouchCounter(touchCount) {
  document.getElementById('touchCounter').innerText = touchCount;
}

function requestChartReset() {
  fetch('/reset')
    .then(response => response.text())
    .then(message => {
      if (message === "Data has been reset.") handleChartReset();
      document.getElementById('resetStatus').innerText = message;
    })
    .catch(error => {
      console.error("Error sending reset request:", error);
      document.getElementById('resetStatus').innerText = "Reset failed. Please try again.";
    });
}

function handleChartReset() {
  document.getElementById('touchCounter').innerText = '0';
  if (touchChart) touchChart.destroy();
  chartData.labels = [];
  chartData.touchCounts = [];
  chartData.touchRates = [];
  fetchChartData();
  setTimeout(() => document.getElementById('resetStatus').innerText = '', 5000);
}
