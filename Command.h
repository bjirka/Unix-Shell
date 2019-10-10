#ifndef _COMMAND_H_
#define _COMMAND_H_
#include <string>
#include <vector>
#include <cstring>

using namespace std;

class Command{
private:
	vector<char*> arguments;
	string inFile = "";
	string outFile = "";
	bool inPipe = false;
	bool outPipe = false;
	bool isBackground = false;
	bool isLastSubCommand = false;
	bool hasSubCommand = false;
	int subCommandPosition = -1;
public:
	~Command();
	void addArgument(string argument);
	vector<char*> getArguments();
	void updateArgument(int index, string argument);
	void setInFile(string fileName);
	string getInFile();
	void setOutFile(string fileName);
	string getOutFile();
	void setInPipe();
	bool getInPipe();
	void setOutPipe();
	bool getOutPipe();
	void setBackground();
	bool getBackground();
	void setLastSub();
	bool getLastSub();
	void setHasSubCommand();
	bool getHasSubCommand();
};

#endif
