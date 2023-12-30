#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define MAX_FILES 32
#define MAX_SIZE (200 * 1024 * 1024)

void printUsage() {
    printf("Usage: tarsau -b file1 file2 ... -o archive_name.sau\n");
    printf("       tarsau -a archive_name.sau directory\n");
    exit(1);
}

void mergeFiles(int argc, char *argv[]) {
    char *archive_name = "a.sau";

    if (argc < 4) {
        printUsage();
    }

    if (strcmp(argv[1], "-b") != 0) {
        printUsage();
    }

    int numFiles = argc - 3;
    if (numFiles > MAX_FILES) {
        printf("Error: Too many files. Maximum number of files is %d.\n", MAX_FILES);
        exit(1);
    }

    int total_size = 0;
    FILE *archive = NULL;

    if (strcmp(argv[argc - 2], "-o") == 0) {
        archive_name = argv[argc - 1];
    }

    archive = fopen(archive_name, "wb");

    if (archive == NULL) {
        perror("Error");
        printf("Error: Could not create archive file: %s\n", archive_name);
        exit(1);
    }

    char buffer[12]; 
    sprintf(buffer, "%010d|", 0);
    fwrite(buffer, sizeof(char), 11, archive);

    for (int i = 2; i < argc - 2; i++) {
        char *filename = argv[i];
        FILE *file = fopen(filename, "rb");

        if (file == NULL) {
            perror("Error");
            printf("Error: Could not open file %s.\n", filename);
            exit(1);
        }

        fseek(file, 0L, SEEK_END);
        int size = ftell(file);
        total_size += size;

        if (total_size > MAX_SIZE) {
            printf("Error: Total size of files exceeds %d bytes.\n", MAX_SIZE);
            exit(1);
        }

        fseek(file, 0L, SEEK_SET);
        char *file_content = (char *)malloc(sizeof(char) * size);
        fread(file_content, sizeof(char), size, file);
        fclose(file);

        char permissions[] = "rw-r--r--";
        char filesize[11]; 
        sprintf(filesize, "%010d", size);

        char record[strlen(filename) + strlen(permissions) + strlen(filesize) + 3];
        sprintf(record, "%s,%s,%s|", filename, permissions, filesize);
        fwrite(record, sizeof(char), strlen(record), archive);

        fwrite(file_content, sizeof(char), size, archive);

        free(file_content);
    }

    fseek(archive, 0L, SEEK_SET);
    sprintf(buffer, "%010d|", total_size);
    fwrite(buffer, sizeof(char), 11, archive);

    fclose(archive);
}

void extractFiles(char *archive_name, char *directory) {
    FILE *archive = fopen(archive_name, "rb");

    if (archive == NULL) {
        perror("Error");
        printf("Error: Could not open archive file: %s\n", archive_name);
        exit(1);
    }

    int total_size;
    fscanf(archive, "%010d|", &total_size);

    if (total_size == 0) {
        printf("Error: Invalid archive file.\n");
        exit(1);
    }

    char *buffer = (char *)malloc(sizeof(char) * total_size);
    fread(buffer, sizeof(char), total_size, archive);
    fclose(archive);

    int offset = 10;

    while (offset < total_size) {
        char filename[255], permissions[10];
        int size;

        sscanf(&buffer[offset], "%[^,],%[^,],%d|", filename, permissions, &size);

        offset += strlen(filename) + strlen(permissions) + sizeof(size) + 3;

        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", directory, filename);

        FILE *file = fopen(filepath, "wb");

        if (file == NULL) {
            perror("Error");
            printf("Error: Could not create file %s.\n", filepath);
            exit(1);
        }

        fwrite(&buffer[offset], sizeof(char), size, file);

        offset += size;
        fclose(file);

        chmod(filepath, strtol(permissions, 0, 8));
    }

    free(buffer);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printUsage();
    }

    if (strcmp(argv[1], "-b") == 0) {
        mergeFiles(argc, argv);
        printf("The files have been merged.\n");
    } else if (strcmp(argv[1], "-a") == 0) {
        if (argc != 4) {
            printf("Error: Invalid number of arguments.\n");
            exit(1);
        }

        char *archive_name = argv[2];
        char *directory = argv[3];

        struct stat st = {0};
        if (stat(directory, &st) == -1) {
            if (mkdir(directory, 0700) != 0) {
                perror("Error");
                printf("Error: Could not create directory %s.\n", directory);
                exit(1);
            }
        }

        extractFiles(archive_name, directory);

        printf("Files opened in the %s directory:\n", directory);
    } else {
        printf("Error: Invalid command.\n");
        exit(1);
    }

    return 0;
}
