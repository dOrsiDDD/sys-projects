#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char** parse_line(char* line) {
    int bufsize = 64, position = 0;
    char** tokens = malloc(bufsize * sizeof(char*));
    char* token;

    if (!tokens) {
        fprintf(stderr, "Allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, " \t\n");
    while (token != NULL) {
        tokens[position] = token;
        position++;
        if (position >= bufsize) {
            bufsize += 64;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                fprintf(stderr, "Reallocation error\n");
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, " \t\r\n\a");
    }

   tokens[position] = NULL; // Null-terminate the array of tokens
    return tokens;
}

int split_pipes(char** args, char*** args1, char*** args2) {
    int = 0;
    int pipe_pos = -1;

    while (args[i] != NULL) {
        if (strcmp(args[i], "|") == 0) {
            if (args[i+1] == NULL) {
                fprintf(stderr, "Syntax error: pipe at end of command\n");
                return 0;
            }
            pipe_pos = i;
            break;
        }
        i++;
    }

    if (pipe_pos == -1) return 0;

    *args1 = malloc((pipe_pos + 1) * sizeof(char*));
    if (*args1 == NULL) {
        perror("mysh: allocation error");
        return 0;
    }

    for (i = 0; i< pipe_pos; i++) {
        (*args1)[1] = args[i];
    }
    (*args1)[pipe_pos] = NULL;

    int args2_len = 0;
    while (args[pipe_pos + 1 + args2_len] != NULL) {
        args2_len++;
    }

    *args2 = malloc((args2_len + 1) * sizeof(char*));
    if (*args2 == NULL) {
        free(*args1);
        perror("mysh: allocation error");
        return 0;
    }

    for (i = 0; i < args2_len; i++) {
        (*args2)[i] = args[pipe_pos + 1 + i];
    }
    (*args2)[args2_len] = NULL;

    return 1;
}

int handle_redirection(char** args) {
    int i = 0;
    int inf_fd = -1, out_fd = -1;
    char* in_file = NULL;
    char* out_file = NULL;

    while (args[i] != NULL) {
        if (strcmp(args[i], "<") == 0) {
            if (args[i+1] == NULL) {
                fprintf(stderr, "Syntax error: input redirection without file\n");
                goto error;
            }
            in_file = args[i+1];
            args[i] = NULL;
        }
        else if (strcmp(args[i], ">") == 0) {
            if (args[i+1] == NULL) {
                fprintf(stderr, "Syntax error: output redirection without file\n");
                goto error;
            }
            out_file = args[i+1];
            args[i] = NULL;
        }
        else if (strcmp(args[i], ">>") == 0) {
            if (args[i+1] == NULL) {
                fprintf(stderr, "Syntax error: append redirection without file\n");
                goto error;
            }
            out_file = args[i+1];
            args[i] = NULL;
        }
        i++;
    }

    if (in_file) {
        in_fd = open(in_file, O_RDONLY);
        if (in_fd == -1) {
            perror("mysh: open input file");
            goto error;
        }
        if (dup2(in_fd, STDIN_FILENO) == -1) {
            perror("mysh: dup2 input redirection");
            goto error;
        }
        close(in_fd);
    }

    if (out_file && strcmp(args[i-2], ">>") == 0) {
        out_fd = open(out_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (out_fd == -1) {
            perror("mysh: open output file");
            goto error;
        }
        if(dup2(out_fd, STDOUT_FILENO) == -1) {
            perror("mysh: dup2 output redirection");
            goto error;
        }
        close(out_fd);
    }

    return 0;
error:
    if (in_fd != -1) close(in_fd);
    if (out_fd != -1) close(out_fd);
    return -1;
}