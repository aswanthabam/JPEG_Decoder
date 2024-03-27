#include <iostream>
#include <string>
using namespace std;

enum LogType
{
    INFO,
    WARNING,
    ERROR
};

class Logger
{
    string message;
    int level = 0;
    int type = LogType::INFO;
    bool done = false;

public:
    Logger operator<<(string message)
    {

        this->message += message;
        return *this;
    }
    Logger operator<<(int message)
    {

        this->message += to_string(message);
        return *this;
    }
    Logger operator<<(const char *message)
    {

        this->message += message;
        return *this;
    }
    Logger operator<<(const char message)
    {

        this->message += message;
        return *this;
    }
    Logger operator<<(string *message)
    {

        this->message += *message;
        return *this;
    }
    Logger operator[](int level)
    {
        this->level = level;
        this->message = "";
        return *this;
    }
    Logger operator>>(ostream &out)
    {
        for (int i = 0; i < level; i++)
            out << "\t";
        if (type == LogType::INFO)
            out << "-> ";
        else if (type == LogType::WARNING)
            out << "WARNING: ";
        else if (type == LogType::ERROR)
            out << "ERROR: ";
        out << message << endl;
        this->done = true;
        return Logger();
    }
    Logger operator|(LogType type)
    {
        this->type = type;
        return *this;
    }
    Logger operator()(LogType type)
    {
        this->type = type;
        return *this;
    }
};

namespace logger
{
    Logger show;
}