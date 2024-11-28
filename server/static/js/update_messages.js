var updateChart = undefined;
var maxDist = 4096;
var sensorCount = 16; // Number of sensors

document.addEventListener('DOMContentLoaded', () => {
    const socket = io();
    const tableBody = document.getElementById('messages-table').getElementsByTagName('tbody')[0];
    const toggleDistance = document.getElementById('toggle-distance');
    const maxTableRecords = 50; // Limit the table to 50 rows

    // Fetch and populate initial data
    fetchInitialData();

    // Handle toggle behavior for the table only
    toggleDistance.addEventListener('change', () => {
        fetchTableData(toggleDistance.checked);
    });

    // Listen for new messages
    socket.on('new_message', (data) => {
        const { message, timestamp } = data;

        // Update the table if the toggle allows it
        if (toggleDistance.checked || !message.startsWith('Distances[mm]:')) {
            addToTable(message, timestamp);
        }

        // Update the chart for distance messages
        if (message.startsWith('Distances[mm]:')) {
            const distances = parseDistances(message);
            if (distances) {
                updateChart(timestamp, distances);
            }
        }
    });

    // Handle reset event
    socket.on('reset', () => {
        // Clear the table
        while (tableBody.firstChild) {
            tableBody.removeChild(tableBody.firstChild);
        }

        // Clear the chart
        const chartData = distanceChart.data;
        chartData.labels = [];
        chartData.datasets.forEach(dataset => dataset.data = []);
        distanceChart.update();
    });

    // Chart.js setup for multiple lines
    const ctx = document.getElementById('distance-chart').getContext('2d');
    const distanceChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: [], // Timestamps
            datasets: Array.from({ length: sensorCount }, (_, i) => ({
                label: `Sensor ${i + 1}`,
                data: [], // Distances for each sensor
                borderColor: `hsl(${(i / sensorCount) * 360}, 70%, 50%)`, // Distinct colors
                borderWidth: 2,
                fill: false
            }))
        },
        options: {
            aspectRatio: 5 / 4,
            scales: {
                x: {
                    type: 'time', // Enable time scale
                    time: {
                        parser: 'YYYY-MM-DD HH:mm:ss', // Match your timestamp format
                        tooltipFormat: 'MMM DD, YYYY HH:mm:ss', // Tooltip format
                        unit: 'second' // Unit of measurement
                    },
                    title: {
                        display: true,
                        text: 'Time'
                    }
                },
                y: {
                    title: {
                        display: true,
                        text: 'Distance (mm)'
                    }
                }
            }
        }
    });

    // Parse distances from the message
    function parseDistances(message) {
        const match = message.match(/Distances\[mm\]:\s*([\d,\s]+)/);
        if (!match) return null;

        const distances = match[1].split(',').map(d => parseInt(d.trim(), 10));
        if (distances.length !== sensorCount || distances.some(d => d < 0 || d > maxDist)) {
            return null; // Ignore invalid or out-of-range data
        }

        return distances;
    }

    // Update chart dynamically for all sensors
    var updateChart = (timestamp, distances) => {
        const chartData = distanceChart.data;

        // Update the labels
        chartData.labels.push(timestamp);

        // Update data for each sensor
        distances.forEach((distance, i) => {
            chartData.datasets[i].data.push(distance);
        });

        // Keep only the last 50 points
        if (chartData.labels.length > 50) {
            chartData.labels.shift();
            chartData.datasets.forEach(dataset => dataset.data.shift());
        }

        distanceChart.update();
    };

    // Fetch initial data for both table and chart
    async function fetchInitialData() {
        try {
            const response = await fetch(`/fetch_messages?show_distance=true`);
            const data = await response.json();

            // Populate the table based on the toggle state
            updateTable(data.messages, toggleDistance.checked);

            // Populate the chart with all distance messages
            updateChartWithMessages(data.messages);
        } catch (error) {
            console.error("Error fetching initial data:", error);
        }
    }

    // Fetch only table data when the toggle changes
    async function fetchTableData(showDistance) {
        try {
            const response = await fetch(`/fetch_messages?show_distance=${showDistance}`);
            const data = await response.json();

            // Populate the table
            updateTable(data.messages, showDistance);
        } catch (error) {
            console.error("Error fetching table data:", error);
        }
    }

    // Add a new row to the table and enforce max row limit
    function addToTable(message, timestamp) {
        const newRow = tableBody.insertRow(0);
        const cell1 = newRow.insertCell(0);
        const cell2 = newRow.insertCell(1);
        cell1.textContent = message;
        cell2.textContent = timestamp;

        // Remove excess rows if table exceeds max records
        while (tableBody.rows.length > maxTableRecords) {
            tableBody.deleteRow(tableBody.rows.length - 1);
        }
    }

    // Update the table with messages
    var updateChart = (timestamp, distances) => {
        const chartData = distanceChart.data;
    
        // Add new timestamp
        chartData.labels.push(timestamp);
    
        // Add data for each sensor
        distances.forEach((distance, i) => {
            chartData.datasets[i].data.push(distance);
        });
    
        // Keep only the last 50 points
        if (chartData.labels.length > 50) {
            chartData.labels.shift();
            chartData.datasets.forEach(dataset => dataset.data.shift());
        }
    
        distanceChart.update();
    };
    

    // Update the chart with all distance messages
    function updateChartWithMessages(messages) {
        const chartData = distanceChart.data;
    
        // Clear existing data to prevent connections between old and new points
        chartData.labels = [];
        chartData.datasets.forEach(dataset => (dataset.data = []));
    
        // Populate the chart with new data
        messages.forEach(({ message, timestamp }) => {
            if (message.startsWith('Distances[mm]:')) {
                const distances = parseDistances(message);
                if (distances) {
                    chartData.labels.push(timestamp); // Add timestamp once
                    distances.forEach((distance, i) => {
                        chartData.datasets[i].data.push(distance); // Add sensor data
                    });
                }
            }
        });
    
        // Sort timestamps to ensure proper plotting
        const sortedData = chartData.labels
            .map((label, index) => ({
                label,
                values: chartData.datasets.map(dataset => dataset.data[index])
            }))
            .sort((a, b) => new Date(a.label) - new Date(b.label)); // Sort by timestamp
    
        chartData.labels = sortedData.map(d => d.label);
        chartData.datasets.forEach((dataset, i) => {
            dataset.data = sortedData.map(d => d.values[i]);
        });
    
        distanceChart.update();
    }
    
});
