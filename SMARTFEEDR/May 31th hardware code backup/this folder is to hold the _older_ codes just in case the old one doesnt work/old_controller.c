#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/inotify.h>

#define INSTRUCTION_FILE "instructions.txt"
#define IMAGE_DIR "./images"  // Directory to store images
#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))

void processCommand(const char *command) {
    if (strcmp(command, "DISPENSE FOOD") == 0) {
        system("./Dispenser");
        fprintf(stderr, "Motor triggered to dispense food\n");
    } else if (strcmp(command, "FOOD LEVEL") == 0) {
        int ret = system("./FoodLevelSensor check >> data/data.txt");
        if (ret == 0) {
            fprintf(stderr, "Read Food Level\n");
        } else {
            fprintf(stderr, "Error reading food level\n");
        }
        if (chmod("data/data.txt", 0777) != 0) {
            perror("Error setting permissions for data.txt");
        }
    } else if (strcmp(command, "WATER LEVEL") == 0) {
        system("./WaterLevelSensor check >> data/data.txt");
        fprintf(stderr, "Read Water Level\n");
      
    } else if (strcmp(command, "CAPTURE") == 0) {
        struct stat st = {0};
        if (stat(IMAGE_DIR, &st) == -1) {
            mkdir(IMAGE_DIR, 0700);
        }
        char commandStr[1024];
        snprintf(commandStr, sizeof(commandStr), "./camera_view1 %s image", IMAGE_DIR);
        system(commandStr);
    }
}

void removeExecutedCommand() {
    FILE *srcFile, *tempFile;
    char buffer[1024];

    srcFile = fopen(INSTRUCTION_FILE, "r");
    tempFile = fopen("instruction_tmp.txt", "w");
    if (srcFile == NULL || tempFile == NULL) {
        perror("Error opening files");
        exit(EXIT_FAILURE);
    }

    // Skip the first line because it's the command just executed
    fgets(buffer, 1024, srcFile);

    // Copy the rest of the file to the temp file
    while (fgets(buffer, 1024, srcFile) != NULL) {
        fputs(buffer, tempFile);
    }

    fclose(srcFile);
    fclose(tempFile);

    remove(INSTRUCTION_FILE);
    rename("instruction_tmp.txt", INSTRUCTION_FILE);
    chmod(INSTRUCTION_FILE, 0666);
}

int main() {
    int inotifyFd, wd;
    char buffer[EVENT_BUF_LEN];
    char command[256];  // Correctly declare the command array here

    inotifyFd = inotify_init();
    if (inotifyFd < 0) {
        perror("inotify_init");
    }

    wd = inotify_add_watch(inotifyFd, ".", IN_MODIFY);

    while (1) {
        FILE *file = fopen(INSTRUCTION_FILE, "r+");
        if (!file) {
            perror("Failed to open instruction file");
            sleep(10);
            continue;
        }

        if (fgets(command, sizeof(command), file)) {
            command[strcspn(command, "\n")] = 0; // Remove newline character
            processCommand(command);
            fclose(file);
            removeExecutedCommand();
        } else {
            fclose(file);
            // Wait for a new instruction to be added
            int length = read(inotifyFd, buffer, EVENT_BUF_LEN);
            if (length < 0) {
                perror("read");
                sleep(10); // Sleep to avoid busy waiting in case of read error
                continue;
            }
            // Check if it is the right event and file
            if (strchr(buffer, IN_MODIFY) && strstr(buffer, INSTRUCTION_FILE)) {
                continue; // Resume the loop
            }
        }

        sleep(10); // Throttle check rate
    }

    inotify_rm_watch(inotifyFd, wd);
    close(inotifyFd);
    return 0;
}
