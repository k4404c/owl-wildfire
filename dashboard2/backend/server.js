const express = require('express');
const app = express();
const nodemailer = require('nodemailer');
const cors = require('cors');
const fetch = require('node-fetch');
require('dotenv').config();

// CORS configuration
app.use(cors({
  origin: [
    'http://localhost:5173',
    'http://localhost:3001',
    'https://owl-wildfire1.vercel.app'  // Remove trailing slash
  ],
  methods: ['GET', 'POST'],
  credentials: true
}));
app.use(express.json());

// Health check route
app.get('/api/health', (req, res) => {
  res.json({ status: 'ok' });
});

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
  try {
    const { sensorData, alerts } = req.body;
    
    if (!sensorData || !alerts) {
      return res.status(400).json({ error: 'Missing required data' });
    }

    let transporter = nodemailer.createTransport({
      host: "smtp.gmail.com",
      port: 587,
      secure: false,
      auth: {
        user: process.env.EMAIL_USER,
        pass: process.env.EMAIL_PASSWORD,
      },
    });

    await transporter.sendMail({
      from: `"Owls Inc" <${process.env.EMAIL_USER}>`,
      to: process.env.EMAIL_USER,
      subject: "🚨 ALERT: Forest Fire Risk Detected",
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
