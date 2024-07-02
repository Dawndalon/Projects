#include <pigpio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // For sleep()

#define SERVO_PIN 13  // GPIO pin used for the servo motor

// Initialize GPIO and set the initial servo position
void initServo() {
    if (gpioInitialise() < 0) {
        fprintf(stderr, "Failed to initialize pigpio library.\n");
        exit(EXIT_FAILURE);
    }
    // Set servo to neutral position
    gpioServo(SERVO_PIN, 2000);
    fprintf(stderr, "Initializing positon\n");
    sleep(3);  // Wait for 1 second
}

// Function to set the servo position
void setServoPosition(int pulseWidth) {
    // Check if the pulse width setting is successful
    if (gpioServo(SERVO_PIN, pulseWidth) != 0) {
        fprintf(stderr, "Failed to set servo position.\n");
        exit(EXIT_FAILURE);
    }
    printf("Servo set to %d microsecond pulse.\n", pulseWidth);
}

int main(int argc, char *argv[]) {
    initServo();

    // Check if the program has the required argument
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <pulse width>\n", argv[0]);
        return 1;
    }

    // Convert the command line argument to an integer
    int rotations = atoi(argv[1]);

    // Validate the pulse width
    if (rotations < 1 || rotations > 10) {
        fprintf(stderr, "Invalid rotation input. Must be between 1 and 10 microseconds.\n");
        return 1;
    }

    for (int i = 0; i < rotations; i++) {
        setServoPosition();
        sleep(1);
        setServoPosition(2500);
        sleep(1);
    }
    
    // // Check if the program has the required argument
    // if (argc < 2) {
    //     fprintf(stderr, "Usage: %s <pulse width>\n", argv[0]);
    //     return 1;
    // }

    // // Convert the command line argument to an integer
    // int pulseWidth = atoi(argv[1]);

    // // Validate the pulse width
    // if (pulseWidth < 500 || pulseWidth > 2500) {
    //     fprintf(stderr, "Invalid pulse width. Must be between 500 and 2500 microseconds.\n");
    //     return 1;
    // }
 
    // // Set the servo position
    // setServoPosition(pulseWidth);

    // // Hold the position for 2 seconds to observe the servo's motion
    // sleep(2);

    // Reset servo position to neutral
    gpioServo(SERVO_PIN, 2000);
    fprintf(stderr, "Returning back to neutral positon\n");
    sleep(3);  // Wait for 1 second

    // Terminate the GPIO library and clean up resources
    gpioTerminate();
    return 0;
}
