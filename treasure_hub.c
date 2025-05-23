<<<<<<< HEAD
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_LINE 512
#define MAX_PATH 512
#define MAX_USERNAME 100
#define MAX_CLUE 256

typedef struct {
    int treasure_id;        
    char username[MAX_USERNAME];
    float latitude;
    float longitude;
    char clue[MAX_CLUE];
    int value;
} Treasure;

pid_t monitor_pid = -1;
int monitor_status = 0;

char current_hunt[128] = "";
char current_username[128] = "";

// ================= SIGNAL HANDLERS (monitor role) =================

void sigusr1_handler(int sig) {
    printf("SIGUSR1 received\n");
    // List all hunts and number of treasures in each
    DIR *dir = opendir(".");
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    printf("Hunts:\n");

    while ((entry = readdir(dir))) {
        if (entry->d_type == DT_DIR &&
            strcmp(entry->d_name, ".") != 0 &&
            strcmp(entry->d_name, "..") != 0 &&
            strcmp(entry->d_name, ".git") != 0 &&  // Exclude .git
            strcmp(entry->d_name, ".vscode") != 0) {  // Exclude .vscode
            DIR *sub = opendir(entry->d_name);
            if (!sub) continue;

            int total_treasures = 0;
            struct dirent *file;
            while ((file = readdir(sub))) {
                if (strstr(file->d_name, ".dat")) {
                    // Count treasures inside this .dat file
                    char filepath[MAX_PATH];
                    snprintf(filepath, sizeof(filepath), "%s/%s", entry->d_name, file->d_name);

                    int fd = open(filepath, O_RDONLY);
                    if (fd == -1) {
                        perror("Error opening file");
                        continue;
                    }

                    Treasure t;
                    ssize_t bytes_read;
                    while ((bytes_read = read(fd, &t, sizeof(Treasure))) > 0) {
                        if (bytes_read == sizeof(Treasure)) {
                            total_treasures++;  // Increment for each treasure read
                        }
                    }
                    close(fd);
                }
            }
            closedir(sub);

            printf("- %s (%d treasures)\n", entry->d_name, total_treasures);
        }
    }

    closedir(dir);
}


void sigusr2_handler(int sig) {
    printf("SIGUSR2 received\n");

    char hunt_id[100] = {0};
    char treasure_id[100] = {0};  
    int mode = 0; // 1 = view specific, 0 = list all

    int fd = open("command.txt", O_RDONLY);
    if (fd >= 0) {
        char buffer[256] = {0};
        read(fd, buffer, sizeof(buffer) - 1);
        close(fd);

        // Print out what is read from the command.txt for debugging
        printf("Read command.txt: '%s'\n", buffer);

        char *line1 = strtok(buffer, "\n");
        char *line2 = strtok(NULL, "\n");

        if (line1) strncpy(hunt_id, line1, sizeof(hunt_id));
        if (line2) {
            strncpy(treasure_id, line2, sizeof(treasure_id));  
            mode = 1;
        }
    } else {
        perror("open command.txt");
        return;
    }

    // Open hunt directory
    DIR *dir = opendir(hunt_id);
    if (!dir) {
        printf("Hunt '%s' not found.\n", hunt_id);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/%s", hunt_id, entry->d_name);

            printf("Checking file: %s\n", filepath);  // Debugging output

            int fd_treasure = open(filepath, O_RDONLY);
            if (fd_treasure >= 0) {
                Treasure t;
                ssize_t bytes = read(fd_treasure, &t, sizeof(Treasure));
                if (bytes == sizeof(Treasure)) {
                    // Check if we are viewing a specific treasure
                    if (mode == 1) {
                        if (t.treasure_id == atoi(treasure_id)) {
                            printf("Treasure with ID '%d' in %s:\n", t.treasure_id, filepath);
                            printf("Username: %s\n", t.username);
                            printf("Latitude: %.6f\n", t.latitude);
                            printf("Longitude: %.6f\n", t.longitude);
                            printf("Clue: %s\n", t.clue);
                            printf("Value: %d\n", t.value);
                            break; // Only break if in view-specific mode
                        }
                    } else {
                        // Show all treasures (no break here)
                        printf("Treasure in %s:\n", filepath);
                        printf("Treasure ID: %d\n", t.treasure_id);
                        printf("Username: %s\n", t.username);
                        printf("Latitude: %.6f\n", t.latitude);
                        printf("Longitude: %.6f\n", t.longitude);
                        printf("Clue: %s\n", t.clue);
                        printf("Value: %d\n", t.value);
                    }
                }
                close(fd_treasure);
            }
        }
    }
    closedir(dir);

    // Display the command prompt again after showing the treasures
    printf("\nCommands:\n");
    printf("start_monitor\nlist_hunts\nlist_treasures <hunt_id>\n");
    printf("view_treasure <hunt_id> <treasure_id>\nstop_monitor\n");
    printf("calculate_score <hunt_id>\nexit\n");
    printf("> ");
}

