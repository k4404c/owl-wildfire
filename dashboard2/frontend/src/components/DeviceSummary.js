import React from 'react';

function DeviceSummary({ data }) {
  return (
    <div className="device-summary">
      <div className="summary-card">
        <h3>Total Devices</h3>
        <p className="summary-value">{data.totalDevices}</p>
      </div>
      <div className="summary-card">
        <h3>Active Devices</h3>
        <p className="summary-value">{data.activeDevices}</p>
      </div>
      <div className="summary-card">
        <h3>Data Points</h3>
        <p className="summary-value">{data.dataPoints}</p>
      </div>
      <div className="summary-card">
        <h3>Alerts</h3>
        <p className="summary-value">{data.alerts}</p>
      </div>
    </div>
  );
}

export default DeviceSummary;