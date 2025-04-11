import React from 'react';
import { BarChart, Bar, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer } from 'recharts';
import './DeviceComparisonChart.css';

function DeviceComparisonChart({ data }) {
  return (
    <div className="comparison-chart">
      <ResponsiveContainer width="100%" height={300}>
        <BarChart data={data}>
          <CartesianGrid strokeDasharray="3 3" />
          <XAxis dataKey="device" />
          <YAxis />
          <Tooltip />
          <Legend />
          <Bar dataKey="dataUsage" fill="#82ca9d" name="Data Usage" />
        </BarChart>
      </ResponsiveContainer>
    </div>
  );
}

export default DeviceComparisonChart;