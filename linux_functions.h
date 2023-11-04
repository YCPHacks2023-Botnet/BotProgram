#ifndef linux_functions_h
#define linux_functions_h

#include <string>
#include "worker.h"
#include <netdb.h>

Worker registerWorker(const char* serverIP, int serverPort, in_addr ipAddress, std::string cpu, std::string ramInfo);
in_addr getIpAddress();
std::string getCPUModelName();
std::string getRAMInfo();

#endif
