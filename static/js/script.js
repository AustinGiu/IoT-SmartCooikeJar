function updateSnackStatus() {
    $.ajax({
        url: '/api/today_status',
        method: 'GET',
        success: function (data) {
            const weight = data.latest_weight || 0;
            const intake = data.today_total_intake || 0;
            const dailyLimit = 5;
            const remaining = Math.max(dailyLimit - intake, 0);

            $('#current-weight').text(weight.toFixed(1));
            $('#pieces-consumed').text(intake);
            $('#remaining-allowance').text(remaining);

            const lockStatus = data.lock_status;
            const lockUntil = data.lock_until ? new Date(data.lock_until) : null;

            // Handle locking status display
            if (lockStatus === "LOCK") {
                $('#status-message')
                    .text('Locked')
                    .css('color', 'red');

                const updateCountdown = () => {
                    const current = new Date();
                    const target = lockUntil || new Date(current.setHours(24, 0, 0, 0)); // punishment or midnight
                    const diff = target - new Date();

                    if (diff > 0) {
                        const hours = Math.floor(diff / (1000 * 60 * 60));
                        const minutes = Math.floor((diff % (1000 * 60 * 60)) / (1000 * 60));
                        const seconds = Math.floor((diff % (1000 * 60)) / 1000);
                        $('#unlock-timer').text(`${hours}h ${minutes}m ${seconds}s remaining to unlock`);
                    } else {
                        $('#unlock-timer').text('Lid should now be unlocked!');
                        clearInterval(window.timerInterval);
                    }
                };

                updateCountdown(); // Call once
                clearInterval(window.timerInterval);
                window.timerInterval = setInterval(updateCountdown, 1000);

            } else {
                $('#status-message')
                    .text('Open')
                    .css('color', 'green');
                $('#unlock-timer').text('');
                clearInterval(window.timerInterval);
            }

            // Refill check
            if (weight < 150) {
                $('#refill').text('Yes').css('color', 'red');
            } else {
                $('#refill').text('No').css('color', 'green');
            }

            // Nutrition per cookie
            const nutritionPerCookie = {
                energy: 188,
                protein: 0.43,
                fat: 1.87,
                carbohydrate: 6.47,
                sugar: 3.5,
                sodium: 47
            };

            const totalNutrition = {
                energy: nutritionPerCookie.energy * intake,
                protein: nutritionPerCookie.protein * intake,
                fat: nutritionPerCookie.fat * intake,
                carbohydrate: nutritionPerCookie.carbohydrate * intake,
                sugar: nutritionPerCookie.sugar * intake,
                sodium: nutritionPerCookie.sodium * intake
            };

            const tableBody = $('#log-table');
            tableBody.empty();

            const row = `
                <tr>
                    <td>${totalNutrition.energy.toFixed(1)}</td>
                    <td>${totalNutrition.protein.toFixed(1)}</td>
                    <td>${totalNutrition.fat.toFixed(1)}</td>
                    <td>${totalNutrition.carbohydrate.toFixed(1)}</td>
                    <td>${totalNutrition.sugar.toFixed(1)}</td>
                    <td>${totalNutrition.sodium.toFixed(1)}</td>
                </tr>
            `;

            tableBody.append(row);
        },
        error: function (error) {
            console.error('Failed to fetch snack status:', error);
        }
    });
}

// Run on page load
$(document).ready(function () {
    updateSnackStatus(); // Initial fetch
    setInterval(updateSnackStatus, 5000); // Fetch every 5 seconds
});
