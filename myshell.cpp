#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
using namespace std;

// ----------------------------
// Function Declarations
// ----------------------------
void executeCommand(vector<string> args, bool background = false);
void handlePiping(vector<string> args);
void handleRedirection(vector<string> args);
void replaceEnvVars(vector<string>& args);
void handleExport(vector<string>& args);

// To track background jobs
vector<pid_t> backgroundJobs;

// ===========================================================
// Function: handleRedirection
// Handles >, >>, <, 2>, 2>>, &>
// ===========================================================
void handleRedirection(vector<string> args) {
    int fd;
    pid_t pid;
    pid = fork();

    if (pid == 0) {  // child process
        // Handle stdout overwrite >
        auto outPos = find(args.begin(), args.end(), ">");
        if (outPos != args.end()) {
            string filename = *(outPos + 1);
            fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);
            args.erase(outPos, outPos + 2);
        }

        // Handle stdout append >>
        auto appendPos = find(args.begin(), args.end(), ">>");
        if (appendPos != args.end()) {
            string filename = *(appendPos + 1);
            fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
            dup2(fd, STDOUT_FILENO);
            close(fd);
            args.erase(appendPos, appendPos + 2);
        }

        // Handle stdin <
        auto inPos = find(args.begin(), args.end(), "<");
        if (inPos != args.end()) {
            string filename = *(inPos + 1);
            fd = open(filename.c_str(), O_RDONLY);
            dup2(fd, STDIN_FILENO);
            close(fd);
            args.erase(inPos, inPos + 2);
        }

        // Handle stderr overwrite 2>
        auto errPos = find(args.begin(), args.end(), "2>");
        if (errPos != args.end()) {
            string filename = *(errPos + 1);
            fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            dup2(fd, STDERR_FILENO);
            close(fd);
            args.erase(errPos, errPos + 2);
        }

        // Handle stderr append 2>>
        auto errAppendPos = find(args.begin(), args.end(), "2>>");
        if (errAppendPos != args.end()) {
            string filename = *(errAppendPos + 1);
            fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
            dup2(fd, STDERR_FILENO);
            close(fd);
            args.erase(errAppendPos, errAppendPos + 2);
        }

        // Handle combined stdout+stderr &>
        auto bothPos = find(args.begin(), args.end(), "&>");
        if (bothPos != args.end()) {
            string filename = *(bothPos + 1);
            fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
            args.erase(bothPos, bothPos + 2);
        }

        // Build args for execvp
        vector<char*> execArgs;
        for (auto& a : args)
            execArgs.push_back(const_cast<char*>(a.c_str()));
        execArgs.push_back(NULL);

        execvp(execArgs[0], execArgs.data());
        perror("execvp failed");
        exit(1);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
    } else {
        perror("fork failed");
    }
}

// ===========================================================
// Function: handlePiping
// Handles multiple pipes (|)
// ===========================================================
void handlePiping(vector<string> args) {
    vector<vector<string>> commands;
    vector<string> current;

    // Split commands by '|'
    for (auto& arg : args) {
        if (arg == "|") {
            commands.push_back(current);
            current.clear();
        } else {
            current.push_back(arg);
        }
    }
    commands.push_back(current);

    int numPipes = commands.size() - 1;
    int pipefds[2 * numPipes];

    // Create all pipes
    for (int i = 0; i < numPipes; i++)
        if (pipe(pipefds + i * 2) < 0) perror("pipe failed");

    int cmdIndex = 0;
    for (auto& cmd : commands) {
        pid_t pid = fork();

        if (pid == 0) {
            // Redirect input from previous pipe
            if (cmdIndex > 0)
                dup2(pipefds[(cmdIndex - 1) * 2], STDIN_FILENO);
            // Redirect output to next pipe
            if (cmdIndex < numPipes)
                dup2(pipefds[cmdIndex * 2 + 1], STDOUT_FILENO);

            // Close all pipe fds
            for (int i = 0; i < 2 * numPipes; i++)
                close(pipefds[i]);

            // Build argv
            vector<char*> execArgs;
            for (auto& a : cmd)
                execArgs.push_back(const_cast<char*>(a.c_str()));
            execArgs.push_back(NULL);

            execvp(execArgs[0], execArgs.data());
            perror("execvp failed");
            exit(1);
        }
        cmdIndex++;
    }

    // Parent closes all pipes
    for (int i = 0; i < 2 * numPipes; i++)
        close(pipefds[i]);

    // Wait for all child processes
    for (int i = 0; i < commands.size(); i++)
        wait(NULL);
}

