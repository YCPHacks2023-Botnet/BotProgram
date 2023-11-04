#ifndef WORKER_H
#define WORKER_H

#include <iostream>

/*
Worker class is defined to build out the bots object from json
*/
class Worker {
public:
    int id;
    std::string ip;
    std::string name;
    std::string cpu;
    std::string ram;

    Worker() {
        // Default constructor
        // You can initialize the member variables to default values here
        id = 0;
        ip = "";
        name = "";
        cpu = "";
        ram = "";
    }

    void setId(int _id) {
        id = _id;
    }

    void setIp(const std::string& _ip) {
        ip = _ip;
    }

    void setName(const std::string& _name) {
        name = _name;
    }

    void setCpu(const std::string& _cpu) {
        cpu = _cpu;
    }

    void setRam(const std::string& _ram) {
        ram = _ram;
    }
};

#endif