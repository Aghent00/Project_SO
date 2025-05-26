#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define MAX_PATH 512
#define MAX_USERNAME 100
#define MAX_CLUE 256
#define MAX_USERS 100  // Limita de utilizatori

// Structure to store treasure information
typedef struct {
    int treasure_id;
    char username[MAX_USERNAME];
    float latitude;
    float longitude;
    char clue[MAX_CLUE];
    int value;
} Treasure;

// Structure to store score for each user
typedef struct {
    char username[MAX_USERNAME];
    int score;
} UserScore;

// Function to find if a user exists in the list of scores
int find_user(UserScore *user_scores, int num_users, const char *username) {
    for (int i = 0; i < num_users; i++) {
        if (strcmp(user_scores[i].username, username) == 0) {
            return i;  // User found
        }
    }
    return -1;  // User not found
}

// Function to calculate score for a given hunt
void calculate_score(const char *hunt_id) {
    DIR *dir = opendir(hunt_id);
    if (!dir) {
        perror("Unable to open hunt directory");
        return;
    }

    struct dirent *entry;
    UserScore user_scores[MAX_USERS];
    int num_users = 0;

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

                    // Check if the user already exists in the list
                    int user_index = find_user(user_scores, num_users, t.username);
                    if (user_index == -1) {
                        // New user, add to the list
                        if (num_users < MAX_USERS) {
                            strcpy(user_scores[num_users].username, t.username);
                            user_scores[num_users].score = t.value;
                            num_users++;
                        }
                    } else {
                        // User exists, add score to the existing user
                        user_scores[user_index].score += t.value;
                    }
                } else {
                    printf("Error reading treasure file %s\n", filepath);
                    break;  // Exit if we can't read the full treasure data
                }
            }

            close(fd);
        }
    }

    closedir(dir);

    // Output the score for each user
    printf("\n----------------------------------------\n");
    for (int i = 0; i < num_users; i++) {
        printf("Total score in '%s' for user '%s' : %d\n", hunt_id, user_scores[i].username, user_scores[i].score);
    }
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
