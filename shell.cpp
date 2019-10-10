#include <iostream>
#include <string>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <fcntl.h>
#include <ctime>
#include "Command.h"
#include "Jobs.h"

using namespace std;


bool readCmdLine(vector<string>* commands);
bool parseCmdLine(string commandString, Command* parsedCommand);

int main(int argc, char* argv[]){

	vector<string> commands;

	string oldpwd = "";

	Jobs jobs;

	int prevOut;

	prevOut = open("/dev/null", O_RDONLY);

	string subCommandOutput = "";

	while(readCmdLine(&commands)){

		Command command;

		// get the next command from the array of commands, parse it into program and arguments
		string cmd = commands.at(0);
		parseCmdLine(cmd, &command);

		vector<char*> argVector = command.getArguments();

		// create pipes for inner-process communication
		int pipeFD[2];
		pipe(pipeFD);

		// update the status of every background job
		jobs.update();

		// handle change of directly commands
		if(strcmp(argVector[0],"cd") == 0){
			// change to previous directory
			if(strcmp(argVector[1],"-") == 0){
				if(oldpwd != ""){
					char cwd[64];
					getcwd(cwd,64);
					if(chdir(oldpwd.c_str()) < 0){
						perror(oldpwd.c_str());
					}else{
						cout << oldpwd << endl;
						oldpwd = cwd;
					}
				}else{
					cout << "cd: OLDPWD not set" << endl;
				}
			// change to the given directory
			}else{
				char cwd[128];
				getcwd(cwd,128);
				if(chdir(argVector[1]) < 0){
					perror(argVector[1]);
				}else{
					oldpwd = cwd;
				}
			}

			commands.erase(commands.begin());
			continue;
		}

		// handle the 'jobs' command
		if(strcmp(argVector[0],"jobs") == 0){
			jobs.display();
			commands.erase(commands.begin());
			continue;
		}

		int pid = fork();
		if(pid == 0){
			if(command.getHasSubCommand()){
				// look in each argument and in/out file for '##$$##' and replace it with subCommandOutput
				bool subCommandFound = false;
				for(int i = 0; i < argVector.size() - 1; ++i){
					if(string(argVector.at(i)).find("##$$##") != string::npos){
						string tmp(argVector.at(i));
						tmp.replace(tmp.find("##$$##"),6,subCommandOutput);
						command.updateArgument(i, tmp);
						subCommandFound = true;
						break;
					}
				}
				if(!subCommandFound && command.getOutFile() != ""){
					string outFile = command.getOutFile();
					if(outFile.find("##$$##") != string::npos){
						outFile.replace(outFile.find("##$$##"),6,subCommandOutput);
						command.setOutFile(outFile);
						subCommandFound = true;
					}
				}
				if(!subCommandFound && command.getInFile() != ""){
					string inFile = command.getInFile();
					if(inFile.find("##$$##") != string::npos){
						inFile.replace(inFile.find("##$$##"),6,subCommandOutput);
						command.setInFile(inFile);
						subCommandFound = true;
					}
				}
			}
			if(command.getOutFile() != ""){
				int fd;
				if((fd = open(command.getOutFile().c_str(), O_WRONLY|O_CREAT, 0666)) < 0){
					perror(NULL);
				}
				dup2(fd,1);
			}else if(command.getOutPipe()){
				dup2(pipeFD[1],1);
			}else if(command.getLastSub()){
				//output to a pipe and the parent will read it and save it
				dup2(pipeFD[1],1);
			}

			if(command.getInFile() != ""){
				int fd;
				if((fd = open(command.getInFile().c_str(), O_RDONLY)) < 0){
					perror(NULL);
				}
				dup2(fd,0);
			}else if(command.getInPipe()){
				dup2(prevOut,0);
			}


			close(pipeFD[0]);
			close(pipeFD[1]);

			if(execvp(argVector[0], &argVector[0]) < 0){
				cout << argVector[0] << ": command not found" <<  endl;
				commands.clear();
				exit(0);
			}
		}else{
			if(!command.getBackground()){
				waitpid(pid, NULL, 0);
			}else{
				string tempCommand = "";
				for(int i = 0; i < argVector.size() - 1; ++i){
					tempCommand.append(argVector.at(i));
					tempCommand += " ";
				}
				int jobNum = jobs.addJob(tempCommand, pid);
				cout << "[" << jobNum << "] " << pid << endl;
			}

			// if this command was the last command in a $(...) then we need to get it's output from the pipe and store it for later
			if(command.getLastSub()){
				char buf[1024];
				memset(buf, '\0', 1024);
				read(pipeFD[0],buf,1024);
				string temp(buf);
				subCommandOutput = string(buf);
				// need to remove trailing newline character
				if(subCommandOutput.at(subCommandOutput.length()-1) == '\n'){
					subCommandOutput.pop_back();
				}
			}

			commands.erase(commands.begin());

			close(prevOut);
			prevOut = pipeFD[0];
			close(pipeFD[1]);
		}
	}

	return 0;
}

