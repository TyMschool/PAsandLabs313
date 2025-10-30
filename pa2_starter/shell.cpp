#include <iostream>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#include <vector>
#include <string>

#include "Tokenizer.h"

// all the basic colours for a shell prompt
#define RED     "\033[1;31m"
#define GREEN	"\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE	"\033[1;34m"
#define WHITE	"\033[1;37m"
#define NC      "\033[0m"

using namespace std;

int execute_pipe_group(vector<Command*>& commands, int start, int end) {
    int num_commands = end - start + 1;
    int fdin = 0;
    vector<pid_t> pids;
    
    for (int i = 0; i < num_commands; i++) {
        int cmd_idx = start + i;
        
        //create pipe
        int pipefd[2];
        if (i < num_commands - 1) {
            if (pipe(pipefd) < 0) {
                perror("pipe");
                exit(2);
            }
        }
        
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(2);
        }
        
        if (pid == 0) {  //CHILD PROCESS
            //setup input
            if (i == 0) {  //first command in group
                if (commands[cmd_idx]->hasInput()) {
                    int fdin_file = open(commands[cmd_idx]->in_file.c_str(), O_RDONLY);
                    if (fdin_file < 0) {
                        perror("open input file");
                        exit(2);
                    }
                    dup2(fdin_file, 0);
                    close(fdin_file);
                }
            } else { //read prev pipee
                dup2(fdin, 0);
                close(fdin);
            }
            
            if (i == num_commands - 1) {  //last command in group
                if (commands[cmd_idx]->hasOutput()) {
                    int fdout = open(commands[cmd_idx]->out_file.c_str(), 
                                   O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fdout < 0) {
                        perror("open output file");
                        exit(2);
                    }
                    dup2(fdout, 1);
                    close(fdout);
                }
            } else {  //write to pipe. not the last command
                dup2(pipefd[1], 1);
                close(pipefd[1]);
                close(pipefd[0]);
            }
            
            //execvp
            vector<char*> argv;
            for (const auto& arg : commands[cmd_idx]->args) {
                argv.push_back((char*)arg.c_str());
            }
            argv.push_back(nullptr);
            
            if (execvp(argv[0], argv.data()) < 0) {
                perror("execvp");
                exit(2);
            }
        }
        
        // PARENT PROCESS
        pids.push_back(pid);
        
        //close write end and save read end for next iteration
        if (i < num_commands - 1) {
            close(pipefd[1]);  //close write end in parent
            if (i > 0) {
                close(fdin);  //close previous read end
            }
            fdin = pipefd[0];  //save read end for next command
        } else {
            if (i > 0) {
                close(fdin);  //close the last read end
            }
        }
    }
    
    //wait for childs
    int last_status = 0;
    for (size_t i = 0; i < pids.size(); i++) {
        int status;
        waitpid(pids[i], &status, 0);
        if (i == pids.size() - 1) {  //save last status
            last_status = status;
        }
    }
    
    return last_status;
}

