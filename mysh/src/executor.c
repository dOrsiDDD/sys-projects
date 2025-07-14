#include "../includes/executor.h"
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

void execute_command(char** args) {
    if (args == NULL || args[0] == NULL) {
        return;
    }

    if (is_builtin(args)) {
        execute_builtin(args);
        return;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("mysh: fork failed");
        return;
    } else if (pid == 0) {
        execvp(args[0], args);

        fprintf(stderr, "mysh: %s: %s\n", args[0], strerror(errno));
        _exit(EXIT_FAILURE);
    } else {
        int status;
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
}

void execute_pipe(char** args1, char** args2) {
    if (args1 == NULL || args1[0] == NULL || args2 == NULL || args2[0] == NULL) {
        fprintf(stderr, "mysh: invalid pipe syntax\n");
        return;
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("mysh: pipe failed");
        return;
    }

    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("mysh: fork failed");
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }

    if (pid1 == 0) {
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            perror("mysh: dup2 failed");
            _exit(EXIT_FAILURE);
        }
        close(pipefd[1]);
        execvp(args1[0], args1);
        fprintf(stderr, "mysh: %s: %s\n", args1[0], strerror(errno));
        _exit(EXIT_FAILURE);
    }

    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("mysh: fork failed");
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }

    if (pid2 == 0) {
        close(pipefd[1]);
        if (dup2(pipefd[0], STDIN_FILENO) == -1) {
            perror("mysh: dup2 failed");
            _exit(EXIT_FAILURE);
        }
        close(pipefd[0]);
        execvp(args2[0], args2);
        fprintf(stderr, "mysh: %s: %s\n", args2[0], strerror(errno));
        _exit(EXIT_FAILURE);
    }

    close(pipefd[0]);
    close(pipefd[1]);

    int status1, status2;
    waitpid(pid1, &status1, 0);
    waitpid(pid2, &status2, 0);
}