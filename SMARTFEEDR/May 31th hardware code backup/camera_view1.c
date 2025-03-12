#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h> 
#include <unistd.h>

#ifndef DT_REG
#define DT_REG 8
#endif

// Function prototype
void captureImage(const char *baseDir);

int main(int argc, char *argv[]) {
    // Check for the correct directory argument
    if (argc < 3) {
        printf("Usage: %s <base_directory> <command>\n", argv[0]);
        return 1;
    }

    const char *baseDir = argv[1];
    const char *command = argv[2];

    if (strcmp(command, "image") == 0) {
        captureImage(baseDir);
    } else {
        printf("Unknown command. Use 'image'.\n");
        return 1;
    }

    return 0;
}

void captureImage(const char *baseDir) {
    char command[256];
    char filePath[256];
    int maxNum = 0;
    DIR *d;
    struct dirent *dir;
    d = opendir(baseDir);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_REG) { // If the entry is a regular file
                int num;
                // Assuming file names are in the format "Pic_%d.jpg"
                if (sscanf(dir->d_name, "Pic_%d.jpg", &num) == 1) { 
                    if (num > maxNum) maxNum = num;
                }
            }
        }
        closedir(d);
    }
    // The next file number
    int fileNum = maxNum + 1;

    // Construct the file path for the new image
    snprintf(filePath, sizeof(filePath), "%s/Pic_%d.jpg", baseDir, fileNum);

    // Construct the command to take a picture and save it to the constructed file path
    snprintf(command, sizeof(command), "libcamera-still --width 1024 --height 768 -o \"%s\"", filePath);
    // Execute the command
    system(command);
}
