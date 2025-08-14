# UPI Payment Integration Guide

## Overview
This document provides detailed information about the UPI payment integration in the Solar Charging Station project.

## Payment Flow
1. User selects charging duration
2. System calculates cost based on duration (₹20 per hour)
3. Generates UPI QR code with payment details
4. User scans QR code using any UPI-enabled payment app
5. User completes payment
6. User confirms payment completion on the interface
7. System enables charging functionality

## Technical Implementation

### UPI QR Code Generation
The system generates a UPI QR code with the following parameters:
- `pa`: Payment address (UPI ID)
- `pn`: Payee name (Solar Charging Station)
- `am`: Amount (calculated based on duration)
- `tn`: Transaction note

Example UPI URL format:
```
upi://pay?pa=your-upi-id@upi&pn=Solar%20Charging%20Station&am=20&tn=Charging%20payment
```

### Payment Verification
Currently, the system uses a simple user confirmation approach:
1. User completes payment through their UPI app
2. User clicks "I have completed the payment" button
3. System enables charging functionality

Note: This is a prototype implementation. For production use, implement proper payment verification using UPI APIs.

### Firebase Integration
The payment status is stored in Firebase with the following structure:
```json
{
  "charging_station": {
    "charging": boolean,
    "duration": number,
    "startTime": number,
    "paymentStatus": boolean
  }
}
```

## Security Considerations
1. **Payment Verification**: Implement proper payment verification using UPI APIs in production
2. **Transaction Records**: Maintain transaction logs for accounting and dispute resolution
3. **Error Handling**: Implement proper error handling for failed payments
4. **Timeout Handling**: Add payment timeout functionality
5. **Refund Process**: Document and implement refund process for failed charging sessions

## Future Improvements
1. Integrate with UPI payment verification APIs
2. Add transaction history
3. Implement automatic payment status checking
4. Add receipt generation
5. Implement refund automation
6. Add admin dashboard for payment monitoring

## Testing
1. Test with different UPI apps (Google Pay, PhonePe, BHIM, etc.)
2. Verify payment flow with different amounts
3. Test error scenarios (payment failure, network issues)
4. Verify charging activation after payment
5. Test payment timeout scenarios

## Troubleshooting
1. **QR Code Not Scanning**
   - Ensure proper screen brightness
   - Verify QR code generation parameters
   - Check if amount is within valid range

2. **Payment Not Reflecting**
   - Check internet connectivity
   - Verify Firebase database rules
   - Check payment confirmation status

3. **Charging Not Starting After Payment**
   - Verify payment status in Firebase
   - Check ESP32 connectivity
   - Verify relay control signals

## Support
For payment-related issues:
1. Check transaction ID in UPI app
2. Verify payment status in Firebase
3. Contact system administrator with transaction details 