#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// custom strstr function
char *myStrStr(const char *str, const char *fword) {
    // if fword is an empty s, return str
    if (!*fword) {
        return (char *)str;
    }
    // loop through each character of str
    for (; *str; ++str) {
        // check if the current character matches the first character of fword
        if (*str == *fword) {
            const char *h = str, *n = fword;
            // continue checking the following characters
            while (*h && *n && *h == *n) {
                ++h;
                ++n;
            }
            // if the end of fword is reached, return the starting position
            if (!*n) {
                return (char *)str;
            }
        }
    }
    // if fword is not found, return NULL
    return NULL;
}

// custom strchr function
char *myStrChr(const char *s, int c) {
    // continue until the character is found or the end of string is reached
    while (*s != (char)c) {
        if (!*s++) {
            return NULL;
        }
    }
    return (char *)s;
}

void printHighlightedWord(const char *line, const char *word, int lineNumber) {
    const char *current = line;
    const char *found = myStrStr(current, word);

    printf("\033[1;32m[%d]\033[0m ", lineNumber); 

    while (found != NULL) {
        // print the part of the line before the word
        printf("%.*s", (int)(found - current), current); 
        printf("\033[1;31m%s\033[0m", word); 
        // move the current position past the word
        current = found + strlen(word);
        // search for the next occurrence
        found = myStrStr(current, word);
    }

    printf("%s\n", current); 
}

// custom getline function
ssize_t myGetline(char **lineptr, size_t *n, int fd) {
    // if the buffer's size is uninitialized or 0, initialize it with a default size
    if (*lineptr == NULL || *n == 0) {
        *n = 120;  // default size
        *lineptr = (char *)malloc(*n); // allocate memory for the buffer
        if (*lineptr == NULL) return -1;
    }
    // temporary variable to store each character read
    char c = '\0';
    ssize_t count = 0; // counter for the number of characters read

    // continue reading characters one by one until a newline is encountered or read fails
    while (read(fd, &c, 1) > 0 && c != '\n') {
        // store the character in the buffer
        (*lineptr)[count++] = c; 
        // if the buffer is full, double its size
        if (count >= *n) {
            *n *= 2;  // double the buffer size
            // reallocate memory with the new size
            *lineptr = (char *)realloc(*lineptr, *n);
            // if reallocation fails, return -1
            if (*lineptr == NULL) return -1; 
        }
    }
    (*lineptr)[count] = '\0'; // add Null to the end of the string
    // return the count of characters read, or -1 if the end was reached without encountering a newline
    return c == '\n' ? count : -1;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        // check for correct number of arguments
        printf("Usage: <command> <search word>\n");
        return EXIT_FAILURE;
    }

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        return EXIT_FAILURE;
    }

    pid_t pid = fork();
    // fork a child process
    if (pid == -1) {
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        // child process closes end of the reading pipe
        close(pipe_fd[0]); 
        // redirect standard output to the writing end of the pipe
        dup2(pipe_fd[1], STDOUT_FILENO);
        // close the original writing end of the pipe
        close(pipe_fd[1]); 
        execlp("sh", "sh", "-c", argv[1], NULL);
        exit(EXIT_FAILURE);
    }

    // parent process closes end of the writing pipe
    close(pipe_fd[1]); 

    char* line = NULL;
    size_t len = 0;
    int lineNumber = 1;

    // read line by line to end of the file
    while (myGetline(&line, &len, pipe_fd[0]) != -1) {
        if (myStrStr(line, argv[2])) {
            printHighlightedWord(line, argv[2], lineNumber);
        }
        lineNumber++;
    }

    free(line);
    close(pipe_fd[0]);
    wait(NULL);

    return 0;
}