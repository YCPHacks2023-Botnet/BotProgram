#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include "rapidjson/document.h"
#include <sstream>
#include "nlohmann/json.hpp"
#include "worker.h"
#include "linux_functions.h"
#include <netdb.h>
#include <fstream>

using json = nlohmann::json;

Worker registerWorker(const char* serverIP, int serverPort, in_addr ipAddress, std::string cpu, std::string ramInfo) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "Failed to create socket." << std::endl;
        return Worker();
    }

    // Set the server address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(serverIP);
    serverAddress.sin_port = htons(serverPort); 

    // Connect to the server
    if (connect(sock, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == -1) {
        std::cerr << "Failed to connect to the server." << std::endl;
        close(sock);
        return Worker();
    }

    // Send an HTTP request to the API endpoint
    // Create the JSON data
    json requestData;
    requestData["cpu"] = cpu;
    requestData["ram"] = ramInfo;
    requestData["ip"] = inet_ntoa(ipAddress);

    // Serialize the JSON data to a string
    std::stringstream requestBodyStream;
    requestBodyStream << requestData;
    std::string requestBody = requestBodyStream.str();

    // Create the HTTP request
    std::string httpRequest = "POST /zombie/register HTTP/1.1\r\n"
        "Host: " + std::string(serverIP) + "\r\n"
        "Connection: close\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: " + std::to_string(requestBody.length()) + "\r\n"
        "\r\n"
        + requestBody;

    // Send the HTTP request
    if (send(sock, httpRequest.c_str(), httpRequest.length(), 0) == -1) {
        std::cerr << "Failed to send HTTP request." << std::endl;
        close(sock);
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

    // Find the start and end of the response data
    std::string responseData;
    size_t responseDataStart = response.find("{");
    size_t responseDataEnd = response.rfind("}");
    if (responseDataStart != std::string::npos && responseDataEnd != std::string::npos) {
        responseData = response.substr(responseDataStart, responseDataEnd - responseDataStart + 1);
    }

    // Parse the JSON response data
    rapidjson::Document document;
    document.Parse(responseData.c_str());

    if (document.HasParseError()) {
        std::cerr << "Failed to parse JSON." << std::endl;
        close(sock);
        return Worker(); 
    }

    // Process the JSON response data
    Worker worker;
    if (document.IsObject()) {
        if (document.HasMember("name") && document["name"].IsString()) {
            worker.setName(document["name"].GetString());
        }
        if (document.HasMember("id") && document["id"].IsInt()) {
            worker.setId(document["id"].GetInt());
        }
    }

    // Clean up
    close(sock);

    // Return the Worker object
    return worker;
}

in_addr getIpAddress() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        std::cerr << "Failed to get hostname." << std::endl;
        in_addr emptyAddress;
        emptyAddress.s_addr = INADDR_NONE;
        return emptyAddress; // Return an empty in_addr
    }

    struct hostent* host = gethostbyname(hostname);
    if (host == nullptr) {
        std::cerr << "Failed to get host information." << std::endl;
        in_addr emptyAddress;
        emptyAddress.s_addr = INADDR_NONE;
        return emptyAddress; // Return an empty in_addr
    }

    struct in_addr addr;
    memcpy(&addr, host->h_addr_list[0], sizeof(struct in_addr));

    std::cout << "IP Address: " << inet_ntoa(addr) << std::endl;

    return addr;
}

std::string getCPUModelName() {
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    std::string modelName;

    while (std::getline(cpuinfo, line)) {
        if (line.find("model name") != std::string::npos) {
            modelName = line.substr(line.find(":") + 2); // Extract the model name
            break;
        }
    }

    return modelName;
}

std::string getRAMInfo() {
    std::ifstream meminfo("/proc/meminfo");
    std::string line;
    std::string ramInfo;

    while (std::getline(meminfo, line)) {
        if (line.find("MemTotal") != std::string::npos) {
            ramInfo = line.substr(line.find(":") + 2); // Extract the total memory size
            break;
        }
    }

    // Convert KB to MB and strip off the "kB" suffix
    size_t pos = ramInfo.find(" kB");
    if (pos != std::string::npos) {
        ramInfo = ramInfo.substr(0, pos);
        int kbValue = std::stoi(ramInfo);
        int mbValue = kbValue / 1024;
        return std::to_string(mbValue);
    }

    return ramInfo;
}