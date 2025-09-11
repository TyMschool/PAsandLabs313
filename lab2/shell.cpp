/****************
LE2: Introduction to Unnamed Pipes
****************/
#include <unistd.h> // pipe, fork, dup2, execvp, close
#include <sys/wait.h> // waitpid
using namespace std;

int main () {
    // lists all the files in the root directory in the long format
    char* cmd1[] = {(char*) "ls", (char*) "-al", (char*) "/", nullptr};
    // translates all input from lowercase to uppercase
    char* cmd2[] = {(char*) "tr", (char*) "a-z", (char*) "A-Z", nullptr};

    // TODO: add functionality
    // Create pipe
    int pipefd[2], status; //pipefd[0] read end, [1] write end
    pipe(pipefd);
    // Create child to run first command
    // In child, redirect output to write end of pipe
    // Close the read end of the pipe on the child side.
    // In child, execute the command
    pid_t child1 = fork();
    if(child1 == 0){
        dup2(pipefd[1] , 1); // makes 1 point to the same place as pipefd[1]
        close(pipefd[1]);
        execvp(cmd1[0], cmd1);
        //return 1;
    }

    // Create another child to run second command
    // In child, redirect input to the read end of the pipe
    // Close the write end of the pipe on the child side.
    // Execute the second command.
    pid_t child2 = fork();
    if(child2 == 0){
        dup2(pipefd[0], 0);
        close(pipefd[0]);
        execvp(cmd2[0], cmd2);
        //return 1;
    }

    // Reset the input and output file descriptors of the parent.
    waitpid(child2, &status, 0);
}
