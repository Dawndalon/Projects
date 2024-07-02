#include <pigpio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // For sleep()

#define SERVO_PIN 13  // GPIO pin used for the servo motor

// Function to initialize GPIO and servo
void setupServo() {
    if (gpioInitialise() < 0) {
        fprintf(stderr, "Failed to initialize pigpio library.\n");
        exit(EXIT_FAILURE);
    }
    // Set the servo to its initial position (closed)
    gpioServo(SERVO_PIN, 2000); // Pulse width for the closed position
    sleep(1); // Wait for the servo to reach the position
}

// Function to dispense food
void dispenseFood() {

    // Pulse widths should be adjusted based on actual hardware calibration
    int openPulseWidth = 1000; // Open position
    int closedPulseWidth = 2000; // Closed position
    
        // Move servo to the open position to dispense food
        gpioServo(SERVO_PIN, openPulseWidth);
        printf("Dispensing food...\n");
        sleep(1); // Wait for food to dispense

        // Return servo to the closed position
        gpioServo(SERVO_PIN, closedPulseWidth);
        printf("Stopping dispensing...\n");
        sleep(1); // Ensure servo has time to move back to the closed position
    
   
}

// Clean up resources
void cleanup() {
    // Ensure the servo is in the closed position before turning off the signal
    gpioServo(SERVO_PIN, 2000); // Closed position
    sleep(1); // Wait to ensure it stays in the closed position
    gpioServo(SERVO_PIN, 0); // Turn off servo signal
    gpioTerminate(); // Terminate library and release resources
}

// void testServo() {
//     int openPulseWidth = 1000; // Open position (90 degrees to the left)
//     int closedPulseWidth = 1500; // Closed position (0 degrees)

//     for (int pw = closedPulseWidth; pw >= openPulseWidth; pw -= 10) {
//         gpioServo(SERVO_PIN, pw);
//         usleep(50000); // Wait 50ms for each step
//     }
//     sleep(1); // Wait at open position
//     for (int pw = openPulseWidth; pw <= closedPulseWidth; pw += 10) {
//         gpioServo(SERVO_PIN, pw);
//         usleep(50000); // Wait 50ms for each step
//     }
//     sleep(1); // Wait at closed position
// }

int main() {
    setupServo();
    dispenseFood();
    cleanup();
    return 0;
}
