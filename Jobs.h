#ifndef _JOBS_H_
#define _JOBS_H_
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <sys/wait.h>

using namespace std;

class Jobs{
private:
	int nextJobNumber = 1;
	vector<string> commands;
	vector<int> pids;
	vector<int> jobNumbers;
	vector<string> status;
public:
	void update();
	void display();
	int addJob(string command, int pid);
	void removeJob(int index);
	int getPid(int index);
	int getJobNumber(int index);
	string getCommand(int index);
	string getStatus(int index);
	void setStatus(int index, string newStatus);
	int size();
};

#endif
