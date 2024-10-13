// Mock data for Ducklinks devices
const mockData = {
    summary: {
      totalDevices: 1000,
      activeDevices: 850,
      dataPoints: 1000000,
      alerts: 5,
    },
    activity: [
      { date: '2023-06-01', activity: 2400 },
      { date: '2023-06-02', activity: 1398 },
      { date: '2023-06-03', activity: 9800 },
      { date: '2023-06-04', activity: 3908 },
      { date: '2023-06-05', activity: 4800 },
      { date: '2023-06-06', activity: 3800 },
      { date: '2023-06-07', activity: 4300 },
    ],
    comparison: [
      { device: 'Device A', dataUsage: 4000 },
      { device: 'Device B', dataUsage: 3000 },
      { device: 'Device C', dataUsage: 2000 },
      { device: 'Device D', dataUsage: 2780 },
      { device: 'Device E', dataUsage: 1890 },
    ],
    distribution: [
      { name: 'Temperature', value: 400 },
      { name: 'Humidity', value: 300 },
      { name: 'Pressure', value: 300 },
      { name: 'Light', value: 200 },
    ],
  };
  
  export const fetchDucklinksData = async () => {
    // Simulate API call delay
    await new Promise(resolve => setTimeout(resolve, 1000));
    return mockData;
  };