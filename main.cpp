// Define constants for server IP address and port number
const char* SERVER_IP = "45.55.70.104";
const int SERVER_PORT = 8080;
//45.55.70.104:8080/Zombie/ZombieTest
#ifdef _WIN32
// Windows-specific code
#include <iostream>
#include "win_functions.h"
#include "worker.h"

int main() {
    Worker worker = registerWorkerWin(SERVER_IP, SERVER_PORT);

    // Check if the worker registration was successful
    if (worker.id != 0) {
        std::cout << "Worker registered successfully!" << std::endl;
        std::cout << "Worker ID: " << worker.id << std::endl;
        std::cout << "Worker Name: " << worker.name << std::endl;
    }
    else {
        std::cerr << "Failed to register worker." << std::endl;
    }

    return 0;
}
#elif defined(__linux__)
// Linux-specific code
#include <iostream>
#include "linux_functions.h"
#include "worker.h"

int main() {
    Worker worker = registerWorker(SERVER_IP, SERVER_PORT);

    // Check if the worker registration was successful
    if (worker.id != 0) {
        std::cout << "Worker registered successfully!" << std::endl;
        std::cout << "Worker ID: " << worker.id << std::endl;
        std::cout << "Worker Name: " << worker.name << std::endl;
    }
    else {
        std::cerr << "Failed to register worker." << std::endl;
    }

    return 0;
}
#endif