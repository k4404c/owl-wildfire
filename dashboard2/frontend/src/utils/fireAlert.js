export const sendFireAlert = async (sensorData) => {
  try {
    const response = await fetch('https://owl-wildfire-9swm.vercel.app/api/send-alert', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({ 
        sensorData,
        alerts: [
          `🚨 Fire Risk Detected (Prediction: ${sensorData.prediction})`,
          `Sensor Readings:`,
          `Temperature: ${sensorData.temperature}°C`,
          `Humidity: ${sensorData.humidity}%`,
          `Gas Level: ${sensorData.gas}`,
          `Pressure: ${sensorData.pressure} hPa`,
          `Altitude: ${sensorData.altitude}m`,
          `Device ID: ${sensorData.deviceId}`,
          `Message ID: ${sensorData.messageId}`,
          `Time: ${sensorData.timestamp}`
        ]
      }),
    });
    
    if (!response.ok) {
      throw new Error('Failed to send alert');
    }
    
    return await response.json();
  } catch (error) {
    console.error('Error sending alert:', error);
    throw error;
  }
}; 