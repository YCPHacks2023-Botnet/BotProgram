#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "rapidjson/document.h"
#include "worker.h"
#include <sstream>
#include "nlohmann/json.hpp"
#include "win_functions.h"

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