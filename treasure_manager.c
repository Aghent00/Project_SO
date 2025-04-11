#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

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

typedef struct {
    char username[MAX_USERNAME];
    float latitude;
    float longitude;
    char clue[MAX_CLUE];
    int value;
} Treasure;


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
    printf("Enter username (unique): ");
    scanf("%s", t.username);

    printf("Enter latitude: ");
    scanf("%f", &t.latitude);

    printf("Enter longitude: ");
    scanf("%f", &t.longitude);

    getchar();  
    printf("Enter clue text: ");
    fgets(t.clue, MAX_CLUE, stdin);
    t.clue[strcspn(t.clue, "\n")] = 0;  

    printf("Enter value: ");
    scanf("%d", &t.value);

    create_hunt_directory(hunt_id);

    char filepath[FILE_PATH_MAX];
    snprintf(filepath, FILE_PATH_MAX, "%s/%s.treasure", hunt_id, t.username);

    FILE *file = fopen(filepath, "w");
    if (!file) {
        perror("Error opening file for writing");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "Username: %s\n", t.username);
    fprintf(file, "Latitude: %.6f\n", t.latitude);
    fprintf(file, "Longitude: %.6f\n", t.longitude);
    fprintf(file, "Clue: %s\n", t.clue);
    fprintf(file, "Value: %d\n", t.value);

    fclose(file);
    printf("Treasure saved successfully to %s\n", filepath);
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
    DIR *dir = opendir(hunt_id);
    if (dir == NULL) {
        perror("Error opening directory");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    long total_size = 0;
    int treasure_count = 0;

    printf("Hunt: %s\n", hunt_id);

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {  
            char filepath[FILE_PATH_MAX];
            snprintf(filepath, FILE_PATH_MAX, "%s/%s", hunt_id, entry->d_name);
            struct stat file_stat;
            if (stat(filepath, &file_stat) == 0) {
                total_size += file_stat.st_size;
                treasure_count++;
                printf("\nTreasure File: %s\n", entry->d_name);
                print_file_info(filepath);
            }
        }
    }
    closedir(dir);

    printf("\nTotal size of treasures: %ld bytes\n", total_size);
    printf("Total number of treasures: %d\n", treasure_count);
}

// Function to read and print a treasure's details from the specified file
void view_treasure(const char *hunt_id, const char *username) {
    char filepath[FILE_PATH_MAX];
    snprintf(filepath, FILE_PATH_MAX, "%s/%s.treasure", hunt_id, username);

    FILE *file = fopen(filepath, "r");
    if (!file) {
        perror("Error opening file");
        return;
    }

    Treasure t;
    fscanf(file, "Username: %s\n", t.username);
    fscanf(file, "Latitude: %f\n", &t.latitude);
    fscanf(file, "Longitude: %f\n", &t.longitude);
    fgets(t.clue, MAX_CLUE, file);
    t.clue[strcspn(t.clue, "\n")] = 0;  
    fscanf(file, "Value: %d\n", &t.value);

    fclose(file);

    printf("Treasure Details:\n");
    printf("Username: %s\n", t.username);
    printf("Latitude: %.6f\n", t.latitude);
    printf("Longitude: %.6f\n", t.longitude);
    printf("Clue: %s\n", t.clue);
    printf("Value: %d\n", t.value);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s add <hunt_id> | list <hunt_id>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "add") == 0) {
        add_treasure(argv[2]);
    } else if (strcmp(argv[1], "list") == 0) {
        list_treasures(argv[2]);
    } else if (strcmp(argv[1], "view") == 0 && argc == 4) {
        view_treasure(argv[2], argv[3]);
    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}