// Signal handler for SIGTERM
void sigterm_handler(int sig) {
    if (sig == SIGTERM) {
        printf("Monitor is preparing to exit...\n");
        usleep(2000000);  // Simulate a 2-second delay
        printf("Exiting monitor...\n");
        exit(0);  // Exit the monitor process
    }
}


void sigchld_handler(int sig) {
    int status;
    // Wait for the monitor process to stop without blocking other operations
    if (waitpid(monitor_pid, &status, WNOHANG) > 0) {
        printf("Monitor process has exited.\n");
    }
}

// ================ MONITOR CONTROL (main program) ==================

void start_monitor() {
    if (monitor_pid != -1) {
        printf("Monitor already running.\n");
        return;
    }

    monitor_pid = fork();
    if (monitor_pid == -1) {
        perror("fork");
        return;
    }

    if (monitor_pid == 0) {
        // Inside the child (monitor) process, set up signal handlers
        struct sigaction sa1 = {0}, sa2 = {0}, sa_term = {0};
        sa1.sa_handler = sigusr1_handler;
        sa2.sa_handler = sigusr2_handler;
        sa_term.sa_handler = sigterm_handler;  // Set the SIGTERM handler
        sigaction(SIGUSR1, &sa1, NULL);
        sigaction(SIGUSR2, &sa2, NULL);
        sigaction(SIGTERM, &sa_term, NULL);  // Register SIGTERM handler

        // Monitor process enters the signal wait loop
        while (1) pause();
    } else {
        monitor_status = 1;
        printf("Monitor started with PID %d\n", monitor_pid);
    }

    // Handle SIGCHLD to check if the monitor has terminated
    struct sigaction sa_chld = {0};
    sa_chld.sa_handler = sigchld_handler;
    sigaction(SIGCHLD, &sa_chld, NULL);
}

void stop_monitor() {
    if (monitor_pid == -1) {
        printf("No monitor running.\n");
        return;
    }

    // Send SIGTERM to stop the monitor
    kill(monitor_pid, SIGTERM);

    // Wait for monitor to exit
    printf("Waiting for the monitor to stop...\n");
    if (waitpid(monitor_pid, NULL, 0) == -1) {
        perror("Error waiting for monitor process");
    }

    // Reset monitor variables
    monitor_pid = -1;
    monitor_status = 0;

    printf("Monitor stopped.\n");
}

void list_hunts() {
    if (monitor_pid == -1) {
        printf("Monitor not running.\n");
        return;
    }
    kill(monitor_pid, SIGUSR1);
    sleep(1); 
}

