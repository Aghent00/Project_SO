#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#ifdef _WIN32
    #include <direct.h>
    #define mkdir(dir, mode) _mkdir(dir)
#else
    #include <sys/stat.h>
    #include <unistd.h>
#endif

#define MAX_USERNAME 100
#define MAX_CLUE 256
#define FILE_PATH_MAX 512

#pragma pack(1)
typedef struct {
<<<<<<< HEAD
    int treasure_id;        
=======
    char treasure_id[50];  // Added Treasure ID
>>>>>>> 21f01c1e0ced3a9b788b6945f6326815e3fd77b7
    char username[MAX_USERNAME];
    float latitude;
    float longitude;
    char clue[MAX_CLUE];
    int value;
} Treasure;
#pragma pack()

// Function to log the operation in the logged_hunt file
void log_operation(const char *hunt_id, const char *operation, const char *details) {
<<<<<<< HEAD
    const char *log_filepath = "hunt_log.txt";  // Global log file path
=======
    char log_filepath[FILE_PATH_MAX];
    snprintf(log_filepath, FILE_PATH_MAX, "%s/logged_hunt.txt", hunt_id);
>>>>>>> 21f01c1e0ced3a9b788b6945f6326815e3fd77b7

    int logfile = open(log_filepath, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (logfile < 0) {
        perror("Error opening log file");
        return;
    }

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[20];
    strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", tm_info);

<<<<<<< HEAD
    // Log the operation with hunt_id
    dprintf(logfile, "[%s] Hunt: %s | Operation: %s | Details: %s\n", timestamp, hunt_id, operation, details);

    close(logfile);
=======
    dprintf(logfile, "[%s] Operation: %s | Details: %s\n", timestamp, operation, details);

    close(logfile);

    // Create symbolic link for the logged_hunt.txt file
    char symlink_path[FILE_PATH_MAX];
    snprintf(symlink_path, FILE_PATH_MAX, "%s/logged_hunt-%s", hunt_id, hunt_id);
    
    if (unlink(symlink_path) != 0 && errno != ENOENT) {
        perror("Error removing existing symbolic link");
        return;
    }

    if (symlink(log_filepath, symlink_path) != 0) {
        perror("Error creating symbolic link");
    }
>>>>>>> 21f01c1e0ced3a9b788b6945f6326815e3fd77b7
}

// Function to create the hunt directory if it does not exist
void create_hunt_directory(const char *hunt_id) {
    if (mkdir(hunt_id, 0755) != 0 && access(hunt_id, F_OK) != 0) {
        perror("Error creating hunt directory");
        exit(EXIT_FAILURE);
    }
}

// Function to add a new treasure to the specified hunt
void add_treasure(const char *hunt_id) {
    Treasure t;
    printf("Enter Treasure ID: ");
<<<<<<< HEAD
    scanf("%d", &t.treasure_id);  // Changed to integer input
=======
    scanf("%s", t.treasure_id);
>>>>>>> 21f01c1e0ced3a9b788b6945f6326815e3fd77b7

    printf("Enter username (unique): ");
    scanf("%s", t.username);

    printf("Enter latitude: ");
    scanf("%f", &t.latitude);

    printf("Enter longitude: ");
    scanf("%f", &t.longitude);

    getchar();  // To consume the newline character left by scanf
    printf("Enter clue text: ");
    fgets(t.clue, MAX_CLUE, stdin);
    t.clue[strcspn(t.clue, "\n")] = 0;  // Remove newline character from the end

    printf("Enter value: ");
    scanf("%d", &t.value);

    create_hunt_directory(hunt_id);

    // Save to treasure.dat file for treasures (APPENDING, not overwriting)
    char filepath[FILE_PATH_MAX];
<<<<<<< HEAD
    snprintf(filepath, FILE_PATH_MAX, "%s/treasure.dat", hunt_id);

    int fd = open(filepath, O_WRONLY | O_CREAT | O_APPEND, 0644);  // O_APPEND ensures new treasure is added to the end
=======
    snprintf(filepath, FILE_PATH_MAX, "%s/%s.treasure", hunt_id, t.treasure_id);

    int fd = open(filepath, O_WRONLY | O_CREAT | O_APPEND, 0644);
>>>>>>> 21f01c1e0ced3a9b788b6945f6326815e3fd77b7
    if (fd == -1) {
        perror("Error opening file for writing");
        exit(EXIT_FAILURE);
    }

<<<<<<< HEAD
    // Append the new treasure to the file
    ssize_t bytes_written = write(fd, &t, sizeof(Treasure));
    if (bytes_written != sizeof(Treasure)) {
        perror("Error writing treasure to file");
    }
    close(fd);

    char details[256];
    snprintf(details, sizeof(details), "Added treasure with Treasure ID '%d'", t.treasure_id);
    log_operation(hunt_id,"add", details);
=======
    write(fd, &t, sizeof(Treasure));
    close(fd);

    char details[256];
    snprintf(details, sizeof(details), "Added treasure with Treasure ID '%s'", t.treasure_id);
    log_operation(hunt_id, "add", details);
>>>>>>> 21f01c1e0ced3a9b788b6945f6326815e3fd77b7

    printf("Treasure saved successfully.\n");
}

// Function to get the file size and last modification time
void print_file_info(const char *filepath) {
    struct stat file_stat;
    if (stat(filepath, &file_stat) == 0) {
        printf("File: %s\n", filepath);
        printf("Size: %ld bytes\n", file_stat.st_size);
        
        char time_str[100];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&file_stat.st_mtime));
        printf("Last modified: %s\n", time_str);
    } else {
        perror("Unable to get file info");
    }
}

