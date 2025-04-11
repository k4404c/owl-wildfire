import React, { useState, useEffect } from 'react';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer } from 'recharts';
import './Dashboard.css';

function Dashboard() {
  const [data, setData] = useState([]);
  const [connected, setConnected] = useState(false);
  
  useEffect(() => {
    // Connect to WebSocket server
    const ws = new WebSocket('ws://localhost:8081');
    
    ws.onopen = () => {
      console.log('Connected to WebSocket server');
      setConnected(true);
    };
    
    ws.onmessage = (event) => {
      const newData = JSON.parse(event.data);
      setData(prevData => {
        // Keep only the last 20 data points
        const updatedData = [...prevData, newData];
        if (updatedData.length > 20) {
          return updatedData.slice(updatedData.length - 20);
        }
        return updatedData;
      });
    };
    
    ws.onclose = () => {
      console.log('Disconnected from WebSocket server');
      setConnected(false);
    };
    
    return () => {
      ws.close();
    };
  }, []);
  
  return (
    <div className="dashboard">
      <h1>IoT Dashboard</h1>
      <div className="connection-status">
        Status: {connected ? 'Connected' : 'Disconnected'}
      </div>
      
      <div className="chart-container">
        <h2>Temperature Readings</h2>
        <ResponsiveContainer width="100%" height={300}>
          <LineChart data={data}>
            <CartesianGrid strokeDasharray="3 3" />
            <XAxis dataKey="timestamp" />
            <YAxis />
            <Tooltip />
            <Legend />
            <Line type="monotone" dataKey="readings.temperature" stroke="#8884d8" name="Temperature" />
          </LineChart>
        </ResponsiveContainer>
      </div>
      
      <div className="chart-container">
        <h2>Humidity Readings</h2>
        <ResponsiveContainer width="100%" height={300}>
          <LineChart data={data}>
            <CartesianGrid strokeDasharray="3 3" />
            <XAxis dataKey="timestamp" />
            <YAxis />
            <Tooltip />
            <Legend />
            <Line type="monotone" dataKey="readings.humidity" stroke="#82ca9d" name="Humidity" />
          </LineChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
}

export default Dashboard; 