void list_treasures(const char *hunt_id) {
    char filepath[MAX_PATH];
    snprintf(filepath, MAX_PATH, "%s/treasure.dat", hunt_id);

    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file for reading");
        return;
    }

    struct stat file_stat;
    if (fstat(fd, &file_stat) == -1) {
        perror("Error getting file information");
        close(fd);
        return;
    }

    size_t file_size = file_stat.st_size;
    Treasure *treasures = malloc(file_size);
    if (treasures == NULL) {
        perror("Error allocating memory for treasures");
        close(fd);
        return;
    }

    ssize_t bytes_read = read(fd, treasures, file_size);
    if (bytes_read == -1) {
        perror("Error reading file");
        free(treasures);
        close(fd);
        return;
    }

    close(fd);

    size_t treasure_count = bytes_read / sizeof(Treasure);

    printf("Hunt: %s\n", hunt_id);
    printf("\nTreasure File: treasure.dat\n");
    printf("File: %s/treasure.dat\n", hunt_id);
    printf("Size: %ld bytes\n", file_size);
    printf("Last modified: %s\n", ctime(&file_stat.st_mtime));

    // Display treasures
    printf("\nTreasures in this hunt:\n");
    for (size_t i = 0; i < treasure_count; ++i) {
        printf("\nTreasure ID: %d\n", treasures[i].treasure_id);
        printf("Username: %s\n", treasures[i].username);
        printf("Latitude: %.6f\n", treasures[i].latitude);
        printf("Longitude: %.6f\n", treasures[i].longitude);
        printf("Clue: %s\n", treasures[i].clue);
        printf("Value: %d\n", treasures[i].value);
    }

    printf("\nTotal size of treasures: %ld bytes\n", file_size);
    printf("Total number of treasures: %zu\n", treasure_count);

    free(treasures);
}


void view_treasure(const char *hunt_id, const char *treasure_id) {
    char filepath[MAX_PATH];
    snprintf(filepath, MAX_PATH, "%s/treasure.dat", hunt_id);

    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        perror("Error opening file for reading");
        return;
    }

    Treasure t;
    ssize_t bytes_read;
    int found = 0;
    while ((bytes_read = read(fd, &t, sizeof(Treasure))) == sizeof(Treasure)) {
        if (t.treasure_id == atoi(treasure_id)) {
            found = 1;
            printf("Treasure ID: %d\n", t.treasure_id);
            printf("Username: %s\n", t.username);
            printf("Latitude: %.6f\n", t.latitude);
            printf("Longitude: %.6f\n", t.longitude);
            printf("Clue: %s\n", t.clue);
            printf("Value: %d\n", t.value);
            break; // Stop searching after finding the treasure
        }
    }

    if (!found) {
        printf("Treasure with ID '%s' not found.\n", treasure_id);
    }

    close(fd);
}

// ====================== CALCULATE SCORE =====================

