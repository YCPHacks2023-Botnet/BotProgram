#ifndef TASK_H
#define TASK_H

#include <iostream>

class TaskParameters {
public:
    std::string address;
    int interval;

    TaskParameters();
};

/*
Task class is defined to build out the task object from json
*/
class Task {
public:
    int id;
    std::string task;
    TaskParameters taskParams;

    Task();
};

enum class Command {
    REQUEST,
    CONTINUE,
    STOP
};

enum class Progress {
    SUCCESS,
    FAILURE,
    WORKING
};

enum class TaskOptions {
    DDOS,
    KEY_LOG,
    PORT_SCAN,
    QUACK,
    STORAGE,
    UNKNOWN
};

std::string progressToString(Progress progress);
std::string taskOptionsToString(TaskOptions taskOptions);
TaskOptions getTaskOption(const std::string& task);

#endif