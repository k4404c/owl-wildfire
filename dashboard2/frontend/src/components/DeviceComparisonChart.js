import React from 'react';
import { BarChart, Bar, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer } from 'recharts';

function DeviceComparisonChart({ data }) {
  return (
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
  );
}

export default DeviceComparisonChart;