// Function to list all treasures in the specified hunt
void list_treasures(const char *hunt_id) {
    char filepath[FILE_PATH_MAX];
    snprintf(filepath, FILE_PATH_MAX, "%s/treasure.dat", hunt_id);

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

// Function to read and print a treasure details from the specified file
void view_treasure(const char *hunt_id, const char *treasure_id) {
    char filepath[FILE_PATH_MAX];
<<<<<<< HEAD
    snprintf(filepath, FILE_PATH_MAX, "%s/treasure.dat", hunt_id);
=======
    snprintf(filepath, FILE_PATH_MAX, "%s/%s.treasure", hunt_id, treasure_id);
>>>>>>> 21f01c1e0ced3a9b788b6945f6326815e3fd77b7

    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        perror("Error opening file for reading");
        return;
    }

    Treasure t;
    ssize_t bytes_read = read(fd, &t, sizeof(Treasure));
<<<<<<< HEAD
    int found = 0;

    while (bytes_read == sizeof(Treasure)) {
        // Check if this treasure matches the requested ID
        if (t.treasure_id == atoi(treasure_id)) {
            found = 1;
            break;
        }
        bytes_read = read(fd, &t, sizeof(Treasure));  // Read next treasure
    }

    close(fd);

    if (found) {
        // Display the treasure details if found
        printf("Treasure Details:\n");
        printf("Treasure ID: %d\n", t.treasure_id);
        printf("Username: %s\n", t.username);
        printf("Latitude: %.6f\n", t.latitude);
        printf("Longitude: %.6f\n", t.longitude);
        printf("Clue: %s\n", t.clue);
        printf("Value: %d\n", t.value);
    } else {
        printf("Treasure with ID '%s' not found.\n", treasure_id);
    }
}

