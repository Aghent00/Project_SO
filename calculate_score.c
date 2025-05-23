#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MAX_PATH 512
#define MAX_USERNAME 100
#define MAX_CLUE 256

// Structure to store treasure information
typedef struct {
    int treasure_id;
    char username[MAX_USERNAME];
    float latitude;
    float longitude;
    char clue[MAX_CLUE];
    int value;
} Treasure;

// Function to calculate score for a given hunt
void calculate_score(const char *hunt_id) {
    DIR *dir = opendir(hunt_id);
    if (!dir) {
        perror("Unable to open hunt directory");
        return;
    }

    struct dirent *entry;
    int total_score = 0;

    while ((entry = readdir(dir))) {
        // Only process .dat files (instead of .treasure)
        if (entry->d_type == DT_REG && strstr(entry->d_name, ".dat")) {
            char filepath[MAX_PATH];
            snprintf(filepath, sizeof(filepath), "%s/%s", hunt_id, entry->d_name);

            // Open treasure file
            int fd = open(filepath, O_RDONLY);
            if (fd == -1) {
                perror("Error opening treasure file");
                continue;
            }

            // Read treasure data
            Treasure t;
            ssize_t bytes_read;
            while ((bytes_read = read(fd, &t, sizeof(Treasure))) > 0) {
                // Ensure we read the full treasure data
                if (bytes_read == sizeof(Treasure)) {
                    printf("\n----------------------------------------\n");
                    printf("Reading treasure file: %s\n", filepath);
                    printf("----------------------------------------\n");
                    printf("Treasure ID     : %d\n", t.treasure_id);
                    printf("Username        : %s\n", t.username);
                    printf("Latitude        : %.6f\n", t.latitude);
                    printf("Longitude       : %.6f\n", t.longitude);
                    printf("Clue            : %s\n", t.clue);
                    printf("Value           : %d\n", t.value);
                    printf("----------------------------------------\n");

                    total_score += t.value;  // Sum the value of the treasure
                } else {
                    printf("Error reading treasure file %s\n", filepath);
                    break;  // Exit if we can't read the full treasure data
                }
            }

            close(fd);
        }
    }

    closedir(dir);
    
    // Output the total score to stdout
    printf("\n----------------------------------------\n");
    printf("Total Score for hunt '%s' : %d\n", hunt_id, total_score);
    printf("----------------------------------------\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <hunt_id>\n", argv[0]);
        return EXIT_FAILURE;
    }

    calculate_score(argv[1]);

    return EXIT_SUCCESS;
}