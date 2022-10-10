#include <sys/wait.h>
#include <sys/types.h>

#include <string.h>
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
int wish_execute(char **args);
int wish_launch(char **args);
char *wish_read_line();
char **wish_split_line(char *line);

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
    wish_error(1);
  } else if (args[2] != NULL) {
    wish_error(2);
  } else {
    if (chdir(args[1]) != 0) {
        wish_error(3);
    }
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

int wish_exit(){
  exit(0);
  return 0;
}

char* itoa(int num,char* str,int radix)
{
    char index[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";//索引表
    unsigned unum;//存放要转换的整数的绝对值,转换的整数可能是负数
    int i=0,j,k;//i用来指示设置字符串相应位，转换之后i其实就是字符串的长度；转换后顺序是逆序的，有正负的情况，k用来指示调整顺序的开始位置;j用来指示调整顺序时的交换。
 
    //获取要转换的整数的绝对值
    if(radix==10&&num<0)//要转换成十进制数并且是负数
    {
        unum=(unsigned)-num;//将num的绝对值赋给unum
        str[i++]='-';//在字符串最前面设置为'-'号，并且索引加1
    }
    else unum=(unsigned)num;//若是num为正，直接赋值给unum
 
    //转换部分，注意转换后是逆序的
    do
    {
        str[i++]=index[unum%(unsigned)radix];//取unum的最后一位，并设置为str对应位，指示索引加1
        unum/=radix;//unum去掉最后一位
 
    }while(unum);//直至unum为0退出循环
 
    str[i]='\0';//在字符串最后添加'\0'字符，c语言字符串以'\0'结束。
 
    //将顺序调整过来
    if(str[0]=='-') k=1;//如果是负数，符号不用调整，从符号后面开始调整
    else k=0;//不是负数，全部都要调整
 
    char temp;//临时变量，交换两个值时用到
    for(j=k;j<=(i-1)/2;j++)//头尾一一对称交换，i其实就是字符串的长度，索引最大值比长度少1
    {
        temp=str[j];//头部赋值给临时变量
        str[j]=str[i-1+k-j];//尾部赋值给头部
        str[i-1+k-j]=temp;//将临时变量的值(其实就是之前的头部值)赋给尾部
    }
 
    return str;//返回转换后的字符串
 
}


/*
 * Print error message whenever encounter any type of error
 */
void wish_error(int i){
  char error_message[30] = "An error has occurred\n";
  write(STDERR_FILENO, error_message, strlen(error_message));

  char str[3];
  itoa(i, str, 10);
  write(STDERR_FILENO, str, strlen(str));
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
      int file_out = open(redirec_path,O_CREAT|O_WRONLY|O_TRUNC, 0644);
      if(file_out>0 ){
        dup2(file_out, STDOUT_FILENO);
	    close(file_out);
      }   
    }
    execvp(args[0], args);
    exit(EXIT_FAILURE);

  } else if (pid < 0){
  // Fork error
    wish_error(5);
  } else {
  // Parent process
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

  if(!line) { // Allocation failed
    wish_error(6);
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
    wish_error(7);
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
  char *line;
  char **args;

  fptr = fopen(file, "r");

  if (fptr == NULL)
  {
  	wish_error(8);
  	exit(1);
  }
  else {
    while(fgets(line, sizeof(line), fptr) != NULL) 
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
    wish_error(9); 
  }

  return 0;
}
