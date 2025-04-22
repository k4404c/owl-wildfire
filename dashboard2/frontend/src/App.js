import React, { useState, useEffect, useMemo } from 'react';
import {
  LineChart,
  Line,
  CartesianGrid,
  XAxis,
  YAxis,
  Tooltip,
  ResponsiveContainer,
} from 'recharts';
import { MapContainer, TileLayer, Marker, Popup } from 'react-leaflet';
import L from 'leaflet';
import 'leaflet/dist/leaflet.css';
import SensorTable from './components/SensorTable';
import { sendFireAlert } from './utils/fireAlert';
import Header from './components/Header';

// Green Icon (Prediction 0)
const greenIcon = new L.Icon({
  iconUrl: 'https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-2x-green.png',
  shadowUrl: 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/0.7.7/images/marker-shadow.png',
  iconSize: [25, 41],
  iconAnchor: [12, 41],
  popupAnchor: [1, -34],
  shadowSize: [41, 41]
});

// Red Icon (Prediction 1)
const redIcon = new L.Icon({
  iconUrl: 'https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-2x-red.png',
  shadowUrl: 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/0.7.7/images/marker-shadow.png',
  iconSize: [25, 41],
  iconAnchor: [12, 41],
  popupAnchor: [1, -34],
  shadowSize: [41, 41]
});

