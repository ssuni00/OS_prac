#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h> 
#include <sys/types.h>
#include <string.h>
#include <limits.h>
#include <dirent.h> 


#define BUFFER_SIZE 4096

int main(int argc, char *argv[]) {
    int opt;
    int flag_f = 0, flag_m = 0, flag_d = 0, flag_v=0;
    int mode_num = 0;

    // parse options
    while ((opt = getopt(argc, argv, "fmdv")) != -1) {
        switch (opt) {
            case 'f': flag_f = 1; break;
            case 'm': flag_m = 1; break;
            case 'd': flag_d = 1; break;
            case 'v': flag_v = 1; break;
            case '?':
                printf("Unknown command : %c \n", optopt);
                return 1;
        }
    }

    if(flag_f){
        mode_num = 1; // mode = -f 
    }if(flag_m){
        mode_num = 4; // mode mv = 4-2 = 2
    }if(flag_d){
        mode_num = 5; // mode dv = 5-2 = 3
    }if(flag_v){
        mode_num -= 2; // mode = -f 
    }

    
    if(mode_num == 1){ // -f mode
        // open source file and target file for writing 
        int source_fd = open(argv[optind], O_RDONLY); // argv[1]
        int target_fd = open(argv[optind + 1], O_WRONLY | O_CREAT); // argv[2]

        // copy contents of source file to target file
        char buffer[BUFFER_SIZE];
        ssize_t bytes_read, bytes_written;
        while ((bytes_read = read(source_fd, buffer, BUFFER_SIZE)) > 0) {
            bytes_written = write(target_fd, buffer, bytes_read);
        }

        close(source_fd);
        close(target_fd);

    }else if (mode_num == 2){ // -mv mode
        char *directory = argv[argc - 1]; // direction directory

        // check dir
        struct stat st;

        // copy each sources to direction dir
        for (int i = 2; i <= argc - 2; ++i) { // start from 3rd element of array, which is next -mv
            char *source_file = argv[i];
            char target_path[PATH_MAX];
            snprintf(target_path, PATH_MAX, "%s/%s", directory, source_file);
            
            // open source file and target file for writing 
            int source_fd = open(source_file, O_RDONLY);
            int target_fd = open(target_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR);

            // copy files     
            char buffer[BUFFER_SIZE];
            ssize_t bytes_read, bytes_written;
            while ((bytes_read = read(source_fd, buffer, BUFFER_SIZE)) > 0) {
                bytes_written = write(target_fd, buffer, bytes_read);
            }
            close(source_fd);
            close(target_fd);

            printf("Copy File %s -> %s\n", source_file, target_path);
        }
    }else if (mode_num == 3){ // -dv mode
        const char *source = argv[argc - 2];
        const char *target = argv[argc - 1];
        DIR *dir;
        struct dirent *entry;
        struct stat stat_buf;

        // open the source directory
        if ((dir = opendir(source)) == NULL) {
            perror("opendir");
            return 1;
        }

        // create target directory if it doesn't exist
        mkdir(target, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

        // read directory entries
        while ((entry = readdir(dir)) != NULL) {
            char source_path[PATH_MAX];
            char target_path[PATH_MAX];
            
            snprintf(source_path, PATH_MAX, "%s/%s", source, entry->d_name);
            snprintf(target_path, PATH_MAX, "%s/%s", target, entry->d_name);

            // skip "." and ".." to copy
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            // get information from file path
            if (lstat(source_path, &stat_buf) == -1) {
                perror("lstat");
                continue;
            }

            // if entry is a directory, recursively copy it
            if (S_ISDIR(stat_buf.st_mode)) {
                DIR *subdir;
                struct dirent *subentry;
                struct stat sub_stat_buf;

                // Open the subdirectory
                if ((subdir = opendir(source_path)) == NULL) {
                    perror("opendir");
                    continue;
                }

                // create subdirectory in target if it doesn't exist
                mkdir(target_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

                // Read subdirectory entries
                while ((subentry = readdir(subdir)) != NULL) {
                    char sub_source_path[PATH_MAX];
                    char sub_target_path[PATH_MAX];
                    
                    snprintf(sub_source_path, PATH_MAX, "%s/%s", source_path, subentry->d_name);
                    snprintf(sub_target_path, PATH_MAX, "%s/%s", target_path, subentry->d_name);

                    // skip "." and ".." to copy
                    if (strcmp(subentry->d_name, ".") == 0 || strcmp(subentry->d_name, "..") == 0)
                        continue;

                   // get information from file path
                    if (lstat(source_path, &stat_buf) == -1) {
                        perror("lstat");
                        continue;
                    }

                    int source_fd = open(sub_source_path, O_RDONLY);
                        int target_fd = open(sub_target_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                        char buffer[BUFFER_SIZE];
                        ssize_t bytes_read, bytes_written;
                        while ((bytes_read = read(source_fd, buffer, BUFFER_SIZE)) > 0) {
                            bytes_written = write(target_fd, buffer, bytes_read);
                        }
                        close(source_fd);
                        close(target_fd);

                        printf("Copy File: %s -> %s\n", source_path, target_path);
                }
                closedir(subdir);
            } else { // if entry is a file, copy it
                int source_fd = open(source_path, O_RDONLY);
                int target_fd = open(target_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                char buffer[BUFFER_SIZE];
                ssize_t bytes_read, bytes_written;
                while ((bytes_read = read(source_fd, buffer, BUFFER_SIZE)) > 0) {
                    bytes_written = write(target_fd, buffer, bytes_read);
                }
                close(source_fd);
                close(target_fd);

                printf("Copy File: %s -> %s\n", source_path, target_path);
            }
        }
        closedir(dir);
    }else{
        printf("Error !!");
    }
}
