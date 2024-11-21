var updateChart = undefined;
var maxDist = 4096;

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
        if (toggleDistance.checked || !message.startsWith('distance:')) {
            addToTable(message, timestamp);
        }

        // Always update the chart for distance messages
        if (message.startsWith('distance:')) {
            const distance = parseInt(message.split(':')[1].trim().replace('[mm]', ''));
            if (distance > maxDist || distance < 0) return;
            if (typeof updateChart === "function") {
                updateChart(timestamp, distance);
            } else {
                console.log(typeof updateChart);
            }
        }
    });

    // Handle reset event
    socket.on('reset', () => {
        // Clear table
        while (tableBody.firstChild) {
            tableBody.removeChild(tableBody.firstChild);
        }

        // Clear chart
        const chartData = distanceChart.data;
        chartData.labels = [];
        chartData.datasets[0].data = [];
        distanceChart.update();
    });

    // Chart.js setup
    const ctx = document.getElementById('distance-chart').getContext('2d');
    const distanceChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: [], // Timestamps
            datasets: [{
                label: 'Distance (mm)',
                data: [], // Distances
                borderColor: 'rgba(75, 192, 192, 1)',
                borderWidth: 2,
                fill: false
            }]
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

    // Update chart dynamically
    var updateChart = (timestamp, distance) => {
        const chartData = distanceChart.data;
        chartData.labels.push(timestamp);
        chartData.datasets[0].data.push(distance);

        // Keep only the last 50 points
        if (chartData.labels.length > 50) {
            chartData.labels.shift();
            chartData.datasets[0].data.shift();
        }

        // Sort timestamps to ensure proper plotting
        const sortedData = chartData.labels
            .map((label, index) => ({ label, value: chartData.datasets[0].data[index] }))
            .sort((a, b) => new Date(a.label) - new Date(b.label));
        chartData.labels = sortedData.map(d => d.label);
        chartData.datasets[0].data = sortedData.map(d => d.value);

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
    function updateTable(messages, showDistance) {
        // Clear the table
        while (tableBody.firstChild) {
            tableBody.removeChild(tableBody.firstChild);
        }
    
        // Filter and sort messages by timestamp in descending order
        const sortedMessages = messages
            .filter(({ message }) => showDistance || !message.startsWith('distance:')) // Respect toggle
            .sort((a, b) => new Date(a.timestamp) - new Date(b.timestamp)); // Descending order
    
        // Populate the table with sorted messages
        sortedMessages.forEach(({ message, timestamp }) => {
            addToTable(message, timestamp);
        });
    }

    // Update the chart with all distance messages
    function updateChartWithMessages(messages) {
        const chartData = distanceChart.data;

        // Clear existing data
        chartData.labels = [];
        chartData.datasets[0].data = [];

        // Populate the chart
        messages.forEach(({ message, timestamp }) => {
            if (message.startsWith('distance:')) {
                const distance = parseInt(message.split(':')[1].trim().replace('[mm]', ''));
                if (distance > maxDist || distance < 0) return;
                chartData.labels.push(timestamp);
                chartData.datasets[0].data.push(distance);
            }
        });

        // Sort timestamps for proper plotting
        const sortedData = chartData.labels
            .map((label, index) => ({ label, value: chartData.datasets[0].data[index] }))
            .sort((a, b) => new Date(a.label) - new Date(b.label));
        chartData.labels = sortedData.map(d => d.label);
        chartData.datasets[0].data = sortedData.map(d => d.value);

        distanceChart.update();
    }
});
