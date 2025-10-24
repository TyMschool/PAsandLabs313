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

int main () {
    //save STDIN, STDOUT in temp variables
    int tempin = dup(0);
    int tempout = dup(1);
    
    vector<pid_t> bg_processes;

    string prev_dir = "";

    for (;;) {
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

        bool is_background = tknr.commands.at(0)->isBackground();

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
            continue;  // Skip to next iteration
        }
        // // print out every command token-by-token on individual lines
        // // prints to cerr to avoid influencing autograder
        // for (auto cmd : tknr.commands) {
        //     for (auto str : cmd->args) {
        //         cerr << "|" << str << "| ";
        //     }
        //     if (cmd->hasInput()) {
        //         cerr << "in< " << cmd->in_file << " ";
        //     }
        //     if (cmd->hasOutput()) {
        //         cerr << "out> " << cmd->out_file << " ";
        //     }
        //     cerr << endl;
        // }

        // fork to create child
        pid_t pid = fork();
        if (pid < 0) {  // error check
            perror("fork");
            exit(2);
        }
        if (pid == 0) {  // if child, exec to run command
        
            // Input redirection: command < input_file
            if (tknr.commands.at(0)->hasInput()) {
                int fdin = open(tknr.commands.at(0)->in_file.c_str(), O_RDONLY);
                if (fdin < 0) {
                    perror("open input file");
                    exit(2);
                }
                dup2(fdin, 0);  //make fd 0 point to the same place as fdin (the file). STDIN(0) now reads from the file
                close(fdin);    //now STDIN points to file, no need for fding
            }
            
            // Output redirection: command > output_file
            if (tknr.commands.at(0)->hasOutput()) {
                int fdout = open(tknr.commands.at(0)->out_file.c_str(), 
                            O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fdout < 0) {
                    perror("open output file");
                    exit(2);
                }
                dup2(fdout, 1);  // Redirect stdout to file
                close(fdout);
            }
            
            //build argv array and execute
            vector<char*> argv;
            for (const auto& arg : tknr.commands.at(0)->args) {
                argv.push_back((char*)arg.c_str());
            }
            argv.push_back(nullptr);
            
            if (execvp(argv[0], argv.data()) < 0) {
                perror("execvp");
                exit(2);
            }
        }
        else {  // if parent, wait for child to finish
            if (is_background) {
                // Don't wait, add to background list
                bg_processes.push_back(pid);
            } else {
                // Wait for foreground process
                int status = 0;
                waitpid(pid, &status, 0);
                if (status > 1) {
                    exit(status);
                }
            }
            
            //restore stdin and stdout
            dup2(tempin, 0);
            dup2(tempout, 1);
        }
    }
    return 0;
}