// ===========================================================
// Function: executeCommand
// Runs a normal (non-piped/non-redirected) command
// ===========================================================
void executeCommand(vector<string> args, bool background) {
    pid_t pid = fork();

    if (pid == 0) {
        vector<char*> execArgs;
        for (auto& a : args)
            execArgs.push_back(const_cast<char*>(a.c_str()));
        execArgs.push_back(NULL);

        execvp(execArgs[0], execArgs.data());
        perror("execvp failed");
        exit(1);
    } else if (pid > 0) {
        if (background) {
            backgroundJobs.push_back(pid);
            cout << "[Background PID " << pid << "]" << endl;
        } else {
            waitpid(pid, NULL, 0);
        }
    } else {
        perror("fork failed");
    }
}

// ===========================================================
// Function: replaceEnvVars
// Expands $VAR with environment variable values
// ===========================================================
void replaceEnvVars(vector<string>& args) {
    for (auto& arg : args) {
        if (!arg.empty() && arg[0] == '$') {
            char* val = getenv(arg.substr(1).c_str());
            if (val)
                arg = string(val);
            else
                arg = "";
        }
    }
}

// ===========================================================
// Function: handleExport
// Implements "export VAR=value"
// ===========================================================
void handleExport(vector<string>& args) {
    if (args.size() < 2) {
        cerr << "Usage: export VAR=value\n";
        return;
    }

    string expr = args[1];
    size_t pos = expr.find('=');
    if (pos == string::npos) {
        cerr << "Invalid export syntax\n";
        return;
    }

    string var = expr.substr(0, pos);
    string val = expr.substr(pos + 1);
    setenv(var.c_str(), val.c_str(), 1);
}

// ===========================================================
// MAIN SHELL LOOP
// ===========================================================
int main() {
    string input;
    cout << "Welcome to My Advanced Shell!" << endl;

    while (true) {
        cout << "mysh> ";
        getline(cin, input);
        if (input.empty()) continue;

        // Tokenize input
        stringstream ss(input);
        vector<string> args;
        string token;
        while (ss >> token)
            args.push_back(token);

        if (args.empty()) continue;

        // Built-ins
        if (args[0] == "exit") break;
        if (args[0] == "cd") {
            const char* path = (args.size() > 1) ? args[1].c_str() : getenv("HOME");
            if (chdir(path) != 0) perror("cd failed");
            continue;
        }
        if (args[0] == "export") {
            handleExport(args);
            continue;
        }
        if (args[0] == "jobs") {
            cout << "Active background jobs:\n";
            for (pid_t pid : backgroundJobs)
                cout << "  PID " << pid << endl;
            continue;
        }

        // Expand environment variables
        replaceEnvVars(args);

        // Background process check
        bool background = false;
        if (args.back() == "&") {
            background = true;
            args.pop_back();
        }

        // Piping / Redirection / Normal
        if (find(args.begin(), args.end(), "|") != args.end())
            handlePiping(args);
        else if (
            find(args.begin(), args.end(), ">") != args.end() ||
            find(args.begin(), args.end(), ">>") != args.end() ||
            find(args.begin(), args.end(), "<") != args.end() ||
            find(args.begin(), args.end(), "2>") != args.end() ||
            find(args.begin(), args.end(), "2>>") != args.end() ||
            find(args.begin(), args.end(), "&>") != args.end())
            handleRedirection(args);
        else
            executeCommand(args, background);
    }

    cout << "Goodbye!" << endl;
    return 0;
}	
