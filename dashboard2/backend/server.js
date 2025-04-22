const express = require('express');
const app = express();
const nodemailer = require('nodemailer');
const cors = require('cors');
const fetch = require('node-fetch');
require('dotenv').config();

// CORS configuration
app.use(cors({
  origin: [
    'http://localhost:5173',  // Vite dev server
    'http://localhost:3001',  // Alternative local development
    'https://owl-wildfire.vercel.app/'  // Your production frontend URL
  ],
  methods: ['GET', 'POST'],
  credentials: true
}));
app.use(express.json());

// Before server starts
console.log("About to start server...");

// After server starts
console.log("Server started successfully on port 8081");

// If you're using Express or similar:

// Route to get authentication token
app.get('/api/auth', async (req, res) => {
  try {
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

// Alert endpoint
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
      to: process.env.EMAIL_USER, // Your email address
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

// Export the app
module.exports = app;
