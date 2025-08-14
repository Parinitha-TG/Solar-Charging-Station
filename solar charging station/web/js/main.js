// DOM Elements
const hoursInput = document.getElementById('hours');
const minutesInput = document.getElementById('minutes');
const secondsInput = document.getElementById('seconds');
const startButton = document.getElementById('startButton');
const timerDisplay = document.getElementById('timer');
const chargingStatus = document.getElementById('chargingStatus');
const connectionStatus = document.getElementById('connectionStatus');
const statusAlert = document.querySelector('.charging-status');
const proceedToPayment = document.getElementById('proceedToPayment');
const paymentSection = document.getElementById('paymentSection');
const timeSelection = document.getElementById('timeSelection');
const chargingSection = document.getElementById('chargingSection');
const verifyPayment = document.getElementById('verifyPayment');
const paymentAmount = document.getElementById('paymentAmount');
const qrCode = document.getElementById('qrCode');

let timerInterval = null;
let endTime = null;
let selectedDuration = 0;
let espControlledSession = false; // Track if we're in an ESP-controlled session

// Constants
const RATE_PER_HOUR = 20; // ₹20 per hour

// Update connection status when Firebase connection changes
const connectedRef = database.ref('.info/connected');
connectedRef.on('value', (snap) => {
    if (snap.val() === true) {
        connectionStatus.textContent = 'Connected';
        connectionStatus.className = 'connected';
    } else {
        connectionStatus.textContent = 'Disconnected';
        connectionStatus.className = 'disconnected';
    }
});

// Listen for charging status changes
chargingRef.on('value', (snapshot) => {
    const data = snapshot.val() || {};
    console.log("Firebase data updated:", data);
    
    // If we're already managing a charging session locally, don't let Firebase status 
    // changes interfere unless charging is explicitly set to false by ESP32
    if (espControlledSession && data.charging === false) {
        console.log("ESP32 has ended charging session");
        espControlledSession = false;
        stopTimer();
        
        // Show time selection when done
        timeSelection.style.display = 'block';
        paymentSection.style.display = 'none';
        chargingSection.style.display = 'none';
        statusAlert.textContent = 'Charging completed by ESP32';
        statusAlert.className = 'charging-status alert alert-success text-center mb-4';
        startButton.disabled = false;
        chargingStatus.textContent = 'No';
        chargingStatus.className = '';
        return;
    }
    
    if (data.charging) {
        chargingStatus.textContent = 'Yes';
        chargingStatus.className = 'active';
        
        // Show charging section when charging is active
        timeSelection.style.display = 'none';
        paymentSection.style.display = 'none';
        chargingSection.style.display = 'block';
        
        // Disable start button while charging
        startButton.disabled = true;
        
        if (data.startTime && data.duration) {
            statusAlert.textContent = `Charging in progress for ${formatTime(data.duration)}`;
            statusAlert.className = 'charging-status alert alert-success text-center mb-4';
            
            // Always make timer visible when charging
            timerDisplay.style.display = 'block';
            
            // Update timer display
            updateTimerDisplay(data.startTime * 1000, data.duration);
        }
    } else {
        chargingStatus.textContent = 'No';
        chargingStatus.className = '';
        
        // Don't stop timer if we're in ESP-controlled session
        if (!espControlledSession) {
            stopTimer();
            
            // If not in payment flow, show time selection
            if (paymentSection.style.display !== 'block') {
                timeSelection.style.display = 'block';
                chargingSection.style.display = 'none';
                statusAlert.textContent = 'Ready to charge';
                statusAlert.className = 'charging-status alert alert-info text-center mb-4';
            }
            
            // Re-enable start button
            startButton.disabled = false;
        }
    }
});

// Convert time inputs to seconds
function getSelectedDuration() {
    const hours = parseInt(hoursInput.value) || 0;
    const minutes = parseInt(minutesInput.value) || 0;
    const seconds = parseInt(secondsInput.value) || 0;
    return (hours * 3600) + (minutes * 60) + seconds;
}

// Calculate payment amount
function calculatePaymentAmount(durationInSeconds) {
    const hours = durationInSeconds / 3600;
    return Math.ceil(hours * RATE_PER_HOUR);
}

// Generate UPI QR Code
function generateUPIQRCode(amount) {
    const upiId = "your-upi-id@upi"; // Replace with your UPI ID
    const merchantName = "Solar Charging Station";
    const transactionNote = "Charging payment";
    
    const upiUrl = `upi://pay?pa=${upiId}&pn=${encodeURIComponent(merchantName)}&am=${amount}&tn=${encodeURIComponent(transactionNote)}`;
    
    const qr = qrcode(0, 'M');
    qr.addData(upiUrl);
    qr.make();
    
    return qr.createImgTag(5);
}

// Format time for display
function formatTime(timeInSeconds) {
    const hours = Math.floor(timeInSeconds / 3600);
    const minutes = Math.floor((timeInSeconds % 3600) / 60);
    const seconds = timeInSeconds % 60;
    return `${String(hours).padStart(2, '0')}:${String(minutes).padStart(2, '0')}:${String(seconds).padStart(2, '0')}`;
}

