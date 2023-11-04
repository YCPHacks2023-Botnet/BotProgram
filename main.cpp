#ifdef _WIN32
// Windows-specific code
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "rapidjson/document.h"
#include "worker.cpp"

int main() {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock." << std::endl;
        return 1;
    }

    // Create a socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Failed to create socket." << std::endl;
        WSACleanup();
        return 1;
    }
    //45.55.70.104:8080 /Management/ManagementTest
    // Set the server address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("10.127.24.32");
    serverAddress.sin_port = htons(3000); // Replace with the appropriate port number

    // Connect to the server
    if (connect(sock, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Failed to connect to the server." << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // Send an HTTP request to the API endpoint
    const char* httpRequest = "GET /api/workers HTTP/1.1\r\n"
        "Host: SERVER_IP_ADDRESS\r\n"
        "Connection: close\r\n"
        "\r\n";

    if (send(sock, httpRequest, strlen(httpRequest), 0) == SOCKET_ERROR) {
        std::cerr << "Failed to send HTTP request." << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
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

    // Find the start of the response data
    std::string responseData;
    size_t responseDataStart = response.find("\r\n\r\n");
    if (responseDataStart != std::string::npos) {
        responseData = response.substr(responseDataStart + 4);
    }

    // Print the response data
    std::cout << "Received response data:\n" << responseData << std::endl;

    // Parse the JSON response data
    rapidjson::Document document;
    document.Parse(responseData.c_str());

    if (document.HasParseError()) {
        std::cerr << "Failed to parse JSON." << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    Worker workerObject;


    if (document.IsArray()) {
        for (rapidjson::SizeType i = 0; i < document.Size(); i++) {
            const rapidjson::Value& worker = document[i];
            if (worker.HasMember("name") && worker["name"].IsString()) {
                std::string name = worker["name"].GetString();
                std::cout << "Name: " << name << std::endl;
                // Set the name of the Worker object
                workerObject.name = name;
            }
            if (worker.HasMember("ip") && worker["ip"].IsString()) {
                std::string ip = worker["ip"].GetString();
                std::cout << "Ip: " << ip << std::endl;
                // Set the IP of the Worker object
                workerObject.ip = ip;
            }
            if (worker.HasMember("id") && worker["id"].IsInt()) {
                int id = worker["id"].GetInt();
                std::cout << "Id: " << id << std::endl;
                // Set the ID of the Worker object
                workerObject.id = id;
            }
        }
    }

    std::cout << workerObject.name << std::endl;


    // Clean up
    closesocket(sock);
    WSACleanup();

    return 0;
}
#elif defined(__linux__)
// Linux-specific code
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include "rapidjson/document.h"

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "Failed to create socket." << std::endl;
        return 1;
    }

    // Set the server address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("10.127.24.32");
    serverAddress.sin_port = htons(3000); // Replace with the appropriate port number

    // Connect to the server
    if (connect(sock, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == -1) {
        std::cerr << "Failed to connect to the server." << std::endl;
        close(sock);
        return 1;
    }

    // Send an HTTP request to the API endpoint
    const char* httpRequest = "GET /api/workers HTTP/1.1\r\n"
        "Host: 10.127.24.32\r\n"
        "Connection: close\r\n"
        "\r\n";

    if (send(sock, httpRequest, strlen(httpRequest), 0) == -1) {
        std::cerr << "Failed to send HTTP request." << std::endl;
        close(sock);
        return 1;
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

    // Find the start of the response data
    std::string responseData;
    size_t responseDataStart = response.find("\r\n\r\n");
    if (responseDataStart != std::string::npos) {
        responseData = response.substr(responseDataStart + 4);
    }

    // Print the response data
    std::cout << "Received response data:\n" << responseData << std::endl;

    // Parse the JSON response data
    rapidjson::Document document;
    document.Parse(responseData.c_str());

    if (document.HasParseError()) {
        std::cerr << "Failed to parse JSON." << std::endl;
        close(sock);
        return 1;
    }

    if (document.IsArray()) {
        for (rapidjson::SizeType i = 0; i < document.Size(); i++) {
            const rapidjson::Value& worker = document[i];
            if (worker.HasMember("name") && worker["name"].IsString()) {
                std::string name = worker["name"].GetString();
                std::cout << "Name: " << name << std::endl;
            }
            if (worker.HasMember("ip") && worker["ip"].IsString()) {
                std::string ip = worker["ip"].GetString();
                std::cout << "Ip: " << ip << std::endl;
            }
            if (worker.HasMember("id") && worker["id"].IsInt()) {
                int id = worker["id"].GetInt();
                std::cout << "Id: " << id << std::endl;
            }
        }
    }

    // Clean up
    close(sock);

    return 0;
}
#endif