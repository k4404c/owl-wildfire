// // server.js
// const WebSocket = require('ws');
// const wss = new WebSocket.Server({ port: 8080 });

// At the top of your file
console.log("Starting server...");

// // wss.on('connection', (ws) => {
// //   console.log('Client connected');

// //   // Send mock data to the client every second
// //   setInterval(() => {
// //     const data = {
// //       summary: {
// //         totalDevices: 1000,
// //         activeDevices: Math.floor(800 + Math.random() * 200), // Random active devices
// //         dataPoints: Math.floor(Math.random() * 1000000), // Random data points
// //         alerts: Math.floor(Math.random() * 10), // Random alerts
// //       },
// //       activity: Array.from({ length: 7 }, (_, i) => ({
// //         date: `2023-06-${i + 1}`,
// //         activity: Math.floor(Math.random() * 10000),
// //       })),
// //       comparison: [
// //         { device: 'Device A', dataUsage: Math.floor(Math.random() * 5000) },
// //         { device: 'Device B', dataUsage: Math.floor(Math.random() * 5000) },
// //         { device: 'Device C', dataUsage: Math.floor(Math.random() * 5000) },
// //       ],
// //       distribution: [
// //         { name: 'Temperature', value: Math.floor(Math.random() * 500) },
// //         { name: 'Humidity', value: Math.floor(Math.random() * 500) },
// //       ],
// //     };
// //     ws.send(JSON.stringify(data));
// //   }, 1000);
// // // });

// // console.log('WebSocket server running on ws://localhost:8080');

const WebSocket = require('ws');
const axios = require('axios');
const express = require('express');
const app = express();
const nodemailer = require('nodemailer');
const cors = require('cors');
require('dotenv').config();

// Add cors middleware
app.use(cors());
app.use(express.json());

const wss = new WebSocket.Server({ port: 8081 });
var token = "";
wss.on('connection', (ws) => {
    console.log('Client connected');

    const startTimestamp = Math.floor(new Date('2023-01-01').getTime() / 1000);// Example start date
    async function gettoken(){
        try {
            // Step 1: Authenticate and get the token
            const authResponse = await fetch("https://beta.owldms.com/owl/api/userdata/authenticate", 
                {
                    method: 'POST',
                    body: JSON.stringify(
                        {
                            "Username": "smallaj1@asu.edu",
                            "Password": "W2AB#6~5$5E+aCu",
                        }
                    ),
                    headers: {
                        'Content-type': 'application/json; charset=UTF-8'
                    }
                }
            );
            const data = await authResponse.json();
            token = data.token;
            console.log("token: ", data.token);
        }
        catch
        {
            console.log('wtf');
        }
    }

   
        gettoken()



    const fetchData = async () => {
        const endTimestamp = Math.floor(Date.now() / 1000); // Current time as Unix timestamp
        const apiUrl = 'https://beta.owldms.com/owl/api/userdata'; // Example API URL
        const papaId = "ASUPAPA2"

        try {
        const response = await fetch(
            `${apiUrl}/getRawData?start=${startTimestamp}&end=${endTimestamp}&papaId=ASUPAPA2`,
            {
                method: 'GET',
                headers: {
                    
                    'Authorization': `Bearer ${token}`,
                },
            }
        );

        if (!response.ok) {
            throw new Error(`Error: ${response.status} ${response.statusText}`);
        }

        const data = await response.json();
        console.log('Fetched Data:', data);

        // Process the raw data as needed
        data.forEach((item) => {
            console.log('Payload:', JSON.parse(item.payload));
        });
    } catch (error) {
        console.error('Error fetching data:', error.message);
    }
    };

    // Start streaming data
    fetchData();

    
    ws.on('close', () => console.log('Client disconnected'));
});

// Before server starts
console.log("About to start server...");

// After server starts
console.log("Server started successfully on port 8081");

// If you're using Express or similar:


