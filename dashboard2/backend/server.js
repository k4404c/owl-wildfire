// // server.js
// const WebSocket = require('ws');
// const wss = new WebSocket.Server({ port: 8080 });

// At the top of your file
console.log("Starting server...");

// // wss.on('connection', (ws) => {
// //   console.log('Client connected');

// //   // Send mock data to the client every second
// //   setInterval(() => {
// //     const data = {
// //       summary: {
// //         totalDevices: 1000,
// //         activeDevices: Math.floor(800 + Math.random() * 200), // Random active devices
// //         dataPoints: Math.floor(Math.random() * 1000000), // Random data points
// //         alerts: Math.floor(Math.random() * 10), // Random alerts
// //       },
// //       activity: Array.from({ length: 7 }, (_, i) => ({
// //         date: `2023-06-${i + 1}`,
// //         activity: Math.floor(Math.random() * 10000),
// //       })),
// //       comparison: [
// //         { device: 'Device A', dataUsage: Math.floor(Math.random() * 5000) },
// //         { device: 'Device B', dataUsage: Math.floor(Math.random() * 5000) },
// //         { device: 'Device C', dataUsage: Math.floor(Math.random() * 5000) },
// //       ],
// //       distribution: [
// //         { name: 'Temperature', value: Math.floor(Math.random() * 500) },
// //         { name: 'Humidity', value: Math.floor(Math.random() * 500) },
// //       ],
// //     };
// //     ws.send(JSON.stringify(data));
// //   }, 1000);
// // // });

// // console.log('WebSocket server running on ws://localhost:8080');

const WebSocket = require('ws');
const axios = require('axios');
const express = require('express');
const app = express();

app.use(express.json());

const wss = new WebSocket.Server({ port: 8081 });
var token = "";
wss.on('connection', (ws) => {
    console.log('Client connected');

    const startTimestamp = Math.floor(new Date('2023-01-01').getTime() / 1000);// Example start date
    async function gettoken(){
        try {
            // Step 1: Authenticate and get the token
            const authResponse = await fetch("https://beta.owldms.com/owl/api/userdata/authenticate", 
                {
                    method: 'POST',
                    body: JSON.stringify(
                        {
                            "Username": "smallaj1@asu.edu",
                            "Password": "W2AB#6~5$5E+aCu",
                        }
                    ),
                    headers: {
                        'Content-type': 'application/json; charset=UTF-8'
                    }
                }
            );
            const data = await authResponse.json();
            token = data.token;
            console.log("token: ", data.token);
        }
        catch
        {
            console.log('wtf');
        }
    }

   
        gettoken()



    const fetchData = async () => {
        const endTimestamp = Math.floor(Date.now() / 1000); // Current time as Unix timestamp
        const apiUrl = 'https://beta.owldms.com/owl/api/userdata'; // Example API URL
        const papaId = "ASUPAPA2"

        try {
        const response = await fetch(
            `${apiUrl}/getRawData?start=${startTimestamp}&end=${endTimestamp}&papaId=ASUPAPA2`,
            {
                method: 'GET',
                headers: {
                    
                    'Authorization': `Bearer ${token}`,
                },
            }
        );

        if (!response.ok) {
            throw new Error(`Error: ${response.status} ${response.statusText}`);
        }

        const data = await response.json();
        console.log('Fetched Data:', data);

        // Process the raw data as needed
        data.forEach((item) => {
            console.log('Payload:', JSON.parse(item.payload));
        });
    } catch (error) {
        console.error('Error fetching data:', error.message);
    }
    };

    // Start streaming data
    fetchData();

    
    ws.on('close', () => console.log('Client disconnected'));
});

// Before server starts
console.log("About to start server...");

// After server starts
console.log("Server started successfully on port 8081");

// If you're using Express or similar:
app.listen(3010, () => {
  console.log(`Server running on port 3010`);
}).on('error', (err) => {
  console.error('Server failed to start:', err);
});



