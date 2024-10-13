// server.js
const WebSocket = require('ws');
const wss = new WebSocket.Server({ port: 8080 });

wss.on('connection', (ws) => {
  console.log('Client connected');

  // Send mock data to the client every second
  setInterval(() => {
    const data = {
      summary: {
        totalDevices: 1000,
        activeDevices: Math.floor(800 + Math.random() * 200), // Random active devices
        dataPoints: Math.floor(Math.random() * 1000000), // Random data points
        alerts: Math.floor(Math.random() * 10), // Random alerts
      },
      activity: Array.from({ length: 7 }, (_, i) => ({
        date: `2023-06-${i + 1}`,
        activity: Math.floor(Math.random() * 10000),
      })),
      comparison: [
        { device: 'Device A', dataUsage: Math.floor(Math.random() * 5000) },
        { device: 'Device B', dataUsage: Math.floor(Math.random() * 5000) },
        { device: 'Device C', dataUsage: Math.floor(Math.random() * 5000) },
      ],
      distribution: [
        { name: 'Temperature', value: Math.floor(Math.random() * 500) },
        { name: 'Humidity', value: Math.floor(Math.random() * 500) },
      ],
    };
    ws.send(JSON.stringify(data));
  }, 1000);
});

console.log('WebSocket server running on ws://localhost:8080');
