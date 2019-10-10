#include "Command.h"

using namespace std;

void Command::addArgument(string argument){
	// remove outer quotation marks
	if(argument.at(0) == '"' || argument.at(0) == '\'') argument.erase(argument.begin());
	if(argument.at(argument.size() - 1) == '"' || argument.at(argument.size() -1) == '\'') argument.pop_back();

	char* cArg = (char*)malloc(argument.length());
	strcpy(cArg, argument.c_str());
	arguments.push_back(cArg);
}

vector<char*> Command::getArguments(){
	arguments.push_back(NULL);
	return arguments;
}

void Command::updateArgument(int index, string argument){
	if(index >= arguments.size()) return;

	free(arguments.at(index));

	char* cArg = (char*)malloc(argument.length());
	strcpy(cArg, argument.c_str());
	arguments.at(index) = cArg;
}

void Command::setInFile(string fileName){
	inFile = fileName;
}

string Command::getInFile(){
	return inFile;
}

void Command::setOutFile(string fileName){
	outFile = fileName;
}

string Command::getOutFile(){
	return outFile;
}

void Command::setInPipe(){
	inPipe = true;
}

bool Command::getInPipe(){
	return inPipe;
}

void Command::setOutPipe(){
	outPipe = true;
}

bool Command::getOutPipe(){
	return outPipe;
}

void Command::setBackground(){
	isBackground = true;
}

bool Command::getBackground(){
	return isBackground;
}

void Command::setLastSub(){
	isLastSubCommand = true;
}

bool Command::getLastSub(){
	return isLastSubCommand;
}

void Command::setHasSubCommand(){
	hasSubCommand = true;
}

bool Command::getHasSubCommand(){
	return hasSubCommand;
}

Command::~Command(){
	for(int i = 0; i < arguments.size(); ++i){
		free(arguments.at(i));
	}
}
