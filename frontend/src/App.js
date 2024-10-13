import React, { useState, useEffect } from 'react';
import DeviceSummary from './components/DeviceSummary';
import ActivityChart from './components/ActivityChart';
import DeviceComparisonChart from './components/DeviceComparisonChart';
import DataDistributionChart from './components/DataDistributionChart';
import './App.css';

function App() {
  const [data, setData] = useState(null);

  useEffect(() => {
    const ws = new WebSocket('ws://localhost:8080');

    ws.onmessage = (event) => {
      const newData = JSON.parse(event.data);
      setData(newData);
    };

    ws.onerror = (error) => console.error('WebSocket error:', error);
    ws.onclose = () => console.log('WebSocket connection closed');

    return () => ws.close();
  }, []);

  if (!data) return <div className="loading">Loading...</div>;

  return (
    <div className="App">
      <header className="App-header">
        <h1>OWL Inc. Ducklinks Dashboard</h1>
      </header>
      <main className="App-main">
        <div className="dashboard-grid">
          <DeviceSummary data={data.summary} />
          <div className="dashboard-card">
            <h2>Device Activity</h2>
            <p>Activity over the last 7 days</p>
            <ActivityChart data={data.activity} />
          </div>
          <div className="dashboard-card">
            <h2>Device Comparison</h2>
            <p>Data usage across devices</p>
            <DeviceComparisonChart data={data.comparison} />
          </div>
          <div className="dashboard-card">
            <h2>Data Distribution</h2>
            <p>Types of data collected</p>
            <DataDistributionChart data={data.distribution} />
          </div>
        </div>
      </main>
    </div>
  );
}

export default App;
