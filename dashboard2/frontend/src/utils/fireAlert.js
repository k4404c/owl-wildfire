import nodemailer from 'nodemailer';

// Configure these thresholds based on your requirements
const ALERT_THRESHOLDS = {
  temperature: 45, // °C
  humidity: 15,    // % (low humidity is dangerous)
  gas: 70000      // Your gas sensor threshold
};

// Email configuration
const emailConfig = {
  from: 'satwikug@gmail.com',
  to: ['satwikug@gmail.com', ''],
  subject: '🚨 ALERT: Potential Forest Fire Detected',
};

// Create email transporter
const transporter = nodemailer.createTransport({
  service: 'gmail',
  auth: {
    user: process.env.EMAIL_USER,
    pass: process.env.EMAIL_PASSWORD,
  }
});

export const checkFireConditions = (sensorData) => {
  const alerts = [];

  // Check temperature
  if (sensorData.temperature > ALERT_THRESHOLDS.temperature) {
    alerts.push(`High temperature detected: ${sensorData.temperature}°C`);
  }

  // Check humidity
  if (sensorData.humidity < ALERT_THRESHOLDS.humidity) {
    alerts.push(`Low humidity detected: ${sensorData.humidity}%`);
  }

  // Check gas levels
  if (sensorData.gas > ALERT_THRESHOLDS.gas) {
    alerts.push(`Elevated gas levels detected: ${sensorData.gas}`);
  }

  return alerts;
};

export const sendFireAlert = async (sensorData, alerts) => {
  const emailBody = `
    🚨 POTENTIAL FIRE HAZARD DETECTED 🚨
    
    Location: ${sensorData.latitude}, ${sensorData.longitude}
    Time: ${new Date().toLocaleString()}
    
    Alert Conditions:
    ${alerts.join('\n')}
    
    Sensor Readings:
    - Temperature: ${sensorData.temperature}°C
    - Humidity: ${sensorData.humidity}%
    - Gas Level: ${sensorData.gas}
    
    Device Information:
    - Device ID: ${sensorData.deviceId}
    - Message ID: ${sensorData.messageId}
    
    Please investigate immediately.
  `;

  try {
    await transporter.sendMail({
      ...emailConfig,
      text: emailBody,
    });
    console.log('Fire alert email sent successfully');
    return true;
  } catch (error) {
    console.error('Failed to send fire alert email:', error);
    return false;
  }
}; 