// Function to remove a treasure from a hunt
void remove_treasure(const char *hunt_id, const char *treasure_id) {
    char filepath[FILE_PATH_MAX];
    snprintf(filepath, FILE_PATH_MAX, "%s/treasure.dat", hunt_id);

    int fd = open(filepath, O_RDONLY);  // Open file for reading
    if (fd == -1) {
        perror("Error opening file for reading");  // If file doesn't open, print error
        return;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {  // Get file size
        perror("Error getting file size");
        close(fd);
        return;
    }

    size_t file_size = st.st_size;
    Treasure *treasures = malloc(file_size);  // Allocate memory for treasures
    if (treasures == NULL) {
        perror("Error allocating memory");
        close(fd);
        return;
    }

    ssize_t bytes_read = read(fd, treasures, file_size);  // Read the treasures from file
    if (bytes_read == -1) {
        perror("Error reading file");
        free(treasures);
        close(fd);
        return;
    }

    close(fd);

    size_t treasure_count = bytes_read / sizeof(Treasure);
    int treasure_index = -1;

    // Find the treasure by matching the ID
    for (size_t i = 0; i < treasure_count; ++i) {
        if (treasures[i].treasure_id == atoi(treasure_id)) {  // Match by ID
            treasure_index = i;
            break;
        }
    }

    if (treasure_index == -1) {
        printf("Treasure with Treasure ID '%s' not found.\n", treasure_id);
        free(treasures);
        return;
    }

    // Shift remaining treasures down
    for (size_t i = treasure_index; i < treasure_count - 1; ++i) {
        treasures[i] = treasures[i + 1];
    }

    // Open file for rewriting (O_TRUNC to clear the old data)
    fd = open(filepath, O_WRONLY | O_TRUNC);  
    if (fd == -1) {
        perror("Error opening file for writing");
        free(treasures);
        return;
    }

    ssize_t bytes_written = write(fd, treasures, (treasure_count - 1) * sizeof(Treasure));  // Write remaining treasures
    if (bytes_written == -1) {
        perror("Error writing file after removing treasure");
    }

    close(fd);
    free(treasures);

    char details[256];
    snprintf(details, sizeof(details), "Removed treasure with Treasure ID '%s'", treasure_id);
    log_operation(hunt_id,"remove_treasure", details);  // Log the removal operation

    printf("Treasure with ID '%s' has been removed.\n", treasure_id);
}

// Function to remove an entire hunt with all treasure files
void remove_hunt(const char *hunt_id) {
    DIR *dir = opendir(hunt_id);
    if (dir == NULL) {
        fprintf(stderr, "Hunt directory '%s' does not exist or cannot be opened.\n", hunt_id);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char filepath[FILE_PATH_MAX];
        snprintf(filepath, FILE_PATH_MAX, "%s/%s", hunt_id, entry->d_name);

        if (unlink(filepath) == 0) {
            printf("Removed treasure file: %s\n", filepath);
        } else {
            perror("Error removing treasure file");
        }
    }

    closedir(dir);

    // Now we proceed to remove the hunt directory itself
    if (rmdir(hunt_id) == 0) {
        char details[256];
        snprintf(details, sizeof(details), "Removed entire hunt '%s'", hunt_id);
        log_operation(hunt_id,"remove_hunt", details);  // Log the removal of the entire hunt
        printf("Hunt '%s' has been completely removed.\n", hunt_id);
    } else {
        perror("Error removing hunt directory");
    }

=======
    if (bytes_read != sizeof(Treasure)) {
        perror("Error reading treasure data or file corrupted");
        close(fd);
        return;
    }

    close(fd);

    printf("Treasure Details:\n");
    printf("Treasure ID: %s\n", t.treasure_id);
    printf("Username: %s\n", t.username);
    printf("Latitude: %.6f\n", t.latitude);
    printf("Longitude: %.6f\n", t.longitude);
    printf("Clue: %s\n", t.clue);
    printf("Value: %d\n", t.value);
>>>>>>> 21f01c1e0ced3a9b788b6945f6326815e3fd77b7
}

// Function to remove a treasure from a hunt
void remove_treasure(const char *hunt_id, const char *treasure_id) {
    char filepath[FILE_PATH_MAX];
    snprintf(filepath, FILE_PATH_MAX, "%s/treasures.dat", hunt_id);

    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file for reading");
        return;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("Error getting file size");
        close(fd);
        return;
    }

    size_t file_size = st.st_size;
    Treasure *treasures = malloc(file_size);
    if (treasures == NULL) {
        perror("Error allocating memory");
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
    int treasure_index = -1;

    for (size_t i = 0; i < treasure_count; ++i) {
        if (strcmp(treasures[i].treasure_id, treasure_id) == 0) {
            treasure_index = i;
            break;
        }
    }

    if (treasure_index == -1) {
        printf("Treasure with Treasure ID '%s' not found.\n", treasure_id);
        free(treasures);
        return;
    }

    // Shift remaining treasures down
    for (size_t i = treasure_index; i < treasure_count - 1; ++i) {
        treasures[i] = treasures[i + 1];
    }

    // Open file for rewriting
    fd = open(filepath, O_WRONLY | O_TRUNC);
    if (fd == -1) {
        perror("Error opening file for writing");
        free(treasures);
        return;
    }

    ssize_t bytes_written = write(fd, treasures, (treasure_count - 1) * sizeof(Treasure));
    if (bytes_written == -1) {
        perror("Error writing file after removing treasure");
    }

    close(fd);
    free(treasures);

    char details[256];
    snprintf(details, sizeof(details), "Removed treasure with Treasure ID '%s'", treasure_id);
    log_operation(hunt_id, "remove_treasure", details);

    printf("Treasure with ID '%s' has been removed.\n", treasure_id);
}

// Function to remove an entire hunt with all treasure files
void remove_hunt(const char *hunt_id) {
    DIR *dir = opendir(hunt_id);
    if (dir == NULL) {
        fprintf(stderr, "Hunt directory '%s' does not exist or cannot be opened.\n", hunt_id);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char filepath[FILE_PATH_MAX];
        snprintf(filepath, FILE_PATH_MAX, "%s/%s", hunt_id, entry->d_name);

        if (unlink(filepath) == 0) {
            printf("Removed treasure file: %s\n", filepath);
        } else {
            perror("Error removing treasure file");
        }
    }

    closedir(dir);

    if (rmdir(hunt_id) == 0) {
        char details[256];
        snprintf(details, sizeof(details), "Removed entire hunt '%s'", hunt_id);
        log_operation(hunt_id, "remove_hunt", details);
        printf("Hunt '%s' has been completely removed.\n", hunt_id);
    } else {
        perror("Error removing hunt directory");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s add <hunt_id> | list <hunt_id> | view <hunt_id> <id> | remove_treasure <hunt_id> <id> | remove_hunt <hunt_id>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "add") == 0) {
        add_treasure(argv[2]);
    } else if (strcmp(argv[1], "list") == 0) {
        list_treasures(argv[2]);
    } else if (strcmp(argv[1], "view") == 0 && argc == 4) {
        view_treasure(argv[2], argv[3]);
    } else if (strcmp(argv[1], "remove_treasure") == 0 && argc == 4) {
      remove_treasure(argv[2], argv[3]);
    } else if (strcmp(argv[1], "remove_hunt") == 0 && argc == 3) {
      remove_hunt(argv[2]);
    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
