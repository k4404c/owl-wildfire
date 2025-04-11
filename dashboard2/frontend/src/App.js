import React, { useState, useEffect } from 'react';
import {
  LineChart,
  Line,
  CartesianGrid,
  XAxis,
  YAxis,
  Tooltip,
  ResponsiveContainer,
} from 'recharts';
import SensorTable from './components/SensorTable';
import { username, password } from './env.js';
import { sendFireAlert } from './utils/fireAlert';
import Header from './components/Header';

function App() {
  const [data, setData] = useState([]);
  const [selectedView, setSelectedView] = useState(1);
  const [error, setError] = useState(null);
  const [isLoading, setIsLoading] = useState(false);
  const [lastAlertTime, setLastAlertTime] = useState(null);

  const fetchData = async () => {
    setIsLoading(true);
    setError(null);

    try {
      // 1. Get the authentication token
      const authResponse = await fetch(
        'https://beta.owldms.com/owl/api/userdata/authenticate',
        {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
          },
          body: JSON.stringify({
            Username: username,
            Password: password,
          }),
        }
      );

      if (!authResponse.ok) {
        throw new Error('Authentication failed');
      }

      const { token } = await authResponse.json();

      // 2. Fetch the sensor data
      const startTimestamp = Math.floor(new Date('2025-03-01').getTime() / 1000);
      const endTimestamp = Math.floor(Date.now() / 1000);

      const dataResponse = await fetch(
        `https://beta.owldms.com/owl/api/userdata/getRawData?start=${startTimestamp}&end=${endTimestamp}&papaId=ASUPAPA2`,
        {
          headers: {
            Authorization: `Bearer ${token}`,
          },
        }
      );

      if (!dataResponse.ok) {
        throw new Error('Failed to fetch sensor data');
      }

      const rawData = await dataResponse.json();

      // Process the data
      const processedData = rawData
        .map((item) => {
          try {
            const parsed = JSON.parse(item.payload);
            if (item.eventType !== 'gps') return null;

            const cleanPayload = parsed.Payload.replace(/\s+/g, ' ').trim();
            const matches = {
              temperature: cleanPayload.match(/Temp:([-\d.]+)/),
              humidity: cleanPayload.match(/Hum:([-\d.]+)/),
              pressure: cleanPayload.match(/Press:([-\d.]+)/),
              altitude: cleanPayload.match(/Alt:([-\d.]+)/),
              gas: cleanPayload.match(/Gas:([-\d.]+)/),
              prediction: cleanPayload.match(/Pred:([-\d.]+)/),
              latitude: cleanPayload.match(/Lat:([-\d.]+)/),
              longitude: cleanPayload.match(/Lng:([-\d.]+)/),
            };

            const processedItem = {
              timestamp: new Date(item.createdAt).toLocaleString(),
              rawTimestamp: new Date(item.createdAt),
              deviceId: parsed.DeviceID,
              messageId: parsed.MessageID,
              temperature: matches.temperature
                ? parseFloat(matches.temperature[1])
                : null,
              humidity: matches.humidity ? parseFloat(matches.humidity[1]) : null,
              pressure: matches.pressure ? parseFloat(matches.pressure[1]) : null,
              altitude: matches.altitude ? parseFloat(matches.altitude[1]) : null,
              gas: matches.gas ? parseFloat(matches.gas[1]) : null,
              prediction: matches.prediction ? parseInt(matches.prediction[1]) : null,
              latitude: matches.latitude ? parseFloat(matches.latitude[1]) : null,
              longitude: matches.longitude ? parseFloat(matches.longitude[1]) : null,
              duckType: parsed.duckType,
              hops: parsed.hops,
              path: parsed.path,
              rawPayload: parsed.Payload,
              eventType: item.eventType,
            };

            // Fire alert logic
            const isNewDataPoint =
              processedItem.rawTimestamp > Date.now() - 60 * 1000;
            const isFireRisk = processedItem.prediction === 1;

            if (
              isNewDataPoint &&
              isFireRisk &&
              (!lastAlertTime || processedItem.rawTimestamp > lastAlertTime)
            ) {
              sendFireAlert(processedItem);
              setLastAlertTime(processedItem.rawTimestamp);
            }

            return processedItem;
          } catch (err) {
            console.warn('Error processing data point:', err, item.payload);
            return null;
          }
        })
        .filter(Boolean);

      if (processedData.length === 0) {
        setError('No valid data points found');
      }

      // Sort by timestamp descending
      processedData.sort((a, b) => b.rawTimestamp - a.rawTimestamp);

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

    // Refresh every 5 minutes
    const intervalId = setInterval(fetchData, 5 * 60 * 1000);
    return () => clearInterval(intervalId);
  }, []);

  return (
    <div className="app">
      <Header />

      <main className="main-content">
        <div className="content-wrapper">
          {/* Status Messages */}
          {isLoading && (
            <div className="status-message loading">
              <div className="spinner"></div>
              <span>Loading data...</span>
            </div>
          )}

          {error && (
            <div className="status-message error">
              <span className="error-icon">⚠️</span>
              <span>{error}</span>
            </div>
          )}

          {/* Control Panel */}
          <div className="control-panel">
            <button onClick={fetchData} className="refresh-button">
              Refresh Data
            </button>

            <div className="view-toggles">
              <button
                onClick={() => setSelectedView(0)}
                className={`view-button ${selectedView === 0 ? 'active' : ''}`}
              >
                Overview
              </button>
              <button
                onClick={() => setSelectedView(1)}
                className={`view-button ${selectedView === 1 ? 'active' : ''}`}
              >
                Detail
              </button>
            </div>
          </div>

          {/* Dashboard Content */}
          {selectedView === 0 ? (
            <div className="dashboard-overview">
              {/* Updated “Overview” Stats Cards */}
              <div className="stats-grid">
  <div className="stat-card">
    <h3>Temperature</h3>
    <p className="stat-value">{data[0]?.temperature.toFixed(2)}°C</p>
  </div>
  <div className="stat-card">
    <h3>Pressure</h3>
    <p className="stat-value">{data[0]?.pressure.toFixed(2)} hPa</p>
  </div>
  <div className="stat-card">
    <h3>Total Readings</h3>
    <p className="stat-value">{data.length}</p>
  </div>
</div>

              {/* Charts */}
              <div className="charts-grid">
                <div className="chart-card">
                  <h3>Temperature Over Time</h3>
                  <ResponsiveContainer width="100%" height={300}>
                    <LineChart data={data}>
                      <CartesianGrid strokeDasharray="3 3" />
                      <XAxis dataKey="timestamp" />
                      <YAxis />
                      <Tooltip />
                      <Line
                        type="monotone"
                        dataKey="temperature"
                        stroke="#ff7300"
                        strokeWidth={2}
                      />
                    </LineChart>
                  </ResponsiveContainer>
                </div>

                <div className="chart-card">
                  <h3>Pressure Over Time</h3>
                  <ResponsiveContainer width="100%" height={300}>
                    <LineChart data={data}>
                      <CartesianGrid strokeDasharray="3 3" />
                      <XAxis dataKey="timestamp" />
                      <YAxis />
                      <Tooltip />
                      <Line
                        type="monotone"
                        dataKey="pressure"
                        stroke="#387908"
                        strokeWidth={2}
                      />
                    </LineChart>
                  </ResponsiveContainer>
                </div>
              </div>
            </div>
          ) : (
            <div className="dashboard-detail">
              <h2>Detailed Sensor Readings</h2>
              <SensorTable data={data} />
            </div>
          )}
        </div>
      </main>
    </div>
  );
}

export default App;
