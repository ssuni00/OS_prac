#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFER_SIZE 4096

int main(int argc, char *argv[]) {
    int opt;
    int flag_f = 0, flag_mv = 0, flag_dv = 0;

    // Parse options
    while ((opt = getopt(argc, argv, "f")) != -1) {
        switch (opt) {
            case 'f':
                flag_f = 1;
                break;
            case 'mv':
                flag_mv = 1;
                break;
            case 'dv':
                flag_dv = 1;
                break;
            case '?':
                printf("Unknown command : %c \n", optopt);
                return 1;
        }
    }

    // Check if the -f flag is provided
    if (flag_f) {
        // Check if correct number of arguments are provided
        if (argc - optind != 2) {
            fprintf(stderr, "Usage: %s -f <source_file> <target_file>\n", argv[0]);
            return 1;
        }

        // Open source file for reading
        int source_fd = open(argv[optind], O_RDONLY);
        if (source_fd == -1) {
            perror("Error opening source file");
            return 1;
        }

        // Open target file for writing (create if not exists, truncate otherwise)
        int target_fd = open(argv[optind + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        if (target_fd == -1) {
            perror("Error opening target file");
            close(source_fd);
            return 1;
        }

        // Copy contents of source file to target file
        char buffer[BUFFER_SIZE];
        ssize_t bytes_read, bytes_written;
        while ((bytes_read = read(source_fd, buffer, BUFFER_SIZE)) > 0) {
            bytes_written = write(target_fd, buffer, bytes_read);
            if (bytes_written != bytes_read) {
                perror("Error writing to target file");
                close(source_fd);
                close(target_fd);
                return 1;
            }
        }

        // Check for read error
        if (bytes_read == -1) {
            perror("Error reading from source file");
            close(source_fd);
            close(target_fd);
            return 1;
        }

        // Close files
        close(source_fd);
        close(target_fd);

        printf("File copied successfully.\n");
    } if(flag_mv){
        printf("mv successfully.\n");
    } if(flag_dv){
        printf("dv successfully.\n");
    }else {
        fprintf(stderr, "Usage: %s -f <source_file> <target_file>\n", argv[0]);
        return 1;
    }

    return 0;
}
