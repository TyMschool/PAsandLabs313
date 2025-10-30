#include <iostream>
#include "Tokenizer.h"

using namespace std;

Tokenizer::Tokenizer (const string _input) {
    error = false;
    input = trim(_input);
    split();
}

Tokenizer::~Tokenizer () {
    for (auto cmd : commands) {
        delete cmd;
    }
    commands.clear();
}

bool Tokenizer::hasError () {
    return error;
}

string Tokenizer::trim (const string in) {
    int i = in.find_first_not_of(" \n\r\t");
    int j = in.find_last_not_of(" \n\r\t");

    if (i >= 0 && j >= i) {
        return in.substr(i, j-i+1);
    }
    return in;
}

void Tokenizer::split() {
    string temp = input;
    
    // Handle quoted strings (keep existing code)
    int index = 0;
    while (temp.find("\"") != string::npos || temp.find("\'") != string::npos) {
        int start = 0;
        int end = 0;
        if (temp.find("\"") != string::npos
            && (temp.find("\'") == string::npos || temp.find("\"") < temp.find("\'"))) {
            start = temp.find("\"");
            end = temp.find("\"", start+1);
            if ((size_t) end == string::npos) {
                error = true;
                cerr << "Invalid command - Non-matching quotation mark on \"" << endl;
                return;
            }
        }
        else if (temp.find("\'") != string::npos) {
            start = temp.find("\'");
            end = temp.find("\'", start+1);
            if ((size_t) end == string::npos) {
                error = true;
                cerr << "Invalid command - Non-matching quotation mark on \'" << endl;
                return;
            }
        }
        
        inner_strings.push_back(temp.substr(start+1, end-start-1));

        string str_beg = temp.substr(0, start);
        string str_mid = " --str " + to_string(index) + " "; 
        string str_end = temp.substr(end+1);
        temp = str_beg + str_mid + str_end;

        index++;
    }

    vector<string> and_groups;
    size_t and_pos = 0;
    while ((and_pos = temp.find("&&")) != string::npos) {
        and_groups.push_back(trim(temp.substr(0, and_pos)));
        temp = trim(temp.substr(and_pos + 2));
    }
    and_groups.push_back(trim(temp));  //add the last group
    
    // process each && group and split by pipes
    for (size_t group_idx = 0; group_idx < and_groups.size(); group_idx++) {
        string group = and_groups[group_idx];
        
        // split by pipes
        vector<string> pipe_commands;
        size_t pipe_pos = 0;
        while ((pipe_pos = group.find("|")) != string::npos) {
            pipe_commands.push_back(trim(group.substr(0, pipe_pos)));
            group = trim(group.substr(pipe_pos + 1));
        }
        pipe_commands.push_back(trim(group));  //add the last group
        
        //create command objects for each pipe command
        for (size_t cmd_idx = 0; cmd_idx < pipe_commands.size(); cmd_idx++) {
            Command* cmd = new Command(trim(pipe_commands[cmd_idx]), inner_strings);
            
            //determine separator
            if (cmd_idx < pipe_commands.size() - 1) {
                cmd->separator = "|";
            } else if (group_idx < and_groups.size() - 1) {
                cmd->separator = "&&";
            } else {
                cmd->separator = "";
            }
            
            commands.push_back(cmd);
        }
    }
}