app.post('/api/send-alert', async (req, res) => {
    const { sensorData, alerts } = req.body;
    
    console.log('Received alert request:', { sensorData, alerts }); // Debug log
    
    let transporter = nodemailer.createTransport({
        host: "smtp.gmail.com",
        port: 587,
        secure: false, // true for 465, false for other ports
        auth: {
          user: process.env.EMAIL_USER,
          pass: process.env.EMAIL_PASSWORD,
        },
      });
  
    try {
        await transporter.sendMail({
            from: `"Owls Inc" <${process.env.EMAIL_USER}>`,
            to: `${process.env.EMAIL_USER}`, // Your email address
            subject: "🚨 ALERT: Forest Fire Risk Detected",
            text:  alerts.join('\n\n') + `
        
            Location Information:
            - Latitude: ${sensorData.latitude}
            - Longitude: ${sensorData.longitude}
            
            IMMEDIATE ACTION REQUIRED
            Please investigate this location immediately.
            
            This is an automated alert based on our fire prediction model.`,
            html: `<div style="font-family: Arial, sans-serif; max-width: 600px; margin: 0 auto;">
        <h1 style="color: #ff4444; text-align: center;">🚨 FIRE RISK ALERT 🚨</h1>
        
        <div style="background-color: #fff3f3; padding: 20px; border-radius: 10px; margin: 20px 0;">
            <h2 style="color: #333;">Alert Details:</h2>
            ${alerts.map(alert => `<p style="color: #666;">${alert}</p>`).join('')}
        </div>

        <div style="background-color: #f5f5f5; padding: 20px; border-radius: 10px; margin: 20px 0;">
            <h2 style="color: #333;">Location Information:</h2>
            <p style="color: #666;">
                <strong>Latitude:</strong> ${sensorData.latitude}<br>
                <strong>Longitude:</strong> ${sensorData.longitude}<br>
                <strong>Altitude:</strong> ${sensorData.altitude}m
            </p>
            
            <div style="margin-top: 10px;">
                <a href="https://www.google.com/maps?q=${sensorData.latitude},${sensorData.longitude}" 
                   style="background-color: #4CAF50; color: white; padding: 10px 20px; text-decoration: none; border-radius: 5px; display: inline-block;">
                    View on Google Maps
                </a>
            </div>
        </div>

        <div style="background-color: #f5f5f5; padding: 20px; border-radius: 10px; margin: 20px 0;">
            <h2 style="color: #333;">Sensor Readings:</h2>
            <p style="color: #666;">
                <strong>Temperature:</strong> ${sensorData.temperature}°C<br>
                <strong>Humidity:</strong> ${sensorData.humidity}%<br>
                <strong>Pressure:</strong> ${sensorData.pressure} hPa<br>
                <strong>Gas Level:</strong> ${sensorData.gas}<br>
                <strong>Prediction:</strong> ${sensorData.prediction}
            </p>
        </div>

        <div style="background-color: #f5f5f5; padding: 20px; border-radius: 10px; margin: 20px 0;">
            <h2 style="color: #333;">Device Information:</h2>
            <p style="color: #666;">
                <strong>Device ID:</strong> ${sensorData.deviceId}<br>
                <strong>Message ID:</strong> ${sensorData.messageId}<br>
                <strong>Event Type:</strong> ${sensorData.eventType}<br>
                <strong>Time:</strong> ${sensorData.timestamp}
            </p>
        </div>

        <div style="background-color: #ff4444; color: white; padding: 20px; border-radius: 10px; margin: 20px 0; text-align: center;">
            <h2 style="margin: 0;">IMMEDIATE ACTION REQUIRED</h2>
            <p>Please investigate this location immediately.</p>
        </div>

        <p style="color: #666; font-size: 12px; text-align: center;">
            This is an automated alert based on our fire prediction model.<br>
            Generated at ${new Date().toLocaleString()}
        </p>
    </div>`,
          });
      await transporter.sendMail({
        from: 'satwikug@gmail.com',
        to: process.env.EMAIL_USER, // Send to yourself for testing
        subject: '🚨 ALERT: Forest Fire Risk Detected',
        text: alerts.join('\n\n') + `
        
        Location Information:
        - Latitude: ${sensorData.latitude}
        - Longitude: ${sensorData.longitude}
        
        IMMEDIATE ACTION REQUIRED
        Please investigate this location immediately.
        
        This is an automated alert based on our fire prediction model.`
      });
      res.json({ success: true });
    } catch (error) {
      console.error('Email error:', error);
      res.status(500).json({ error: error.message });
    }
  });

  // Add these routes after your existing code but before app.listen()

  // Route to get authentication token
  app.get('/api/auth', async (req, res) => {
    try {
      const authResponse = await fetch(
        "https://beta.owldms.com/owl/api/userdata/authenticate",
        {
          method: 'POST',
          body: JSON.stringify({
            "Username": "smallaj1@asu.edu",
            "Password": "W2AB#6~5$5E+aCu",
          }),
          headers: {
            'Content-type': 'application/json; charset=UTF-8'
          }
        }
      );

      if (!authResponse.ok) {
        throw new Error('Authentication failed');
      }

      const data = await authResponse.json();
      res.json({ token: data.token });
    } catch (error) {
      console.error('Auth error:', error);
      res.status(500).json({ error: error.message });
    }
  });

  // Route to get sensor data
  app.get('/api/sensor-data', async (req, res) => {
    try {
      // First get the auth token
      const authResponse = await fetch(
        "https://beta.owldms.com/owl/api/userdata/authenticate",
        {
          method: 'POST',
          body: JSON.stringify({
            "Username": process.env.USERNAME,
            "Password": process.env.PASSWORD,
          }),
          headers: {
            'Content-type': 'application/json; charset=UTF-8'
          }
        }
      );

      if (!authResponse.ok) {
        throw new Error('Authentication failed');
      }

      const authData = await authResponse.json();
      const token = authData.token;

      // Get the time parameters from query string or use defaults
      const startTimestamp = req.query.start || Math.floor(new Date('2025-03-01').getTime() / 1000);
      const endTimestamp = req.query.end || Math.floor(Date.now() / 1000);

      // Then fetch the sensor data
      const dataResponse = await fetch(
        `https://beta.owldms.com/owl/api/userdata/getRawData?start=${startTimestamp}&end=${endTimestamp}&papaId=ASUPAPA2`,
        {
          headers: {
            'Authorization': `Bearer ${token}`,
          },
        }
      );

      if (!dataResponse.ok) {
        throw new Error('Failed to fetch sensor data');
      }

      const sensorData = await dataResponse.json();
      res.json(sensorData);

    } catch (error) {
      console.error('Data fetch error:', error);
      res.status(500).json({ error: error.message });
    }
  });

  app.listen(3010, () => {
    console.log(`Server running on port 30`);
  }).on('error', (err) => {
    console.error('Server failed to start:', err);
  });

