#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <vector>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <sstream>

#include "walker_driver/Clock.h"
#include "walker_driver/SerialPort.h"

class HDC2450Controller
{
public:
    std::string USBport;
    unsigned int baudrate;
    SerialPort serial;
    int commandTimeout;
    Clock clock;

    std::string lastReceived;
    std::string lastQuery;
    std::string lastError;

    bool invalidtry = false;

    HDC2450Controller() {}

    HDC2450Controller(std::string port, unsigned int baud)
    {
        USBport = port;
        baudrate = baud;
        commandTimeout = 5;
    }

    ~HDC2450Controller()
    {
        if(serial.isOpen()){
            serial.close();
        }
    }

    std::string getlastError(){
        return "For Query < " + remove(remove(lastQuery, '\r')) + " > Received < " + lastReceived + " > Last detected Error :  " + lastError;
    }

    bool invalidInit(){
        return invalidtry;
    }

    bool ping()
    {
        serial.writeByte(0x05);
        char data = serial.readByte();
        return (data == 0x06);
    }

    bool dataFilter(std::string data)
    {
        return data.find_first_of('=') != std::string::npos;
    }

    bool error(std::string data)
    {
        return data.find_first_of('-') == 0;
    }

    bool errorOnCommand(std::string data)
    {
        return data.find_first_of('-') != std::string::npos;
    }

    bool ok(std::string data)
    {
        return data.find_first_of('+') != std::string::npos;
    }

    bool open(size_t watchdog_time = 500, bool echo = false)
    {
        invalidtry = false;

        if (serial.open(USBport.c_str(), baudrate) == true)
        {
            serial.flush();
            
            if (!this->ping())
            {
                invalidtry = true;
                return false;
            }

            this->setEcho(echo);
            this->setWatchdog(watchdog_time);

            return true;
        }
        return false;
    }

    void close()
    {
        serial.close();
    }

    std::string issueQuery(const std::string &query)
    {
        lastQuery = query;

        if (serial.isOpen() == true)
        {
            serial.flush();
            serial.write(query.c_str());
            std::string databuffer;

            clock.start();
            databuffer = serial.read(100);
            databuffer = remove(databuffer, '\r');
            lastError = "";

            while (clock.toc() <= commandTimeout)
            {
                if (error(databuffer) == false)
                {
                    if (dataFilter(databuffer) == true && contains(&databuffer, '*'))
                    {
                        lastReceived = remove(databuffer);
                        return lastReceived;
                    }
                    else
                    {
                        databuffer = remove(databuffer + serial.read(100), '\r');
                    }
                }
                else
                {
                    lastReceived = remove(databuffer);
                    lastError = "Bad request ( Received - )";
                    return lastReceived;
                }
            }
            
            lastError = "System timeout";
            lastReceived = remove(databuffer);
            return lastReceived;
        }
        
        lastError = "Error opening port";
        lastReceived = "";
        return "";
    }

    bool issueCommand(const std::string &command)
    {
        lastQuery = command;
        lastError = "";

        if (serial.isOpen() == true)
        {
            serial.flush();
            serial.write(command.c_str());
            std::string databuffer;

            clock.start();
            databuffer = serial.read(20);
            while (clock.elapsed() <= commandTimeout)
            {
                lastReceived = remove(remove(databuffer, '\r'));

                if (ok(databuffer) == true)
                {
                    return true;
                }
                else if (errorOnCommand(databuffer) == true)
                {
                    lastError = "Bad request";
                    return false;
                }
                else
                {
                    databuffer = serial.read(20);
                }
            }
            
            lastReceived = remove(remove(databuffer, '\r'));
            lastError = "System timeout";
            return false;
        }
        
        lastReceived = "";
        lastError = "Error opening port";
        return false;
    }

    bool setEcho(bool echo)
    {
        std::stringstream value;
        value << "^ECHOF " << (int)echo << "\r";
        return issueCommand(value.str());
    }

    bool setWatchdog(int watch)
    {
        std::stringstream value;
        value << "^RWD " << watch << "\r";
        return issueCommand(value.str());
    }

    bool setEncoderCounters(int channel, int val)
    {
        std::stringstream value;
        value << "!C " << channel << " " << val << "\r";
        return issueCommand(value.str());
    }

    bool setMotorCommand(int Motor1, int Motor2)
    {
        std::stringstream value;
        if (Motor1 <= 1000 && Motor1 >= -1000){
            if (Motor2 <= 1000 && Motor2 >= -1000){
                value << "!M " << Motor1 << " " << Motor2 << "\r";
                return issueCommand(value.str());
            }
        }
        return false;
    }

    bool releaseShutdown()
    {
        return issueCommand("!MG\r");
    }

    bool setShutdown()
    {
        return issueCommand("!EX\r");
    }

