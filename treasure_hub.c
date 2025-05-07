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
}