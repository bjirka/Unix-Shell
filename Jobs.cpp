#include "Jobs.h"

using namespace std;

void Jobs::update(){
	for(int i = 0; i < jobNumbers.size(); ++i){
		if(status.at(i) != "Done"){
			int newStatus;
			int wpid = waitpid(pids.at(i), &newStatus, WNOHANG|WUNTRACED);
			if(wpid < 0){
				cout << "I think something went wrong" << endl;
			}else{
				if(wpid > 0){
					if(WIFEXITED(newStatus)){
						status.at(i) = "Done";
					}else if(WIFSTOPPED(newStatus)){
						status.at(i) = "Stopped";
					}else if(WIFCONTINUED(newStatus)){
						status.at(i) = "Continued";
					}
				}
			}
		}
	}
}

void Jobs::display(){
	for(int i = 0; i < jobNumbers.size(); ++i){
		cout << "[" << jobNumbers.at(i) << "] " << setfill(' ') << setw(25) << left << status.at(i) << " " << commands.at(i) << endl;
		if(status.at(i) == "Done"){
			removeJob(i);
			--i;
		}
	}
}

int Jobs::addJob(string command, int pid){
	commands.push_back(command);
	pids.push_back(pid);
	jobNumbers.push_back(nextJobNumber);
	status.push_back("Running");
	++nextJobNumber;
	return (nextJobNumber - 1);
}

void Jobs::removeJob(int index){
	commands.erase(commands.begin() + index);
	pids.erase(pids.begin() + index);
	jobNumbers.erase(jobNumbers.begin() + index);
	status.erase(status.begin() + index);
	if(pids.size() == 0) nextJobNumber = 1;
}

int Jobs::getPid(int index){
	return pids[index];
}

int Jobs::getJobNumber(int index){
	return jobNumbers[index];
}

string Jobs::getCommand(int index){
	return commands[index];
}

string Jobs::getStatus(int index){
	return status[index];
}

void Jobs::setStatus(int index, string newStatus){
	status[index] = newStatus;
}

int Jobs:: size(){
	return pids.size();
}
