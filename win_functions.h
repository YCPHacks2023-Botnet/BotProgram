#ifndef win_functions_h
#define win_functions_h

#include <string>
#include "worker.h"
#include <WinSock2.h>

Worker registerWorkerWin(const char* serverIP, int serverPort, in_addr ipAddress, std::string cpu, std::string ramInfo);
in_addr getIpAddress();
std::string getCPUInfo();
std::string getRAMInfo();

#endif
