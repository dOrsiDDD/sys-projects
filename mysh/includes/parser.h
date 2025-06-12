#ifndef PARSER_H
#define PARSER_H

char** parse_line(char *line);
int split_pipes(char **args, char ***args1, char ***args2);
void handle_redirection(char **args);

#endif