    bool setSpeed(int Motor1, int Motor2)
    {
        std::stringstream value;
        if (Motor1 <= 1000 && Motor1 >= -1000){
            if (Motor2 <= 1000 && Motor2 >= -1000){
                value << "!G " << Motor1 << " " << Motor2 << "\r";
                return issueCommand(value.str());
            }
        }
        return false;
    }

    std::vector<int> parser(std::string data) {
        std::vector<int> output;
        
        std::string result = data.substr(data.find_first_of('=') + 1);
        size_t npos = 0; // Changed to size_t to fix signedness warning
        
        while(npos != std::string::npos){
            npos = result.find_first_of(':');
            output.push_back(atoi(result.substr(0, npos).c_str()));
            result = result.substr(npos + 1).c_str();
        }

        return output;
    }

    std::vector<int> readMotorAmps()
    {
        std::string result = issueQuery("?A\r");
        std::vector<int> output;

        if (!result.empty()) {
            output = parser(result);
        } else {
            output.push_back(INT_MAX);
            output.push_back(INT_MAX);
        }
        return output;
    }

    std::vector<int> readEncoderSpeedRPM()
    {
        std::string result = issueQuery("?S\r");
        std::vector<int> output;

        if (!result.empty()) {
            output = parser(result);
        } else {
            output.push_back(INT_MAX);
            output.push_back(INT_MAX);
        }
        return output;
    }

    std::vector<int> readBatteryAmps()
    {
        std::string result = issueQuery("?BA\r");
        std::vector<int> output;

        if (!result.empty()) {
            output = parser(result);
        } else {
            output.push_back(INT_MAX);
            output.push_back(INT_MAX);
        }
        return output;
    }

    std::vector<int> readAbsoluteEncoderCount()
    {
        std::string result = issueQuery("?C\r");
        std::vector<int> output;

        if (!result.empty()) {
            output = parser(result);
        } else {
            output.push_back(INT_MAX);
            output.push_back(INT_MAX);
        }
        return output;
    }

    std::vector<int> readEncoderCountRelative()
    {
        std::string result = issueQuery("?CR\r");
        std::vector<int> output;

        if (!result.empty()) {
            output = parser(result);
        } else {
            output.push_back(0);
            output.push_back(0);
        }
        return output;
    }

    std::vector<int> readClosedLoopError()
    {
        std::string result = issueQuery("?E\r");
        std::vector<int> output;

        if (!result.empty()) {
            output = parser(result);
        } else {
            output.push_back(INT_MAX);
            output.push_back(INT_MAX);
        }
        return output;
    }

    std::string readFirmwareIDString()
    {
        return issueQuery("?FID\r");
    }

    std::vector<int> readActualMotorCommand()
    {
        std::string result = issueQuery("?M\r");
        std::vector<int> output;

        if (!result.empty()) {
            output = parser(result);
        } else {
            output.push_back(INT_MAX);
            output.push_back(INT_MAX);
        }
        return output;
    }

    std::vector<int> readAppliedPowerLevel()
    {
        std::string result = issueQuery("?P\r");
        std::vector<int> output;

        if (!result.empty()) {
            output = parser(result);
        } else {
            output.push_back(INT_MAX);
            output.push_back(INT_MAX);
        }
        return output;
    }

    std::vector<int> readInternalVoltages()
    {
        std::string result = issueQuery("?V\r");
        std::vector<int> output;

        if (!result.empty()) {
            output = parser(result);
        } else {
            output.push_back(INT_MAX);
            output.push_back(INT_MAX);
        }
        return output;
    }

    std::string remove(std::string str, char test){
        std::stringstream testStr;
        // Changed to size_t to fix signedness warning
        for(size_t i = 0; i < str.length(); i++){
            if(str[i] >= 32){
                testStr << str[i];
            } else if(str[i] == test){
                testStr << '*';
            }
        }
        return testStr.str();
    }

    std::string remove(std::string str){
        std::stringstream testStr;
        // Changed to size_t to fix signedness warning
        for(size_t i = 0; i < str.length(); i++){
            if(str[i] >= 32){
                if(str[i] != '*'){
                    testStr << str[i];
                }
            }
        }
        return testStr.str();
    }

    bool contains(std::string *str, char test){
        // Changed to size_t to fix signedness warning
        for(size_t i = 0; i < str->length(); i++){
            if(str->at(i) == test){
                return true;
            }
        }
        return false;
    }

    // Standard getters and setters moved securely inside the class definition
    unsigned int getBaudrate() const {
        return baudrate;
    }

    void setBaudrate(unsigned int value) {
        baudrate = value;
    }

    int getCommandTimeout() const {
        return commandTimeout;
    }

    void setCommandTimeout(int value) {
        commandTimeout = value;
    }

    std::string getUSBport() const {
        return USBport;
    }

    void setUSBport(const std::string &value) {
        USBport = value;
    }
};

#endif // CONTROLLER_H
