/**
 * SLOsh - San Luis Obispo Shell
 * CSC 453 - Operating Systems
 * 
 * TODO: Complete the implementation according to the comments
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <sys/wait.h>
 #include <sys/types.h>
 #include <fcntl.h>
 #include <signal.h>
 #include <limits.h>
 #include <errno.h>
 
 /* Define PATH_MAX if it's not available */
 #ifndef PATH_MAX
 #define PATH_MAX 4096
 #endif
 
 #define MAX_INPUT_SIZE 1024
 #define MAX_ARGS 64
 
 /* Global variable for signal handling */
 volatile sig_atomic_t child_running = 0;
 
 /* Forward declarations */
 void display_prompt(void);
 
 /**
  * Signal handler for SIGINT (Ctrl+C)
  * 
  * TODO: Implement the signal handler to:
  * 1. Print a newline
  * 2. If no child is running, display a prompt
  * 3. Make sure the shell doesn't exit when SIGINT is received
  */
 void sigint_handler(int sig) {
     /* TODO: Your implementation here */
     printf("\n");
     if(child_running == 0){
        printf("No child process running\n");
        display_prompt();
     }else{
        kill(child_running, SIGINT);
     }
     return;
 }
 
 /**
  * Display the command prompt with current directory
  */
 void display_prompt(void) {
     char cwd[PATH_MAX];
     
     if (getcwd(cwd, sizeof(cwd)) != NULL) {
         printf("%s> ", cwd);
     } else {
         perror("getcwd");
         printf("SLOsh> ");
     }
     fflush(stdout);
 }
 
 /**
  * Parse the input line into command arguments
  * 
  * TODO: Parse the input string into tokens and store in args array
  * 
  * @param input The input string to parse
  * @param args Array to store parsed arguments
  * @return Number of arguments parsed
  */
 int parse_input(char *input, char **args) {
     /* TODO: Your implementation here */
     // Exit on command temporarily here for testing
     int count = 0;

     // Tokenize the input string using whitespace as the delimiter
     char *token = strtok(input, " \t\n");
     while (token != NULL && count < MAX_ARGS - 1) {
         args[count++] = token;
         token = strtok(NULL, " \t\n");
     }
 
     // Null-terminate the args array
     args[count] = NULL;
 
     return count;
 }
 
  /**
  * Execute the pipe command
  * 
  * @param args Array of command arguments (NULL-terminated)
  * @param cmdIndex Pipe comman index in arg array
  */
  void execute_pipe(char **cmdLeft, char **cmdRight) {
    int fds[2];
    pipe(fds);

    pid_t child1 = fork();
    pid_t child2 = fork();
    if((child1 < 0) || (child2 < 0)){ //error while forking
        perror("failed fork");
        exit(EXIT_FAILURE);
    } else{ //successful forking
        if(child1 == 0){ // cmdLeft child process
            signal(SIGINT, SIG_DFL); //reset signal handing
            close(fds[0]);
            dup2(fds[1], STDOUT_FILENO); // redirect stdout to pipe
            close(fds[1]);
            execvp(cmdLeft[0], cmdLeft); //execute the command
            perror(cmdLeft[0]);
            exit(EXIT_FAILURE);
        }
        if(child2 == 0){ // cmdRight child process
            signal(SIGINT, SIG_DFL); // reset signal handing
            close(fds[1]);
            dup2(fds[0], STDIN_FILENO); // redirect stdin to pipe
            close(fds[0]);
            execvp(cmdRight[0], cmdRight); // execute the command
            perror(cmdRight[0]);
            exit(EXIT_FAILURE);
        }
    }
    //parent process
    int status;
    close(fds[0]);
    close(fds[1]);
    child_running = child1;
    waitpid(child_running, &status, 0);
    //if child1 ended with failure
    if(WEXITSTATUS(status)){printf("Child exited with status %d.\n", WEXITSTATUS(status));}
    child_running = child2;
    waitpid(child_running, &status, 0);
    //if child2 ended with failure
    if(WEXITSTATUS(status)){printf("Child exited with status %d.\n", WEXITSTATUS(status));}
  }

  /**
  * Execute the redirect command
  * 
  * @param args Array of command arguments (NULL-terminated)
  * @param cmdIndex Pipe comman index in arg array
  * @param clobber Flag for overwritting redirect destination
  */
  void execute_redirect(char **args, int cmdIndex, int clobber) {
    int fd;
    char *filename = args[cmdIndex + 1];
    args[cmdIndex] = NULL;
    if(clobber){fd = open(filename, (O_WRONLY | O_CREAT | O_TRUNC), 0644);}
    else{fd = open(filename, (O_WRONLY | O_CREAT | O_APPEND), 0644);}
    if(fd < 0){
        perror("open");
        exit(EXIT_FAILURE);
    }
    if((child_running = fork()) == 0){ //child process
        signal(SIGINT, SIG_DFL); //reset signal handing
        dup2(fd, STDOUT_FILENO); // redirect stdout to the destination file
        close(fd);
        execvp(args[0], args); //execute the command
        perror(args[0]);
        exit(EXIT_FAILURE);
     }
     else if(child_running < 0){ //error while forking
        perror("failed fork");
        exit(EXIT_FAILURE);
     } else { //parent process
        int status;
        waitpid(child_running, &status, 0);
        //if child ended with failure
        if(WEXITSTATUS(status)){
            printf("Child exited with status %d.\n", WEXITSTATUS(status));
        }
        close(fd);
    }
  }

 /**
  * Execute the given command with its arguments
  * 
  * TODO: Implement command execution with support for:
  * 1. Basic command execution
  * 2. Pipes (|)
  * 3. Output redirection (> and >>)
  * 
  * @param args Array of command arguments (NULL-terminated)
  */
 void execute_command(char **args) {
     /* TODO: Your implementation here */
     
     /* Hints:
      * 1. Fork a child process
      * 2. In the child, reset signal handling and execute the command
      * 3. In the parent, wait for the child and handle its exit status
      * 4. For pipes, create two child processes connected by a pipe
      * 5. For redirection, use open() and dup2() to redirect stdout
      */

     char *commands[MAX_ARGS][MAX_ARGS];
     int cmdIndex = 0, argIndex = 0, redirectIndex = -1, overwrite = 0;

     for(int i=0; args[i] != NULL; i++){
         if(strcmp(args[i], ">") == 0){
            redirectIndex = i;
            overwrite = 1;
         } else if(strcmp(args[i], ">>") == 0){
            redirectIndex = i;
         }
     }
      
     for (int i = 0; args[i] != NULL; i++) {
         if (strcmp(args[i], "|") == 0) {
             commands[cmdIndex][argIndex] = NULL;
             cmdIndex++;
             argIndex = 0;
         } else {
             commands[cmdIndex][argIndex] = args[i];
             argIndex++;
         }
     }
     commands[cmdIndex][argIndex] = NULL;
     cmdIndex++;


    // If there's only one command, execute it directly
    if (cmdIndex == 1) {
        if ((child_running = fork()) == 0) { // Child process
            signal(SIGINT, SIG_DFL); // Reset signal handling
            execvp(commands[0][0], commands[0]); // Execute the command
            perror(commands[0][0]);
            exit(EXIT_FAILURE);
        } else if (child_running < 0) { // Error while forking
            perror("fork");
            exit(EXIT_FAILURE);
        } else { // Parent process
            int status;
            waitpid(child_running, &status, 0);
            if (WEXITSTATUS(status)) {
                printf("Child exited with status %d.\n", WEXITSTATUS(status));
            }
        }
        return;
    }

    int fds[2];
    int in_fd = STDIN_FILENO; // Input file descriptor for the first command

    for (int i = 0; i < cmdIndex; i++) {
        pipe(fds); // Create a pipe

        if ((child_running = fork()) == 0) { // Child process
            signal(SIGINT, SIG_DFL); // Reset signal handling

            // Redirect input
            if (in_fd != STDIN_FILENO) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }

            // Redirect output if not the last command
            if (i < cmdIndex - 1) {
                dup2(fds[1], STDOUT_FILENO);
            }
            close(fds[0]);
            close(fds[1]);

            execvp(commands[i][0], commands[i]); // Execute the command
            perror(commands[i][0]);
            exit(EXIT_FAILURE);
        } else if (child_running < 0) { // Error while forking
            perror("fork");
            exit(EXIT_FAILURE);
        } else { // Parent process
            int status;
            waitpid(child_running, &status, 0);
            if (WEXITSTATUS(status)) {
                printf("Child exited with status %d.\n", WEXITSTATUS(status));
            }

            // Close the write end of the pipe and update in_fd
            close(fds[1]);
            in_fd = fds[0];
        }
    }
 }
 
 /**
  * Check for and handle built-in commands
  * 
  * TODO: Implement support for built-in commands:
  * - exit: Exit the shell
  * - cd: Change directory
  * 
  * @param args Array of command arguments (NULL-terminated)
  * @return 0 to exit shell, 1 to continue, -1 if not a built-in command
  */
 int handle_builtin(char **args) {
     /* TODO: Your implementation here */
     if(strcmp(args[0], "exit") == 0){
        return 0; /* Exit shell */
     }else if(strcmp(args[0], "cd") == 0){
        if(args[1] == NULL){
            fprintf(stderr, "cd: missing argument\n");
            return 1;
        }
        if(chdir(args[1]) != 0){
            perror("cd");
        }
        return 1; /* Continue shell */
     }
     return -1;  /* Not a builtin command */
 }
 
 int main(void) {
     char input[MAX_INPUT_SIZE];
     char *args[MAX_ARGS];
     int status = 1;
     int builtin_result;
     
     /* TODO: Set up signal handling for SIGINT (Ctrl+C) */
     if(signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("signal");
        exit(EXIT_FAILURE);
     }

     while (status) {
         display_prompt();
         
         /* Read input and handle signal interruption */
         if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
             /* TODO: Handle EOF and signal interruption */
             break;
         }
         
         /* Parse input */
         parse_input(input, args);
         
         /* Handle empty command */
         if (args[0] == NULL) {
             continue;
         }
         
         /* Check for built-in commands */
         builtin_result = handle_builtin(args);
         if (builtin_result >= 0) {
             status = builtin_result;
             continue;
         }
         
         /* Execute external command */
         execute_command(args);
     }
     
     printf("SLOsh exiting...\n");
     return EXIT_SUCCESS;
 }