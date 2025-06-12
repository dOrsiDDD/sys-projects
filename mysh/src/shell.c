#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../includes/shell.h"
#include "../includes/executor.h"
#include "../includes/builtins.h"
#include "../includes/parser.h"

void shell_loop() {
    char *line;
    char **args;

    do {
        line = read_line();
        args = parse_line(line);
        if (!is_builtin(args)) {
            char** args1, **args2;
            if (split_pipes(args, &args1, &args2)) {
                execute_pipes(args1, args2);
                free(args1);
                free(args2);
            } else {
                handle_redirection(args);
                execute_command(args);
            }
        }

        free(line);
        free(args);
    } while (1);
}

char* read_line() {
    char *line = NULL;
    size_t bufsize = 0;

    printf("mysh> ");
    fflush(stdout);

    if (getline(&line, &bufsize, stdin) == -1) {
        if (feof(stdin)) {
            exit(EXIT_SUCCESS);
        } else {
            perror("readline");
            exit(EXIT_FAILURE);
        }
    }
    return line;
}