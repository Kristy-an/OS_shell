/*********************************************
 @author Hongyan Zhao
 @brief  WISH(Wisconsin Shell)
**********************************************/

#include <sys/wait.h>
#include <sys/types.h>

#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

char* path[100];

//built-in command
int wish_exit(char **args);
int wish_cd();
int wish_path();

//List of built-in commands' names and functions.
char *builtin_str[] = { "exit", "cd", "path" };
int num_builtins = 3;
int (*builtin_func[])(char **) = { &wish_exit, &wish_cd, &wish_path };

// Other functions
void wish_error();
int wish_execute(char **args);
int wish_launch(char **args);
char *wish_read_line();
char **wish_split_line(char *line);

/*
 * Built-in command: change directory.
 * @param args List of args. args[0] is cd. args[1] is the path.
 */
int wish_cd(char **args){
   int status = chdir(args[1]);
   if(status == -1) {
     wish_error();
   }
  return 1;
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

int wish_exit(char **args){
  if( args[1]!= NULL) {wish_error();}
  exit(0);
  return 0;
}


/*if(strcmp(trim(argv[0]),"./wish")!= 0 ) wish_error();
 * Print error message whenever encounter any type of error
 */
void wish_error(){
  char error_message[30] = "An error has occurred\n";
  write(STDERR_FILENO, error_message, strlen(error_message));
}



/*
 * Launch a program and wait for termination.
 * @param args is Null-terminated list of arguments.
 * @retrun Always return 1 to continue execution.
 */
int wish_launch(char **args){
  pid_t pid;
  int status;
  char* redirec_sign =NULL;
  char* redirec_path =NULL;

  // Check redirection
  int i = 0;
  while ( *(args+i) != NULL){ // Check all tokens
    if(strcmp( *(args+i),">")) {
      redirec_sign = *(args+i);
      redirec_path = *(args+i+1); // Record the path;
      break;          
    }
  }
 
  pid = fork();
  if (pid == 0) {
  // Child process
    // If need Redirection
    if ( redirec_sign != NULL){
      // if(redirec_path == NULL) wish_error();
      int file_out = open(redirec_path,O_CREAT|O_WRONLY|O_TRUNC, 0644);
      if(file_out>0 ){
        dup2(file_out, STDOUT_FILENO);
        close(file_out);
      }  
    }
    if ( execvp(args[0], args) == -1 ) wish_error();
    exit(EXIT_FAILURE);
  } else if (pid < 0){
  // Fork errorif(strcmp(trim(argv[0]),"./wish")!= 0 ) wish_error();
    wish_error();
  } else {
  // Parent process
    do {
      waitpid(pid, &status, WUNTRACED);
    } while( !WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

/*
 * Execute shell built-in or launch program.
 * return 1 if the shell should continue running;
 *        0 if it should terminate;
 */
int wish_execute(char **args){
  if(args[0] == NULL) return 1;

  // Build in command
  int i;
  for (i = 0; i < num_builtins; i++){
    if (strcmp(args[0], builtin_str[i]) == 0) return (*builtin_func[i])(args);
  }

  // Not built-in, run wish_launch to find the program.
  return wish_launch(args);
}


/*
 * Read a line of input from stdin.
 * @return The line from stdin.
 */
char *wish_read_line(){
  char *line = malloc( 1024*sizeof(char));
  ssize_t size = 0;

  if(!line) { // Allocation failed
    wish_error();
    exit(EXIT_FAILURE);
  }

  if (getline(&line, &size, stdin) == -1) {
    if(feof(stdin)) {
      free(line);
      exit(EXIT_SUCCESS); //we receiced an EOF
    } else {
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

  if(!tokens) {
    wish_error();
  }
  token = strtok(line, WISH_DELIM);
  while (token != NULL){
    tokens[p] = token;
    token = strtok(NULL, WISH_DELIM);
    p++;
  }

  tokens[p] = NULL;
  return tokens;
}


int wish_interact(){
  char *line;
  char **args;
  while (1) {
      printf("wish> ");
      line = wish_read_line();
      args = wish_split_line(line);
      wish_execute(args);

      free(line);
      free(args);
  }
  return 1;
}

int wish_batch(char file[100]) {
  FILE *fptr;
  char *line =malloc (2048 *sizeof(char));
  char **args;

  fptr = fopen(file, "r");

  if (fptr == NULL)
  {
    wish_error();
    exit(1);
  }
  else {
    while(fgets(line, 2048, fptr) != NULL)
    {
      args = wish_split_line(line);
      wish_execute(args);
    }
  }

  free(args);
  fclose(fptr);
  return 1;
}


int main(int argc, char *argv[]){

path[0] = "/bin";
  FILE *file;
  char *line = malloc(1024 *sizeof(char));
  char **args;
  int status;
  int size = 1024;

  if (argc == 1){
    wish_interact();
  } else if (argc == 2){
    wish_batch(argv[1]);
  } else {
    wish_error();
  }

  return 0;
}
