function updateSnackStatus() {
    $.ajax({
        url: '/api/today_status',
        method: 'GET',
        success: function (data) {
            const weight = data.latest_weight || 0;
            const intake = data.today_total_intake || 0;
            const dailyLimit = 5;
            const remaining = Math.max(dailyLimit - intake, 0); // Avoid negative numbers

            // Update the page elements with the fetched data
            $('#current-weight').text(weight.toFixed(1));
            $('#pieces-consumed').text(intake);
            $('#remaining-allowance').text(remaining);
            // the lid lock status
            if (remaining === 0) {
                $('#status-message')
                    .text('Locked')
                    .css('color', 'red');
            } else {
                $('#status-message')
                    .text('Open')
                    .css('color', 'green');
            }


            // Nutrition data per cookie (could be changed into other brands of cookies)
            const nutritionPerCookie = {
                energy: 188,
                protein: 0.43,
                fat: 1.87,
                carbohydrate: 6.47,
                sugar: 3.5,
                sodium: 47
            };

            // Calculate total nutrition
            const totalNutrition = {
                energy: nutritionPerCookie.energy * intake,
                protein: nutritionPerCookie.protein * intake,
                fat: nutritionPerCookie.fat * intake,
                carbohydrate: nutritionPerCookie.carbohydrate * intake,
                sugar: nutritionPerCookie.sugar * intake,
                sodium: nutritionPerCookie.sodium * intake
            };

            // Update nutrition table
            const tableBody = $('#log-table');
            tableBody.empty(); // Clear previous row

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