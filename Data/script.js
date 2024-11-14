var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
var touchChart;

window.addEventListener('load', onLoad);

function onLoad() {
  initWebSocket();
  fetchChartData();
}

function initWebSocket() {
  websocket = new WebSocket(gateway);
  websocket.onmessage = event => {
    if (event.data === "reset") handleChartReset();
    else document.getElementById('touchCounter').innerText = event.data;
  };
  websocket.onerror = error => console.error("WebSocket Error:", error);
  websocket.onclose = () => setTimeout(initWebSocket, 5000);
}

function fetchChartData() {
  fetch('/data')
    .then(response => response.json())
    .then(data => displayChart(data))
    .catch(error => console.error('Error fetching chart data:', error));
}

function displayChart(data) {
  const ctx = document.getElementById('touchChart').getContext('2d');
  const labels = data.map(entry => new Date(parseInt(entry.timestamp)).toLocaleTimeString());
  const touchCounts = data.map(entry => entry.touchCount);
  const touchRates = data.map(entry => entry.touchRate);

  if (touchChart) touchChart.destroy();

  touchChart = new Chart(ctx, {
    type: 'line',
    data: {
      labels: labels,
      datasets: [
        {
          label: 'Total Touch Count',
          data: touchCounts,
          borderColor: 'rgba(75, 192, 192, 1)',
          backgroundColor: 'rgba(75, 192, 192, 0.2)',
          yAxisID: 'y'
        },
        {
          label: 'Touch Rate (per minute)',
          data: touchRates,
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

function requestChartReset() {
  fetch('/reset')
    .then(response => response.text())
    .then(message => {
      if (message === "Chart data has been reset.") handleChartReset();
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
  fetchChartData();
  setTimeout(() => document.getElementById('resetStatus').innerText = '', 5000);
}
