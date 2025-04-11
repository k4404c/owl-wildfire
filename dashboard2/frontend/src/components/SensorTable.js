import React from 'react';
import { Table, TableBody, TableCell, TableHead, TableHeaderCell, TableRow } from '@tremor/react';
import './SensorTable.css';

function SensorTable({ data }) {
  return (
    <div className="sensor-table-container">
      <table className="sensor-table">
        <TableHead>
          <TableRow>
            <TableHeaderCell>Timestamp</TableHeaderCell>
            <TableHeaderCell>Device ID</TableHeaderCell>
            <TableHeaderCell>Message ID</TableHeaderCell>
            <TableHeaderCell>Temperature (°C)</TableHeaderCell>
            <TableHeaderCell>Pressure (hPa)</TableHeaderCell>
            <TableHeaderCell>Duck Type</TableHeaderCell>
            <TableHeaderCell>Hops</TableHeaderCell>
            <TableHeaderCell>Path</TableHeaderCell>
          </TableRow>
        </TableHead>
        <TableBody>
          {data.map((item, index) => (
            <TableRow key={index}>
              <TableCell>{item.timestamp}</TableCell>
              <TableCell>{item.deviceId}</TableCell>
              <TableCell>{item.messageId}</TableCell>
              <TableCell>{item.temperature.toFixed(2)}</TableCell>
              <TableCell>{item.pressure.toFixed(2)}</TableCell>
              <TableCell>{item.duckType}</TableCell>
              <TableCell>{item.hops}</TableCell>
              <TableCell>{item.path || '-'}</TableCell>
            </TableRow>
          ))}
        </TableBody>
      </table>
    </div>
  );
}

export default SensorTable;