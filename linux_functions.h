#ifndef linux_functions_h
#define linux_functions_h

#include <string>
#include "worker.h"

Worker registerWorker(const char* serverIP, int serverPort);

#endif
