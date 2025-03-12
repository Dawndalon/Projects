#include <pigpio.h>
#include <stdio.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"

#define SERVO_PIN 13  // GPIO pin used for the servo motor

// Function to initialize GPIO and servo
void setupServo() {
    if (gpioInitialise() < 0) {
        fprintf(stderr, "Failed to initialize pigpio library.\n");
        exit(EXIT_FAILURE);
    }
    gpioServo(SERVO_PIN, 2000); // Pulse width for the closed position
    vTaskDelay(pdMS_TO_TICKS(1000)); // FreeRTOS delay, instead of sleep
}

// Function to dispense food
void dispenseFood() {
    int openPulseWidth = 1000; // Open position
    int closedPulseWidth = 2000; // Closed position

    gpioServo(SERVO_PIN, openPulseWidth);
    printf("Dispensing food...\n");
    vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for servo to move to open position

    gpioServo(SERVO_PIN, closedPulseWidth);
    printf("Stopping dispensing...\n");
    vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for servo to move back to closed position
}

// Clean up resources
void cleanup() {
    gpioServo(SERVO_PIN, 2000); // Closed position
    vTaskDelay(pdMS_TO_TICKS(1000)); // Wait to ensure it stays in the closed position
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
