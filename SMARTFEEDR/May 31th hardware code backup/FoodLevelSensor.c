#include <stdio.h>
#include <stdlib.h>
#include <gpiod.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <bits/time.h>
#include "FreeRTOS.h"
#include "task.h"

#define TRIG_LINE_NUM 3   // Use GPIO line numbers based on BCM numbering
#define ECHO_LINE_NUM 2
#define CHIP_NAME "gpiochip0"
#define TIMEOUT_MICROSECONDS 250000  // 250 milliseconds timeout for waiting echo
#define CM_THRESHOLD (6 * 2.54)

void measure_distance(struct gpiod_line *trig_line, struct gpiod_line *echo_line) {
    gpiod_line_set_value(trig_line, 1);
    vTaskDelay(MICROSECONDS_TO_TICKS(10)); // Send trigger pulse
    gpiod_line_set_value(trig_line, 0);

    struct timespec start, end;
    int started = 0, ended = 0;
    long duration;
    float distance;

    // Wait for echo to go high with timeout
    long start_time = clock();
    while (gpiod_line_get_value(echo_line) == 0) {
        if ((clock() - start_time) > TIMEOUT_MICROSECONDS) {
            fprintf(stderr, "Echo pulse not detected (start)\n");
            return;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &start);
    started = 1;

    // Wait for echo to go low with timeout
    start_time = clock();
    while (gpiod_line_get_value(echo_line) == 1) {
        if ((clock() - start_time) > TIMEOUT_MICROSECONDS) {
            fprintf(stderr, "Echo pulse not detected (end)\n");
            return;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    ended = 1;

    if (started && ended) {
        duration = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
        distance = (duration / 2.0) * 0.0343;

        if (distance > CM_THRESHOLD) {
            printf("food_level: low\n");
        } else {
            printf("food_level: high\n");
        }
    } else {
        fprintf(stderr, "Failed to measure distance\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2 || strcmp(argv[1], "check") != 0) {
        fprintf(stderr, "Invalid command. Use: %s check\n", argv[0]);
        return 1;
    }

    
    struct gpiod_chip *chip = gpiod_chip_open_by_name(CHIP_NAME);
    if (!chip) {
        fprintf(stderr, "Error opening GPIO chip.\n");

        return 1;
    }

    struct gpiod_line *trig_line = gpiod_chip_get_line(chip, TRIG_LINE_NUM);
    struct gpiod_line *echo_line = gpiod_chip_get_line(chip, ECHO_LINE_NUM);
    if (!trig_line || !echo_line) {
        fprintf(stderr, "Error getting GPIO lines.\n");
        gpiod_chip_close(chip);
    
        return 1;
    }

    if (gpiod_line_request_output(trig_line, "ultrasonic_trigger", 0) < 0 ||
        gpiod_line_request_input(echo_line, "ultrasonic_echo") < 0) {
        fprintf(stderr, "Error setting line directions.\n");
        gpiod_chip_close(chip);

        return 1;
    }

    measure_distance(trig_line, echo_line);

    gpiod_line_release(trig_line);
    gpiod_line_release(echo_line);
    gpiod_chip_close(chip);
 

    return 0;
}