bool readCmdLine(vector<string>* commands){

	if(commands->size() == 0){

		string cmdLine;

		char cwd[64];
		getcwd(cwd, 64);
		char user[64];
		getlogin_r(user, 64);
		do{
			time_t t = time(NULL);
			tm* now = localtime(&t);
			cout << user << " | ";
			cout  << right << setfill('0') << setw(2) << (now->tm_mon + 1) << "/" << setfill('0') << setw(2) << (now->tm_mday) << "/" << setfill('0') << setw(2) << (now->tm_year - 100) << " ";
			cout  << setfill('0') << setw(2) << (now->tm_hour) << ":" << setfill('0') << setw(2) << (now->tm_min) << " | ";
			cout  << cwd << "> ";
			getline(cin, cmdLine);
		}while(cmdLine == "");

		if(cmdLine == "exit") exit(0);

		bool inString = false;
		int quoteStyle = -1; // 1 for single quote, 2 for double quote
		string subCommand = "";
		bool inSubCommand = false;

		string commandLine = "";

		// loop through the command given and extract a sub command (ex: "$(ls -l | grep out)") if one exists
		for(int i = 0; i < cmdLine.length(); ++i){
			if(cmdLine.at(i) == '$' && (cmdLine.length() - 1) > i && cmdLine.at(i+1) == '('){
				inSubCommand = true;
				++i;
				commandLine += "##$$##";
			}else if(inSubCommand){
				if(cmdLine.at(i) == ')' && !inString){
					inSubCommand = false;
				}else if(cmdLine.at(i) == '"' && (!inString || quoteStyle == 2)){
					if(inString){
						inString = false;
						quoteStyle = -1;
					}else{
						inString = true;
						quoteStyle = 2;
					}
					subCommand += cmdLine.at(i);
				}else if(cmdLine.at(i) == '\'' && (!inString || quoteStyle == 1)){
					if(inString){
						inString = false;
						quoteStyle = -1;
					}else{
						inString = true;
						quoteStyle = 1;
					}
					subCommand += cmdLine.at(i);
				}else{
					subCommand += cmdLine.at(i);
				}
			}else{
				commandLine += cmdLine.at(i);
			}
		}

		// append the subcommand to the begining of the commands
		if(subCommand != ""){
			commandLine = subCommand + "||" + commandLine;
		}


		inString = false;
		quoteStyle = -1;
		string tempCmd = "";

		// loop through the command line and split into individual commands on |
		for(int i = 0; i < commandLine.length(); ++i){
			if(commandLine.at(i) == '"' && (!inString || quoteStyle == 2)){
				if(inString){
					inString = false;
					quoteStyle = -1;
				}else{
					inString = true;
					quoteStyle = 2;
				}
				tempCmd += commandLine.at(i);
			}else if(commandLine.at(i) == '\'' && (!inString || quoteStyle == 1)){
				if(inString){
					inString = false;
					quoteStyle = -1;
				}else{
					inString = true;
					quoteStyle = 1;
				}
				tempCmd += commandLine.at(i);
			}else if(commandLine.at(i) == '|' && !inString){
				if(commandLine.at(i+1) == '|'){
					tempCmd += "||";
					commands->push_back(tempCmd);
					tempCmd = "";
					++i;
				}else{
					tempCmd += "|";
					commands->push_back(tempCmd);
					tempCmd = "|";
				}
			}else{
				if(commandLine.at(i) != ' ' || (tempCmd != "" && tempCmd != "|")){
					tempCmd += commandLine.at(i);
				}
			}
		}
		if(tempCmd != "") commands->push_back(tempCmd);

	}

	return true;
}

