import React, { useState, useEffect } from 'react';
import { LineChart, Line, CartesianGrid, XAxis, YAxis, Tooltip, Legend, ResponsiveContainer } from 'recharts';
import SensorTable from './components/SensorTable';
import { username,password } from './env.js';
import { sendFireAlert } from './utils/fireAlert';

const dataFormatter = (number) => {
  return Intl.NumberFormat('us').format(number).toString();
};

function App() {
  const [data, setData] = useState([]);
  const [selectedView, setSelectedView] = useState(1);
  const [error, setError] = useState(null);
  const [isLoading, setIsLoading] = useState(false);
  const [lastAlertTime, setLastAlertTime] = useState(null);

  const parsePayloadData = (payloadString) => {
    const temperatureMatch = payloadString.match(/Temp: ([\d.]+)/);
    const pressureMatch = payloadString.match(/Pressure: ([\d.]+)/);
    
   

    return {
      temperature: parseFloat(temperatureMatch[1]),
      pressure: parseFloat(pressureMatch[1])
    };
  };

  const fetchData = async () => {
    setIsLoading(true);
    setError(null);
    
    try {
      // First, get the authentication token
      const authResponse = await fetch('https://beta.owldms.com/owl/api/userdata/authenticate', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          Username: username,
          Password: password,
        }),
      });

      if (!authResponse.ok) {
        throw new Error('Authentication failed');
      }

      const { token } = await authResponse.json();

      // Then fetch the sensor data
      const startTimestamp = Math.floor(new Date('2025-03-01').getTime() / 1000);
      const endTimestamp = Math.floor(Date.now() / 1000);
      
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

      const rawData = await dataResponse.json();
      
      // Process the data with fixed parsing
      const processedData = rawData.map((item) => {
        try {
          const data = JSON.parse(item.payload);
          
          // Only process GPS events
          if (item.eventType !== "gps") {
            return null;
          }

          const cleanPayload = data.Payload.replace(/\s+/g, ' ').trim();
          console.log('Cleaned payload:', cleanPayload); // Debug log

          // Updated regex patterns to match the actual format
          const matches = {
            temperature: cleanPayload.match(/Temp:([-\d.]+)/),
            humidity: cleanPayload.match(/Hum:([-\d.]+)/),
            pressure: cleanPayload.match(/Press:([-\d.]+)/),
            altitude: cleanPayload.match(/Alt:([-\d.]+)/),
            gas: cleanPayload.match(/Gas:([-\d.]+)/),
            prediction: cleanPayload.match(/Pred:([-\d.]+)/),
            latitude: cleanPayload.match(/Lat:([-\d.]+)/),
            longitude: cleanPayload.match(/Lng:([-\d.]+)/)
          };

          const processedItem = {
            timestamp: new Date(item.createdAt).toLocaleString(),
            rawTimestamp: new Date(item.createdAt),
            deviceId: data.DeviceID,
            messageId: data.MessageID,
            temperature: matches.temperature ? parseFloat(matches.temperature[1]) : null,
            humidity: matches.humidity ? parseFloat(matches.humidity[1]) : null,
            pressure: matches.pressure ? parseFloat(matches.pressure[1]) : null,
            altitude: matches.altitude ? parseFloat(matches.altitude[1]) : null,
            gas: matches.gas ? parseFloat(matches.gas[1]) : null,
            prediction: matches.prediction ? parseInt(matches.prediction[1]) : null,
            latitude: matches.latitude ? parseFloat(matches.latitude[1]) : null,
            longitude: matches.longitude ? parseFloat(matches.longitude[1]) : null,
            duckType: data.duckType,
            hops: data.hops,
            path: data.path,
            rawPayload: data.Payload,
            eventType: item.eventType
          };

          // Debug log
          console.log('Processed item:', processedItem);

          // Only send alerts for new data points with prediction = 1
          const isNewDataPoint = processedItem.rawTimestamp > (Date.now() - 60000);
          const isFireRisk = processedItem.prediction === 1;
          
          if (isNewDataPoint && isFireRisk && 
              (!lastAlertTime || processedItem.rawTimestamp > lastAlertTime)) {
            sendFireAlert(processedItem);
            setLastAlertTime(processedItem.rawTimestamp);
            console.log('Fire alert triggered for new data point:', processedItem);
          }

          return processedItem;
        } catch (err) {
          console.warn('Error processing data point:', err, item.payload);
          return null;
        }
      }).filter(Boolean);

      if (processedData.length === 0) {
        setError('No valid data points found');
        return;
      }

      // Sort data by timestamp (newest first)
      processedData.sort((a, b) => new Date(b.timestamp) - new Date(a.timestamp));

      if (processedData.length > 0) {
        const latestFireRisk = processedData.find(item => item.prediction === 1);
        if (latestFireRisk) {
          console.log('Found fire risk data point:', latestFireRisk);
          sendFireAlert(latestFireRisk);
        } else {
          console.log('No data points with prediction = 1 found');
        }
      }
      setData(processedData);
    } catch (err) {
      setError(err.message);
      console.error('Error fetching data:', err);
    } finally {
      setIsLoading(false);
    }
  };

  useEffect(() => {
    fetchData();
    
    // Set up periodic refresh every 5 minutes
    const intervalId = setInterval(fetchData, 5 * 60 * 1000);
    
    return () => clearInterval(intervalId);
  }, []);

  const latestData = data[0] || { temperature: 0, pressure: 0 };

  return (
    <div className="p-6 bg-gray-100 min-h-screen">
      <h1 className="mb-4">Sensor Dashboard</h1>
      
      {isLoading && (
        <div className="bg-blue-100 border border-blue-400 text-blue-700 px-4 py-3 rounded relative mb-4" role="alert">
          <span className="block sm:inline">Loading data...</span>
        </div>
      )}
      
      {error && (
        <div className="bg-red-100 border border-red-400 text-red-700 px-4 py-3 rounded relative mb-4" role="alert">
          <strong className="font-bold">Error: </strong>
          <span className="block sm:inline">{error}</span>
        </div>
      )}
      
      <div className="mb-4">
        <button 
          onClick={fetchData}
          className="bg-blue-500 hover:bg-blue-700 text-white font-bold py-2 px-4 rounded"
        >
          Refresh Data
        </button>
      </div>

      <div>
        <button onClick={() => setSelectedView(0)}>Overview</button>
        <button onClick={() => setSelectedView(1)}>Detail</button>
      </div>

      {selectedView === 0 ? (
        <div className="mt-6">
          <div className="grid grid-cols-1 gap-6 sm:grid-cols-2 lg:grid-cols-3">
            <div className="p-4 border rounded shadow">
              <h2>Temperature</h2>
              <p className="text-3xl font-bold">{latestData.temperature.toFixed(2)}°C</p>
            </div>
            <div className="p-4 border rounded shadow">
              <h2>Pressure</h2>
              <p className="text-3xl font-bold">{latestData.pressure.toFixed(2)} hPa</p>
            </div>
            <div className="p-4 border rounded shadow">
              <h2>Total Readings</h2>
              <p className="text-3xl font-bold">{data.length}</p>
            </div>
          </div>
          <div className="mt-6 grid grid-cols-1 gap-6">
            <div className="p-4 border rounded shadow">
              <h2>Temperature Over Time</h2>
              <ResponsiveContainer width="100%" height={300}>
                <LineChart data={data}>
                  <CartesianGrid strokeDasharray="3 3" />
                  <XAxis dataKey="timestamp" />
                  <YAxis />
                  <Tooltip />
                  <Line type="monotone" dataKey="temperature" stroke="#ff7300" />
                </LineChart>
              </ResponsiveContainer>
            </div>
            <div className="p-4 border rounded shadow">
              <h2>Pressure Over Time</h2>
              <ResponsiveContainer width="100%" height={300}>
                <LineChart data={data}>
                  <CartesianGrid strokeDasharray="3 3" />
                  <XAxis dataKey="timestamp" />
                  <YAxis />
                  <Tooltip />
                  <Line type="monotone" dataKey="pressure" stroke="#387908" />
                </LineChart>
              </ResponsiveContainer>
            </div>
          </div>
        </div>
      ) : (
        <div className="mt-6">
          <h2>Detailed Sensor Readings</h2>
          <SensorTable data={data} />
        </div>
      )}
    </div>
  );
}

export default App;