// Proceed to payment button handler
proceedToPayment.addEventListener('click', () => {
    selectedDuration = getSelectedDuration();
    
    if (selectedDuration <= 0) {
        statusAlert.textContent = 'Please select a valid charging duration';
        statusAlert.className = 'charging-status alert alert-danger text-center mb-4';
        return;
    }

    const amount = calculatePaymentAmount(selectedDuration);
    paymentAmount.textContent = amount;
    qrCode.innerHTML = generateUPIQRCode(amount);
    
    timeSelection.style.display = 'none';
    paymentSection.style.display = 'block';
    statusAlert.textContent = 'Please complete the payment';
    statusAlert.className = 'charging-status alert alert-info text-center mb-4';
});

// Payment verification button handler
verifyPayment.addEventListener('click', () => {
    // In a real implementation, you would verify the payment status here
    // For this prototype, we'll assume payment is successful
    paymentSection.style.display = 'none';
    chargingSection.style.display = 'block';
    statusAlert.textContent = 'Payment successful! You can start charging now.';
    statusAlert.className = 'charging-status alert alert-success text-center mb-4';
    startButton.disabled = false;
});

// Update timer display
function updateTimerDisplay(startTime, duration) {
    // Clear any existing timer interval
    if (timerInterval) {
        clearInterval(timerInterval);
        timerInterval = null;
    }
    
    const now = Date.now();
    const end = startTime + (duration * 1000);
    const timeLeft = Math.max(0, Math.floor((end - now) / 1000));

    console.log("Updating timer display:", { startTime, duration, timeLeft });
    
    // Always ensure timer is visible
    timerDisplay.style.display = 'block';
    
    if (timeLeft > 0) {
        timerDisplay.textContent = formatTime(timeLeft);
        
        // Start a new interval
        timerInterval = setInterval(() => {
            const currentTimeLeft = Math.max(0, Math.floor((end - Date.now()) / 1000));
            console.log("Timer update:", currentTimeLeft);
            
            if (currentTimeLeft <= 0) {
                // Just display 00:00:00, don't stop charging
                timerDisplay.textContent = "00:00:00";
                statusAlert.textContent = 'Waiting for ESP32 to complete charging...';
                
                // Stop the timer interval but don't change Firebase
                if (timerInterval) {
                    clearInterval(timerInterval);
                    timerInterval = null;
                }
            } else {
                timerDisplay.textContent = formatTime(currentTimeLeft);
            }
        }, 1000);
    } else {
        timerDisplay.textContent = "00:00:00";
    }
}

// Stop the charging process
function stopCharging() {
    // Reset ESP controlled session flag
    espControlledSession = false;
    
    // Update Firebase to stop charging
    chargingRef.update({
        charging: false
    }).then(() => {
        console.log("Successfully stopped charging in Firebase");
    }).catch((error) => {
        console.error("Error stopping charging:", error);
    });
    
    stopTimer();
    statusAlert.textContent = 'Charging completed';
    statusAlert.className = 'charging-status alert alert-success text-center mb-4';
    
    // Reset UI to initial state
    timeSelection.style.display = 'block';
    paymentSection.style.display = 'none';
    chargingSection.style.display = 'none';
}

// Stop and reset timer
function stopTimer() {
    if (timerInterval) {
        clearInterval(timerInterval);
        timerInterval = null;
    }
    timerDisplay.style.display = 'none';
    endTime = null;
}

// Start button click handler
startButton.addEventListener('click', async () => {
    try {
        // Convert to seconds for ESP32 compatibility
        const currentTimeInSeconds = Math.floor(Date.now() / 1000);
        
        // Set ESP controlled session flag
        espControlledSession = true;
        
        console.log("Start button clicked, updating Firebase with:", {
            charging: true,
            duration: selectedDuration,
            startTime: currentTimeInSeconds,
            paymentStatus: true
        });
        
        // First, update the charging status
        await chargingRef.update({
            charging: true,
            duration: selectedDuration,
            startTime: currentTimeInSeconds,
            paymentStatus: true
        });
        
        console.log("Firebase successfully updated!");
        
        // Force-read the values back to confirm they were set
        const snapshot = await chargingRef.once('value');
        console.log("Current Firebase values after update:", snapshot.val());
        
        // Update UI
        statusAlert.textContent = `Charging started for ${formatTime(selectedDuration)}`;
        statusAlert.className = 'charging-status alert alert-success text-center mb-4';
        startButton.disabled = true;
        
    } catch (error) {
        console.error("Error updating Firebase:", error);
        alert("Error starting charging. Please try again.");
    }
});

// Input validation
[hoursInput, minutesInput, secondsInput].forEach(input => {
    input.addEventListener('input', () => {
        let value = parseInt(input.value) || 0;
        if (value < 0) value = 0;
        if (value > parseInt(input.max)) value = parseInt(input.max);
        input.value = value;
    });
}); 