bool parseCmdLine(string commandString, Command* parsedCommand){

	enum PARSE_MODE {ARGS, INFILE, OUTFILE};

	string tempParsed = "";
	PARSE_MODE parse_mode = ARGS;
	bool inString = false;
	int quoteStyle = -1;

	// if command starts with a pipe then set inPipe flag
	if(commandString.at(0) == '|'){
		parsedCommand->setInPipe();
		commandString.erase(commandString.begin());
	}

	// if command ends with a pipe set outPipe flag, but if it ends with a double pipe then set lastSub flag
	if(commandString.at(commandString.size() - 1) == '|'){
		if(commandString.at(commandString.size() - 2) == '|'){
			parsedCommand->setLastSub();
			commandString.pop_back();
			commandString.pop_back();
		}else{
			parsedCommand->setOutPipe();
			commandString.pop_back();
		}
	}

	commandString += " ";

	if(commandString.find("##$$##") != string::npos){
		parsedCommand->setHasSubCommand();
	}

	for(int i = 0; i < commandString.length(); ++i){
		if(commandString.at(i) == '"' && (!inString || quoteStyle == 2 )){
			if(inString){
				inString = false;
				quoteStyle = -1;
			}else{
				inString = true;
				quoteStyle = 2;
			}
			tempParsed += commandString.at(i);
		}else if (commandString.at(i) == '\'' && (!inString || quoteStyle == 1)){
			if(inString){
				inString = false;
				quoteStyle = -1;
			}else{
				inString = true;
				quoteStyle = 1;
			}
			tempParsed += commandString.at(i);
		}else if(commandString.at(i) == ' ' && !inString){
			if(tempParsed != ""){
				if(parse_mode == ARGS){
					parsedCommand->addArgument(tempParsed);
				}else if(parse_mode == INFILE){
					parsedCommand->setInFile(tempParsed);
				}else if(parse_mode == OUTFILE){
					parsedCommand->setOutFile(tempParsed);
				}
				tempParsed = "";
			}
		}else if(commandString.at(i) == '>' && !inString){
			if(tempParsed != ""){
				if(parse_mode == ARGS){
					parsedCommand->addArgument(tempParsed);
				}else if(parse_mode == INFILE){
					parsedCommand->setInFile(tempParsed);
				}else if(parse_mode == OUTFILE){
					parsedCommand->setOutFile(tempParsed);
				}
				tempParsed = "";
			}
			parse_mode = OUTFILE;
		}else if(commandString.at(i) == '<' && !inString){
			if(tempParsed != ""){
				if(parse_mode == ARGS){
					parsedCommand->addArgument(tempParsed);
				}else if(parse_mode == INFILE){
					parsedCommand->setInFile(tempParsed);
				}else if(parse_mode == OUTFILE){
					parsedCommand->setOutFile(tempParsed);
				}
				tempParsed = "";
			}
			parse_mode = INFILE;
		}else if(commandString.at(i) == '&' && !inString){
			if(tempParsed != ""){
				if(parse_mode == ARGS){
					parsedCommand->addArgument(tempParsed);
				}else if(parse_mode == INFILE){
					parsedCommand->setInFile(tempParsed);
				}else if(parse_mode == OUTFILE){
					parsedCommand->setOutFile(tempParsed);
				}
			}
			parsedCommand->setBackground();
		}else{
			tempParsed += commandString.at(i);
		}
	}

	return true;
}
