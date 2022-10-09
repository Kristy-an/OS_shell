#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

//built-in command
int wish_exit();
int wish_cd();
int wish_path();

void wish_error();
int wish_execute();

char* path[100];
//List of built-in commands' names and functions.
char *builtin_str[] = { "exit", "cd", "path" };
int num_builtins = 3;
int (*builtin_func[])(char **) = { &wish_exit, &wish_cd, &wish_path };


/*
 * Built-in command: change directory.
 * @param args List of args. args[0] is cd. args[1] is the path.
 */
int wish_cd(char **args){
  if (args[1] == NULL) {
    wish_error();
  } else {
    if (chdir(args[1]) != 0) wish_error();
  }
  return 1;
}

/*
 * Print error message whenever encounter any type of error
 */
void wish_error(){
  char error_message[30] = "An error has occurred\n";
  write(STDERR_FILENO, error_message, strlen(error_message));
}


/*
 * add arguments to the search path of the shell
 * overwrite the old path with the newly specified path.
 */
int wish_path(char **args){
  int p = 0;
  while(1){
    if( args[p] == NULL ) break;
    path[p] = args[p+1];
    p++;
  }
  return 1;
}

int wish_exit(){
  exit(0);
  return 0;
}

int wish_launch(char **args);
/*
 * Execute shell built-in or launch program.
 * return 1 if the shell should continue running;
 *        0 if it should terminate;
 */
int wish_execute(char **args){
  if(args[0] == NULL) return 1;

  int i;
  for (i = 0; i < num_builtins; i++){
    if (strcmp(args[0], builtin_str[i]) == 0) return (*builtin_func[i])(args);
  }

  return wish_launch(args);
}


/*
 * Launch a program and wait for termination.
 *
 */
int wish_launch(char **args){
  pid_t pid;
  int status;

  pid = fork();
  if (pid == 0) {
    if (execvp(args[0], args))
      wish_error();
  } else if (pid < 0){
    wish_error();
  } else {
    do {
      waitpid(pid, &status, WUNTRACED);
    } while( !WIFEXITED(status) && !WIFSIGNALED(status));
  }
  return 1;
}

/*
 * Read a line of input from stdin.
 * @return The line from stdin.
 */
char *wish_read_line(){
  char *line = malloc( 1024*sizeof(char));
  ssize_t size = 0;
  char *redirec = NULL;

  if (getline(&line, &size, stdin) == -1) {
    if(feof(stdin)) {
      exit(EXIT_SUCCESS); //we receiced an EOF
    } else {
      wish_error();
      exit(EXIT_FAILURE);
    }
  }

  return line;

}

/*
 * Split a line in to tokens.
 * @return NULL-terminated array of tokens.
 */
#define WISH_DELIM " \t\r\n\a"
char **wish_split_line(char *line){
  char **tokens = malloc(1024*sizeof(char*));
  char *token;
  int p = 0; //position

  if(!tokens) wish_error();
  token = strtok(line, WISH_DELIM);
  while (token != NULL){
    tokens[p] = token;
    token = strtok(NULL, WISH_DELIM);
    p++;
  }

  tokens[p] = NULL;
  return tokens;
}



int main(int argc, char *argv[]){


  char *line;
  char **args;
  int status;

  while(1){
    if (argc == 1){
    // built-in command mode
      printf("wish> ");
      line = wish_read_line();
      args = wish_split_line(line);
      wish_execute(args);

      free(line);
      free(args);
    }
  }
}