void calculate_score(const char *hunt_id) {
    int pipe_fds[2];
    if (pipe(pipe_fds) == -1) {
        perror("Pipe creation failed");
        return;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("Fork failed");
        return;
    }

    if (pid == 0) {
        // Child process: Run the external score calculation program
        close(pipe_fds[0]);  // Close the read end of the pipe

        // Redirect stdout to the pipe
        if (dup2(pipe_fds[1], STDOUT_FILENO) == -1) {
            perror("dup2 failed");
            exit(EXIT_FAILURE);
        }

        // Prepare the command to run (ensure correct path to calculate_score)
        char cmd[MAX_PATH];
        snprintf(cmd, MAX_PATH, "./calculate_score %s", hunt_id); // Ensure path is correct

        // Execute the external score calculation program
        if (execlp("./calculate_score", "calculate_score", hunt_id, (char *)NULL) == -1) {
            perror("exec failed");
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process: Read the result from the pipe
        close(pipe_fds[1]);  // Close the write end of the pipe

        char result[MAX_PATH];
        ssize_t bytes_read;
        
        // Read the full output from the pipe
        while ((bytes_read = read(pipe_fds[0], result, sizeof(result) - 1)) > 0) {
            result[bytes_read] = '\0';  // Null-terminate the result string
            printf("%s", result);  // Print the result chunk by chunk
        }

        if (bytes_read == -1) {
            perror("Read from pipe failed");
        }

        close(pipe_fds[0]);  // Close the read end

        // Wait for the child process to finish
        waitpid(pid, NULL, 0);
    }
}

void exit_program() {
    if (monitor_pid != -1) {
        printf("Error: Monitor is still running. Stop it first.\n");
    } else {
        printf("Goodbye.\n");
        exit(0);
    }
}

// ======================= MAIN LOOP =========================

int main() {
    char command[256];

    while (1) {
        printf("\nCommands:\n");
        printf("  start_monitor\n  list_hunts\n  list_treasures <hunt_id>\n");
        printf("  view_treasure <hunt_id> <treasure_id>\n  calculate_score <hunt_id>\n  stop_monitor\n  exit\n");
        printf("> ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = '\0';

        if (strcmp(command, "start_monitor") == 0) {
            start_monitor();
        } else if (strcmp(command, "stop_monitor") == 0) {
            stop_monitor();
        } else if (strcmp(command, "list_hunts") == 0) {
            list_hunts();
        } else if (strncmp(command, "list_treasures", 14) == 0) {
            char hunt[128];
            if (sscanf(command, "list_treasures %s", hunt) == 1) {
                list_treasures(hunt);
            } else {
                printf("Usage: list_treasures <hunt_id>\n");
            }
        } else if (strncmp(command, "view_treasure", 13) == 0) {
            char hunt[128], user[128];
            if (sscanf(command, "view_treasure %s %s", hunt, user) == 2) {
                view_treasure(hunt, user);
            } else {
                printf("Usage: view_treasure <hunt_id> <treasure_id>\n");
            }
        } else if (strncmp(command, "calculate_score", 15) == 0) {
            char hunt[128];
            if (sscanf(command, "calculate_score %s", hunt) == 1) {
                calculate_score(hunt);
            } else {
                printf("Usage: calculate_score <hunt_id>\n");
            }
        } else if (strcmp(command, "exit") == 0) {
            exit_program();
        } else {
            printf("Unknown command: %s\n", command);
        }
    }

    return 0;
=======
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_LINE 512
#define MAX_PATH 512

pid_t monitor_pid = -1;
int monitor_status = 0;

char current_hunt[128] = "";
char current_username[128] = "";

// ================= SIGNAL HANDLERS (monitor role) =================

void sigusr1_handler(int sig) {
    // List all hunts and number of treasures in each
    DIR *dir = opendir(".");
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    printf("Hunts:\n");

    while ((entry = readdir(dir))) {
        if (entry->d_type == DT_DIR &&
            strcmp(entry->d_name, ".") != 0 &&
            strcmp(entry->d_name, "..") != 0) {
            DIR *sub = opendir(entry->d_name);
            if (!sub) continue;

            int count = 0;
            struct dirent *file;
            while ((file = readdir(sub))) {
                if (strstr(file->d_name, ".txt")) {
                    count++;
                }
            }
            closedir(sub);
            printf("- %s (%d treasures)\n", entry->d_name, count);
        }
    }

    closedir(dir);
}

void sigusr2_handler(int sig) {
    if (sig == SIGUSR2) {
        char hunt_id[100] = {0};
        char username[100] = {0};
        int mode = 0; // 1 = view specific, 0 = list all

        int fd = open("command.txt", O_RDONLY);
        if (fd >= 0) {
            char buffer[256] = {0};
            read(fd, buffer, sizeof(buffer) - 1);
            close(fd);

            char *line1 = strtok(buffer, "\n");
            char *line2 = strtok(NULL, "\n");

            if (line1) strncpy(hunt_id, line1, sizeof(hunt_id));
            if (line2) {
                strncpy(username, line2, sizeof(username));
                mode = 1;
            }
        } else {
            perror("open command.txt");
            return;
        }

        // Open hunt directory
        DIR *dir = opendir(hunt_id);
        if (!dir) {
            printf("Hunt '%s' not found.\n", hunt_id);
            return;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_REG) {
                char filepath[512];
                snprintf(filepath, sizeof(filepath), "%s/%s", hunt_id, entry->d_name);

                int fd_treasure = open(filepath, O_RDONLY);
                if (fd_treasure >= 0) {
                    char buffer[1024] = {0};
                    ssize_t bytes = read(fd_treasure, buffer, sizeof(buffer) - 1);
                    buffer[bytes] = '\0';
                    close(fd_treasure);

                    // Match by username if viewing one
                    if (mode == 1) {
                        if (strstr(buffer, username)) {
                            printf("Treasure for %s in %s:\n%s\n", username, filepath, buffer);
                            break;
                        }
                    } else {
                        printf("Treasure in %s:\n%s\n", filepath, buffer);
                    }
                }
            }
        }
        if (sig == SIGUSR2) {
            usleep(2000000);  // Simulate delay of 2 seconds
            printf("Stopping monitor...\n");
            exit(0);  // Exit after delay
        }
        
        closedir(dir);
    }
}

void sigchld_handler(int sig) {
    int status;
    waitpid(monitor_pid, &status, WNOHANG);  // Avoid blocking if the monitor is already stopped
    monitor_pid = -1;  // Reset the monitor_pid
    printf("Monitor process has exited.\n");
}

// ================ MONITOR CONTROL (main program) ==================

void start_monitor() {
    if (monitor_pid != -1) {
        printf("Monitor already running.\n");
        return;
    }

    monitor_pid = fork();
    if (monitor_pid == -1) {
        perror("fork");
        return;
    }

    if (monitor_pid == 0) {
        struct sigaction sa1 = {0}, sa2 = {0};
        sa1.sa_handler = sigusr1_handler;
        sa2.sa_handler = sigusr2_handler;
        sigaction(SIGUSR1, &sa1, NULL);
        sigaction(SIGUSR2, &sa2, NULL);

        while (1) pause();  
    } else {
        monitor_status = 1;
        printf("Monitor started with PID %d\n", monitor_pid);
    }
    struct sigaction sa_chld = {0};
    sa_chld.sa_handler = sigchld_handler;
    sigaction(SIGCHLD, &sa_chld, NULL);
}

void stop_monitor() {
    if (monitor_pid == -1) {
        printf("No monitor running.\n");
        return;
    }

    kill(monitor_pid, SIGTERM);
    waitpid(monitor_pid, NULL, 0);
    monitor_pid = -1;
    monitor_status = 0;
    printf("Monitor stopped.\n");
}

void list_hunts() {
    if (monitor_pid == -1) {
        printf("Monitor not running.\n");
        return;
    }
    kill(monitor_pid, SIGUSR1);
    sleep(1); 
}

void list_treasures(const char *hunt_id) {
    if (monitor_pid == -1) {
        printf("No monitor process is running.\n");
        return;
    }

    // Write hunt_id to a shared file
    int fd = open("command.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open command.txt");
        return;
    }
    write(fd, hunt_id, strlen(hunt_id));
    close(fd);

    kill(monitor_pid, SIGUSR2);
}

void view_treasure(const char *hunt_id, const char *username) {
    if (monitor_pid == -1) {
        printf("No monitor process is running.\n");
        return;
    }

    // Write hunt_id and username to command.txt
    int fd = open("command.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open command.txt");
        return;
    }

    dprintf(fd, "%s\n%s\n", hunt_id, username); // first line: hunt_id, second line: username
    close(fd);

    kill(monitor_pid, SIGUSR2);
}