int main () {
    //save STDIN, STDOUT in temp variables
    int tempin = dup(0);
    int tempout = dup(1);
    
    vector<pid_t> bg_processes;

    string prev_dir = "";

    bool should_exit = false;

    for (;;) {
        if (should_exit) break;
        //reap those jawns
        for (auto it = bg_processes.begin(); it != bg_processes.end(); ) {
            int status;
            pid_t result = waitpid(*it, &status, WNOHANG);
            if (result != 0) { 
                it = bg_processes.erase(it); //process done or error. removed from bg
            } else {
                ++it;
            }
        }

        const char* user = getenv("USER"); //gets name
        time_t now = time(nullptr); //gets time
        string time_str = ctime(&now); //to string
        time_str = time_str.substr(4, 15);  //extract "Mon DD HH:MM:SS"
        
        char cwd[1024];
        getcwd(cwd, sizeof(cwd)); //current working directorry
        // need date/time, username, and absolute path to current dir
        cout << YELLOW << time_str << " " << user << ":" << cwd << "$" << NC << " ";
        
        // get user inputted command
        string input;
        getline(cin, input);

        if (input == "exit") {  // print exit message and break out of infinite loop
            cout << RED << "Now exiting shell..." << endl << "Goodbye" << NC << endl;
            break;
        }

        // get tokenized commands from user input
        Tokenizer tknr(input);
        if (tknr.hasError()) {  // continue to next prompt if input had an error
            continue;
        }
        //cd check we do this here because we want our working directory to change
        //the child would change -> die -> parent wouldnt change. not good :(
        if (tknr.commands.at(0)->args.at(0) == "cd") {
            if (tknr.commands.at(0)->args.size() > 1) {
                string target = tknr.commands.at(0)->args.at(1);
                
                if (target == "-") {
                    //to previous directory
                    if (!prev_dir.empty()) {
                        char current[1024];
                        getcwd(current, sizeof(current)); //save the now directory
                        if (chdir(prev_dir.c_str()) == 0) {  //go to prev
                            prev_dir = current;  // previous directory is where we were before the change
                        } else {
                            perror("cd");
                        }
                    }
                } else {
                    //cd
                    char current[1024];
                    getcwd(current, sizeof(current));
                    if (chdir(target.c_str()) == 0) {
                        prev_dir = current;
                    } else {
                        perror("cd");
                    }
                }
            } else {
                //cd no args goes to HOME
                char current[1024];
                getcwd(current, sizeof(current));
                const char* home = getenv("HOME");
                if (chdir(home) == 0) {
                    prev_dir = current;
                } else {
                    perror("cd");
                }
            }
            continue;  //skip to next iteration
        }
        int i = 0;
        while (i < (int)tknr.commands.size()) {
            //check for exit
            if (tknr.commands[i]->args.at(0) == "exit") {
                cout << RED << "Now exiting shell..." << endl << "Goodbye" << NC << endl;
                should_exit = true;
                break;
            }
            
            //find the end of the curr pipe group
            int group_end = i;
            while (group_end < (int)tknr.commands.size() && 
                tknr.commands[group_end]->separator == "|") {
                group_end++;
            }
            
            int last_status = 0;
            
            if (group_end > i) {
                //multiple commands w/ pipes
                last_status = execute_pipe_group(tknr.commands, i, group_end);
            } else {
                //single command
                pid_t pid = fork();
                if (pid < 0) {
                    perror("fork");
                    exit(2);
                }
                
                if (pid == 0) {  // CHILD PROCESS
                    //input redirection
                    if (tknr.commands[i]->hasInput()) {
                        int fdin = open(tknr.commands[i]->in_file.c_str(), O_RDONLY);
                        if (fdin < 0) {
                            perror("open input file");
                            exit(2);
                        }
                        dup2(fdin, 0);
                        close(fdin);
                    }
                    
                    //output redirection
                    if (tknr.commands[i]->hasOutput()) {
                        int fdout = open(tknr.commands[i]->out_file.c_str(), 
                                    O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (fdout < 0) {
                            perror("open output file");
                            exit(2);
                        }
                        dup2(fdout, 1);
                        close(fdout);
                    }
                    
                    //execvp
                    vector<char*> argv;
                    for (const auto& arg : tknr.commands[i]->args) {
                        argv.push_back((char*)arg.c_str());
                    }
                    argv.push_back(nullptr);
                    
                    if (execvp(argv[0], argv.data()) < 0) {
                        perror("execvp");
                        exit(2);
                    }
                } else {  // PARENT PROCESS
                    waitpid(pid, &last_status, 0);
                }
            }
            
            //check if we should continue (for && operator)
            if (group_end < (int)tknr.commands.size()) {
                //next command
                if (tknr.commands[group_end]->separator == "&&") {
                    //check if previous command succeeded
                    if (WIFEXITED(last_status)) {
                        int exit_status = WEXITSTATUS(last_status);
                        if (exit_status != 0) {
                            //previous command failed stop the && chain
                            break;
                        }
                    } else {
                        //command didn't exit normally. stop
                        break;
                    }
                }
            }
            
            //next command/group
            i = group_end + 1;
        }   
        //restore stdin and stdout
        dup2(tempin, 0);
        dup2(tempout, 1);
    }
    return 0;
}
