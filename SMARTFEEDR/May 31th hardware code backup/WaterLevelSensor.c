#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <gpiod.h>
#include <string.h>

#define SENSOR_PIN_NUM 17   // GPIO pin connected to the liquid level sensor
#define CHIP_NAME "gpiochip0"

int sensorState = 1;  // Assume high water level initially

void checkWaterLevel() {
    FILE *fp = fopen("./data/WaterData.txt", "a");  // Open file in append mode
    if (fp == NULL) {
        perror("Error opening file for writing");
        return;
    }

    struct gpiod_chip *chip = gpiod_chip_open_by_name(CHIP_NAME);
    if (!chip) {
        perror("Error opening GPIO chip");
        fclose(fp);
        return;
    }

    struct gpiod_line *sensor_line = gpiod_chip_get_line(chip, SENSOR_PIN_NUM);
    if (!sensor_line) {
        perror("Error getting GPIO line");
        gpiod_chip_close(chip);
        fclose(fp);
        return;
    }

    if (gpiod_line_request_input(sensor_line, "liquid_level_sensor") < 0) {
        perror("Error requesting GPIO line as input");
        gpiod_chip_close(chip);
        fclose(fp);
        return;
    }

    // Update water level state
    int newSensorState = gpiod_line_get_value(sensor_line);
    if (newSensorState < 0) {
        perror("Error reading GPIO line value");
    } else {
        sensorState = newSensorState;
        // Determine water level status based on sensor state and write to file
        if (sensorState == 0) {
            fprintf(fp, "water_level: low\n");
        } else {
            fprintf(fp, "water_level: high\n");
        }
    }

    fflush(fp);
    fclose(fp);
    gpiod_line_release(sensor_line);
    gpiod_chip_close(chip);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "No command specified.\n");
        return 1;
    }
    if (strcmp(argv[1], "check") == 0) {
        checkWaterLevel();
    } else {
        fprintf(stderr, "Invalid command. Use 'check'.\n");
        return 1;
    }

    return 0;
}