void exit_program() {
    if (monitor_pid != -1) {
        printf("Error: Monitor is still running. Stop it first.\n");
    } else {
        printf("Goodbye.\n");
        exit(0);
    }
}

// ======================= MAIN LOOP =========================

int main() {
    char command[256];

    while (1) {
        printf("\nCommands:\n");
        printf("  start_monitor\n  list_hunts\n  list_treasures <hunt_id>\n");
        printf("  view_treasure <hunt_id> <username>\n  stop_monitor\n  exit\n");
        printf("> ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = '\0';

        if (strcmp(command, "start_monitor") == 0) {
            start_monitor();
        } else if (strcmp(command, "stop_monitor") == 0) {
            stop_monitor();
        } else if (strcmp(command, "list_hunts") == 0) {
            list_hunts();
        } else if (strncmp(command, "list_treasures", 14) == 0) {
            char hunt[128];
            if (sscanf(command, "list_treasures %s", hunt) == 1) {
                list_treasures(hunt);
            } else {
                printf("Usage: list_treasures <hunt_id>\n");
            }
        } else if (strncmp(command, "view_treasure", 13) == 0) {
            char hunt[128], user[128];
            if (sscanf(command, "view_treasure %s %s", hunt, user) == 2) {
                view_treasure(hunt, user);
            } else {
                printf("Usage: view_treasure <hunt_id> <username>\n");
            }
        } else if (strcmp(command, "exit") == 0) {
            exit_program();
        } else {
            printf("Unknown command: %s\n", command);
        }
    }

    return 0;
>>>>>>> 21f01c1e0ced3a9b788b6945f6326815e3fd77b7
}