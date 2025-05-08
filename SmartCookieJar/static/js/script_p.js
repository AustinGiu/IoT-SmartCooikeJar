$(document).ready(function () {
    const nutritionPerCookie = {
        energy: 188,
        protein: 0.43,
        fat: 1.87,
        carbohydrate: 6.47,
        sugar: 3.5,
        sodium: 47
    };

    function updateUsageStats() {
        $.ajax({
            url: '/api/usage_summary',
            method: 'GET',
            success: function (data) {
                const totalDays = data.days_used;
                const totalCookies = data.total_cookies;
                const daysWith5 = data.days_with_5_cookies;
                const daysWith0 = data.days_with_0_cookies;

                $('#days-counted').text(totalDays);
                $('#pieces-consumed').text(totalCookies);

                const totalCalories = totalCookies * nutritionPerCookie.energy;
                const caloriesMissed = (5 * totalDays - totalCookies) * nutritionPerCookie.energy;
                const avgDailyIntake = (totalCookies / totalDays).toFixed(2);

                $('#log-table').html(`
                    <tr>
                        <td>${totalCalories}</td>
                        <td>${caloriesMissed}</td>
                        <td>${avgDailyIntake}</td>
                        <td>${daysWith5}</td>
                        <td>${daysWith0}</td>
                    </tr>
                `);
            },
            error: function (err) {
                console.error('Failed to load usage stats:', err);
            }
        });
    }

    function fetchIntakeChartData() {
        $.ajax({
            url: '/api/7_day_intake',
            method: 'GET',
            success: function (data) {
                const intakeData = data.daily_intakes || [];

                const labels = intakeData.map(entry => entry.date);
                const values = intakeData.map(entry => entry.cookies);

                const ctx = document.getElementById('intakeChart').getContext('2d');

                // Destroy existing chart if needed
                if (window.intakeChart && typeof window.intakeChart.destroy === 'function') {
                    window.intakeChart.destroy();
                }

                // Create chart
                window.intakeChart = new Chart(ctx, {
                    type: 'line',
                    data: {
                        labels: labels.reverse(),
                        datasets: [{
                            label: 'Cookies Eaten',
                            data: values.reverse(),
                            fill: false,
                            borderColor: 'rgba(75, 192, 192, 1)',
                            tension: 0.1
                        }]
                    },
                    options: {
                        responsive: true,
                        scales: {
                            y: {
                                beginAtZero: true,
                                stepSize: 1
                            }
                        }
                    }
                });
            },
            error: function (err) {
                console.error('Error fetching intake data:', err);
            }
        });
    }


    // Initial calls
    updateUsageStats();
    fetchIntakeChartData();

    // Auto refresh both every 5 seconds
    setInterval(() => {
        updateUsageStats();
        fetchIntakeChartData();
    }, 5000);
});