function App() {
  const [data, setData] = useState([]);
  const [selectedView, setSelectedView] = useState(1);
  const [error, setError] = useState(null);
  const [isLoading, setIsLoading] = useState(false);
  const [lastAlertTime, setLastAlertTime] = useState(null);
  const [selectedDuration, setSelectedDuration] = useState('1m');

  const fetchData = async () => {
    setIsLoading(true);
    setError(null);

    try {
      // Fetch data from our backend proxy
      const dataResponse = await fetch('http://localhost:3010/api/sensor-data');
      
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

  // Filter data based on selected duration
  const filteredData = useMemo(() => {
    const now = Date.now();
    let cutoffDate = 0;

    switch (selectedDuration) {
      case '1w':
        cutoffDate = now - 7 * 24 * 60 * 60 * 1000;
        break;
      case '1m':
        cutoffDate = now - 30 * 24 * 60 * 60 * 1000; // Approx 1 month
        break;
      case '6m':
        cutoffDate = now - 6 * 30 * 24 * 60 * 60 * 1000; // Approx 6 months
        break;
      case '1y':
        cutoffDate = now - 365 * 24 * 60 * 60 * 1000; // Approx 1 year
        break;
      default:
        cutoffDate = 0; // Or handle as 'All time' if needed
    }

    // Filter and sort (sorting might already be handled after fetch, but ensuring here)
    return data
      .filter(item => item.rawTimestamp.getTime() >= cutoffDate)
      .sort((a, b) => a.rawTimestamp - b.rawTimestamp); // Charts typically expect ascending order

  }, [data, selectedDuration]);

  useEffect(() => {
    fetchData();

    // Refresh every 5 minutes
    const intervalId = setInterval(fetchData, 5 * 60 * 1000);
    return () => clearInterval(intervalId);
  }, []);

  // --- Map Center Logic ---
  // Calculate a default center or based on data
  const defaultCenter = [33.42, -111.93]; // Default center (e.g., Tempe, AZ)
  const mapCenter = useMemo(() => {
    const validCoords = filteredData.filter(d => d.latitude && d.longitude);
    if (validCoords.length > 0) {
        // Use the latest data point with coordinates as the center
        return [validCoords[validCoords.length - 1].latitude, validCoords[validCoords.length - 1].longitude];
    }
    return defaultCenter;
  }, [filteredData]);
  // --- End Map Center Logic ---

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

            {/* Duration Selection Buttons */}
            <div className="duration-toggles">
              <button
                onClick={() => setSelectedDuration('1w')}
                className={`duration-button ${selectedDuration === '1w' ? 'active' : ''}`}
              >
                1 Week
              </button>
              <button
                onClick={() => setSelectedDuration('1m')}
                className={`duration-button ${selectedDuration === '1m' ? 'active' : ''}`}
              >
                1 Month
              </button>
              <button
                onClick={() => setSelectedDuration('6m')}
                className={`duration-button ${selectedDuration === '6m' ? 'active' : ''}`}
              >
                6 Months
              </button>
              <button
                onClick={() => setSelectedDuration('1y')}
                className={`duration-button ${selectedDuration === '1y' ? 'active' : ''}`}
              >
                1 Year
              </button>
            </div>

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
              {/* Overview Stats Cards - Use filteredData */}
              <div className="stats-grid">
                <div className="stat-card">
                  <h3>Temperature</h3>
                  {/* Display latest temp from filtered data */}
                  <p className="stat-value">{filteredData[filteredData.length - 1]?.temperature?.toFixed(2)}°C</p>
                </div>
                <div className="stat-card">
                  <h3>Pressure</h3>
                  {/* Display latest pressure from filtered data */}
                  <p className="stat-value">{filteredData[filteredData.length - 1]?.pressure?.toFixed(2)} hPa</p>
                </div>
                <div className="stat-card">
                  <h3>Total Readings (Period)</h3>
                  {/* Show count for the filtered period */}
                  <p className="stat-value">{filteredData.length}</p>
                </div>
              </div>

              {/* Map Container */}
              <div className="map-card">
                <h3>Sensor Locations</h3>
                <MapContainer center={mapCenter} zoom={13} style={{ height: '400px', width: '100%' }}>
                  <TileLayer
                    attribution='&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
                    url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
                  />
                  {filteredData.map((item, index) => {
                    // Ensure lat/lon exist before creating a marker
                    if (item.latitude && item.longitude) {
                      return (
                        <Marker
                          key={index} // Use a more stable key if possible (e.g., item.messageId)
                          position={[item.latitude, item.longitude]}
                          icon={item.prediction === 1 ? redIcon : greenIcon} // Choose icon based on prediction
                        >
                          <Popup>
                            Timestamp: {item.timestamp}<br />
                            Temp: {item.temperature}°C<br />
                            Pressure: {item.pressure} hPa<br />
                            Prediction: {item.prediction} ({item.prediction === 1 ? 'Fire Risk' : 'No Risk'})<br />
                            Coords: {item.latitude.toFixed(4)}, {item.longitude.toFixed(4)}
                          </Popup>
                        </Marker>
                      );
                    }
                    return null; // Don't render marker if no coords
                  })}
                </MapContainer>
              </div>

              {/* Charts - Use filteredData */}
              <div className="charts-grid">
                <div className="chart-card">
                  <h3>Temperature Over Time</h3>
                  <ResponsiveContainer width="100%" height={300}>
                    {/* Pass filteredData to the chart */}
                    <LineChart data={filteredData}>
                      <CartesianGrid strokeDasharray="3 3" />
                      <XAxis dataKey="timestamp" />
                      <YAxis />
                      <Tooltip />
                      <Line
                        type="monotone"
                        dataKey="temperature"
                        stroke="#ff7300"
                        strokeWidth={2}
                        dot={false} // Optional: hide dots for potentially many data points
                      />
                    </LineChart>
                  </ResponsiveContainer>
                </div>

                <div className="chart-card">
                  <h3>Pressure Over Time</h3>
                  <ResponsiveContainer width="100%" height={300}>
                    {/* Pass filteredData to the chart */}
                    <LineChart data={filteredData}>
                      <CartesianGrid strokeDasharray="3 3" />
                      <XAxis dataKey="timestamp" />
                      <YAxis domain={[75000, 100000]} />
                      <Tooltip />
                      <Line
                        type="monotone"
                        dataKey="pressure"
                        stroke="#387908"
                        strokeWidth={2}
                        dot={false} // Optional: hide dots
                      />
                    </LineChart>
                  </ResponsiveContainer>
                </div>
              </div>
            </div>
          ) : (
            <div className="dashboard-detail">
              <h2>Detailed Sensor Readings</h2>
              {/* Pass filteredData to the table */}
              <SensorTable data={filteredData} />
            </div>
          )}
        </div>
      </main>
    </div>
  );
}

export default App;
