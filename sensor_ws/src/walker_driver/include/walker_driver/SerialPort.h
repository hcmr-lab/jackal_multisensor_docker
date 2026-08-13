#ifndef SERIALPORT_H
#define SERIALPORT_H

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <math.h>
#include <poll.h>
#include <signal.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <unistd.h>
#include <pthread.h>//Threads
#include "walker_driver/Clock.h"


//template <class T>

class SerialPort
{
public:
    //! File descriptor
    int fd;
    //! Baud rate
    int baudRate;

    std::string port;

    pthread_t Thread;
    bool isThreaded;
    void (*Callback)(char *, int);




    //! Constructor
    SerialPort(){
        fd=-1;
        isThreaded=false;
    }

    SerialPort(SerialPort& cpy){

        std::cout<<"Copy Constructor"<<std::endl;
        fd=cpy.fd;
        isThreaded=cpy.isThreaded;
        Callback=cpy.Callback;
        baudRate=cpy.baudRate;
        port=cpy.port;
    }
    //! Destructor
    ~SerialPort(){
        isThreaded=false;
        ::close(fd);
    }


    bool open(std::string port_, int baudRaterate = 115200){

        baudRate=baudRaterate;
        port=port_;

        if(isOpen()) ::close(fd);

        fd = ::open(port_.c_str(), O_RDWR | O_NDELAY  | O_NONBLOCK | O_NOCTTY);


        if(fd == -1)
        {
            const char *extra_msg = "";
            switch(errno)
            {
            case EACCES:
                extra_msg = "You probably don't have premission to open the port for reading and writing.";
                break;

            case ENOENT:
                extra_msg = "The requested port does not exist. Is the hokuyo connected? Was the port name misspelled?";
                break;
            }
            //  printf( "Failed to open port: %s. %s (errno = %d). %s", port.c_str(), strerror(errno), errno, extra_msg);
            return false;
        }


        struct flock fl;
        fl.l_type = F_WRLCK;
        fl.l_whence = SEEK_SET;
        fl.l_start = 0;
        fl.l_len = 0;
        fl.l_pid = getpid();

        if(fcntl(fd, F_SETLK, &fl) != 0){

            //  printf("Device %s is already locked. Try 'lsof | grep %s' to find other processes that currently have the port open.", port.c_str(), port.c_str());
            ::close(fd);
            return false;

        }

        // Settings for USB?
        struct termios newtio;
        tcgetattr(fd, &newtio);
        memset (&newtio.c_cc, 0, sizeof (newtio.c_cc));
        newtio.c_cflag = CS8 | CLOCAL | CREAD;
        newtio.c_iflag = IGNPAR;
        newtio.c_oflag = 0;
        newtio.c_lflag = 0;
        cfsetspeed(&newtio, baudRaterate);
        baudRate = baudRaterate;

        // Activate new settings
        tcflush(fd, TCIFLUSH);
        if(tcsetattr(fd, TCSANOW, &newtio) < 0){
            //  printf( "Unable to set serial port attributes. The port you specified (%s) may not be a serial port.", port.c_str()); /// @todo tcsetattr returns true if at least one attribute was set. Hence, we might not have set everything on success.
            ::close(fd);
            return false;
        }


        //        int saved_flags = fcntl(fd, F_GETFL);
        //        fcntl(fd, F_SETFL, saved_flags & O_NONBLOCK);

        return true;
    }

    void close(){
        if(isOpen()){ ::close(fd);
            fd=-1;}
    }

    bool isOpen() { return fd != -1; }
    int getbaudRate() { return baudRate; }

    int write(const char * data, int length = -1){
        int len = length==-1 ? strlen(data) : length;

        // IO is currently non-blocking. This is what we want for the more cerealon read case.
        int origflags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, origflags & ~O_NONBLOCK); // TODO: @todo can we make this all work in non-blocking?
        int retval = ::write(fd, data, len);
        fcntl(fd, F_SETFL, origflags | O_NONBLOCK);

        if(retval == len){ return retval;
        }else{
            //  printf("write failed\n");
            return -1;
        }


    }
    int write(std::string data){

        //std::cout<<"On write : "<<data<<std::endl;
        int len = data.length();

        // IO is currently non-blocking. This is what we want for the more cerealon read case.
        int origflags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, origflags & ~O_NONBLOCK); // TODO: @todo can we make this all work in non-blocking?
        int retval = ::write(fd, data.c_str(), len);
        fcntl(fd, F_SETFL, origflags | O_NONBLOCK);

        flush();
        if(retval == len){ return retval;
        }else{
            ////  printf("write failed\n");
            return -1;
        }

    }

    int writeByte(char data){


        int len = 1;

        // IO is currently non-blocking. This is what we want for the more cerealon read case.
        int origflags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, origflags & ~O_NONBLOCK); // TODO: @todo can we make this all work in non-blocking?
        int retval = ::write(fd, &data, len);
        fcntl(fd, F_SETFL, origflags | O_NONBLOCK);

        if(retval == len){ return retval;
        }else{
            ////  printf("write failed\n");
            return -1;
        }

    }
    char readByte(){


        int ret;

        struct pollfd ufd[1];
        int retval;
        ufd[0].fd = fd;
        ufd[0].events = POLLIN;

        int timeout = 2000; //-1; // For compatibility with former behavior, 0 means no timeout. For poll, negative means no timeout.

        if((retval = poll(ufd, 1, timeout)) < 0){
              printf( "poll failed -- error = %d: %s\n", errno, strerror(errno));
            return '\0';
        }

        if(retval == 0){
              printf("Timeout reached\n");
            return '\0';
        }


        if(ufd[0].revents & POLLERR){

              printf("Error on socket, possibly unplugged\n");
            return '\0';
        }

        char buffer;
        ret = ::read(fd, &buffer, 1);

        if(ret == -1 && errno != EAGAIN && errno != EWOULDBLOCK){

              printf("Read failed\n");

            return '\0';
        }

        return buffer;

    }


    int unsafeRead(char * buffer, int max_length ){

        int ret;


        ret = ::read(fd, buffer, max_length);

        if(ret == -1 && errno != EAGAIN && errno != EWOULDBLOCK){


            return -1;
        }

        return ret;
    }

    int read(char * buffer, int max_length, int timeout = -1){

        int ret;

        struct pollfd ufd[1];
        int retval;
        ufd[0].fd = fd;
        ufd[0].events = POLLIN;

        if(timeout == 0) timeout = -1; // For compatibility with former behavior, 0 means no timeout. For poll, negative means no timeout.

        if((retval = poll(ufd, 1, timeout)) < 0){
            ////  printf( "poll failed -- error = %d: %s", errno, strerror(errno));
            return -1;
        }

        if(retval == 0){
            ////  printf("Timeout reached\n");
            return -1;
        }


        if(ufd[0].revents & POLLERR){

            ////  printf("Error on socket, possibly unplugged\n");
            return -1;
        }

        ret = ::read(fd, buffer, max_length);

        if(ret == -1 && errno != EAGAIN && errno != EWOULDBLOCK){

            ////  printf("Read failed\n");

            return -1;
        }

        return ret;
    }

    std::string read(int max_length){


        int timeout = -1;


        struct pollfd ufd[1];
        int retval;
        ufd[0].fd = fd;
        ufd[0].events = POLLIN;



        if(timeout == 0) timeout = -1; // For compatibility with former behavior, 0 means no timeout. For poll, negative means no timeout.

        if((retval = poll(ufd, 1, timeout)) < 0){
            ////  printf( "poll failed -- error = %d: %s", errno, strerror(errno));
            return "";
        }

        if(retval == 0){
            ////  printf("Timeout reached\n");
            return "";
        }

        if(ufd[0].revents & POLLERR){

            ////  printf("Error on socket, possibly unplugged\n");
            return "";
        }

        char *buffer;

        buffer=new char[max_length];
        int ret = ::read(fd, buffer, max_length);


        if(ret == -1 && errno != EAGAIN && errno != EWOULDBLOCK){

            ////  printf("Read failed\n");

            delete[] buffer;

            return "";
        }


        buffer[ret]='\0';
        std::string output(buffer);

        delete[] buffer;

        return output;


    }

    int readBytes(char * buffer, int length, int timeout = -1){

        int ret;
        int current = 0;

        struct pollfd ufd[1];
        int retval;
        ufd[0].fd = fd;
        ufd[0].events = POLLIN;

        if(timeout == 0) timeout = -1; // For compatibility with former behavior, 0 means no timeout. For poll, negative means no timeout.

        while(current < length)
        {
            if((retval = poll(ufd, 1, timeout)) < 0) {
                ////  printf( "Poll failed -- error = %d: %s\n", errno, strerror(errno));
                return 0;
            }

            if(retval == 0) {
                ////  printf( "Timeout reached\n");
                return 0;
            }

            if(ufd[0].revents & POLLERR){
                ////  printf( "Error on socket, possibly unplugged\n");
                return 0;
            }

            ret = ::read(fd, &buffer[current], length-current);

            if(ret == -1 && errno != EAGAIN && errno != EWOULDBLOCK){
                ////  printf( "Read failed\n");
                return 0;
            }

            current += ret;
        }
        return current;

    }
    int readLine(char * buffer, int length, int timeout = -1){
        int ret;
        int current = 0;

        struct pollfd ufd[1];
        int retval;
        ufd[0].fd = fd;
        ufd[0].events = POLLIN;

        if(timeout == 0) timeout = -1; // For compatibility with former behavior, 0 means no timeout. For poll, negative means no timeout.

        while(current < length-1)
        {
            if(current > 0)
                if(buffer[current-1] == '\n')
                    return current;
            if((retval = poll(ufd, 1, timeout)) < 0) {

                ////  printf( "Poll failed -- error = %d: %s \n", errno, strerror(errno));
                return 0;
            }

            if(retval == 0){
                ////  printf( "Timeout reached\n");
                return 0;
            }

            if(ufd[0].revents & POLLERR){
                ////  printf( "Error on socket, possibly unplugged\n");
                return 0;
            }

            ret = ::read(fd, &buffer[current], length-current);

            if(ret == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
                ////  printf( "Read failed\n");
                return 0;
            }

            current += ret;
        }
        ////  printf( "Buffer filled without end of line being found\n");
        return 0;
    }

    bool readLine(std::string * buffer, int timeout = -1){
        int ret;

        struct pollfd ufd[1];
        int retval;
        ufd[0].fd = fd;
        ufd[0].events = POLLIN;

        if(timeout == 0) timeout = -1; // For compatibility with former behavior, 0 means no timeout. For poll, negative means no timeout.

        buffer->clear();
        while(buffer->size() < buffer->max_size()/2)
        {
            // Look for the end char
            ret = buffer->find_first_of('\n');
            if(ret > 0)
            {
                // If it is there clear everything after it and return
                buffer->erase(ret+1, buffer->size()-ret-1);
                return true;
            }

            if((retval = poll(ufd, 1, timeout)) < 0) {
                ////  printf( "Poll failed -- error = %d: %s\n", errno, strerror(errno));
                return false;
            }

            if(retval == 0){
                ////  printf("Timeout reached\n");
                return false;
            }

            if(ufd[0].revents & POLLERR) {
                ////  printf( "Error on socket, possibly unplugged\n");
                return false;
            }

            char temp_buffer[128];
            ret = ::read(fd, temp_buffer, 128);

            if(ret == -1 && errno != EAGAIN && errno != EWOULDBLOCK){
                ////  printf( "Read failed\n");
            }

            // Append the new data to the buffer
            try{ buffer->append(temp_buffer, ret); }
            catch(std::length_error& le)
            {
                ////  printf( "Buffer filled without reaching end of data stream\n");
                return false;
            }
        }
        ////  printf( "Buffer filled without end of line being found\n");
        return false;
    }
    bool readBetween(std::string * buffer, char start, char end, int timeout = -1){

        int ret;

        struct pollfd ufd[1];
        int retval;
        ufd[0].fd = fd;
        ufd[0].events = POLLIN;

        if(timeout == 0) timeout = -1; // For compatibility with former behavior, 0 means no timeout. For poll, negative means no timeout.

        // Clear the buffer before we start
        buffer->clear();
        while(buffer->size() < buffer->max_size()/2)
        {
            if((retval = poll(ufd, 1, timeout)) < 0){
                ////  printf( "Poll failed -- error = %d: %s", errno, strerror(errno));
                return false;
            }

            if(retval == 0) {
                ////  printf("Timeout reached\n");
                return false;
            }

            if(ufd[0].revents & POLLERR) {
                ////  printf( "Error on socket, possibly unplugged\n");
                return false;
            }

            char temp_buffer[128];
            ret = ::read(fd, temp_buffer, 128);

            if(ret == -1 && errno != EAGAIN && errno != EWOULDBLOCK){
                ////  printf("Read failed\n");
                return false;
            }

            // Append the new data to the buffer
            try{ buffer->append(temp_buffer, ret); }
            catch(std::length_error& le)
            {
                ////  printf( "Buffer filled without reaching end of data stream\n");
                return false;
            }

            // Look for the start char
            ret = buffer->find_first_of(start);
            // If it is not on the buffer, clear it
            if(ret == -1) buffer->clear();
            // If it is there, but not on the first position clear everything behind it
            else if(ret > 0) buffer->erase(0, ret);

            // Look for the end char
            ret = buffer->find_first_of(end);
            if(ret > 0)
            {
                // If it is there clear everything after it and return
                buffer->erase(ret+1, buffer->size()-ret-1);
                return true;
            }
        }
        ////  printf( "Buffer filled without reaching end of data stream\n");
    }

    bool readTill(std::string * buffer, char end, int timeout = -1){

        int ret;

        struct pollfd ufd[1];
        int retval;
        ufd[0].fd = fd;
        ufd[0].events = POLLIN;

        if(timeout == 0) timeout = -1; // For compatibility with former behavior, 0 means no timeout. For poll, negative means no timeout.

        // Clear the buffer before we start
        buffer->clear();
        while(buffer->size() < buffer->max_size()/2)
        {
            if((retval = poll(ufd, 1, timeout)) < 0){
                ////  printf( "Poll failed -- error = %d: %s", errno, strerror(errno));
                return false;
            }

            if(retval == 0) {
                ////  printf("Timeout reached\n");
                return false;
            }

            if(ufd[0].revents & POLLERR) {
                ////  printf( "Error on socket, possibly unplugged\n");
                return false;
            }

            char temp_buffer[128];
            ret = ::read(fd, temp_buffer, 128);

            if(ret == -1 && errno != EAGAIN && errno != EWOULDBLOCK){
                ////  printf("Read failed\n");
                return false;
            }

            // Append the new data to the buffer
            try{ buffer->append(temp_buffer, ret); }
            catch(std::length_error& le)
            {
                ////  printf( "Buffer filled without reaching end of data stream\n");
                return false;
            }


            // Look for the end char
            int ret = buffer->find_first_of(end);
            if(ret > 0)
            {
                // If it is there clear everything after it and return
                buffer->erase(ret+1, buffer->size()-ret-1);
                return true;
            }
        }
        ////  printf( "Buffer filled without reaching end of data stream\n");
    }
    int flush(){
        int retval = tcflush(fd, TCIOFLUSH);
        if(retval != 0){
            ////  printf( "tcflush failed\n");
            return -1;
        }
        return retval;
    }


    void * ReadThread(void){

        struct pollfd ufd[1];
        ufd[0].fd = fd;
        ufd[0].events = POLLIN;


        char buffer[128];


        while(isThreaded)
        {
            //std::cout<<" -pool  :"<<std::endl;
            //std::cout<<" ReadThread  :"<<ufd[0].fd<<"  "<<ufd[0].events<<std::endl;

            //double tc=0;
            // if(ppoll(ufd, 1, &init,NULL) > 0)
            // Clock clk;
            // clk.tic();

            //                struct timespec init;
            //                clock_gettime(CLOCK_MONOTONIC, &init );
            //                init.tv_nsec+=5000000;

            //                while (init.tv_nsec >= 1000000000) {
            //                    init.tv_nsec -= 1000000000;
            //                    init.tv_sec++;
            //                }

            if(poll(ufd, 1,5) > 0)
            {
                //std::cout<<" +pool  :"<<std::endl;
                //perror(" +pool  :");

                if(!(ufd[0].revents & POLLERR))
                {

                    int dt = unsafeRead(buffer, 128 );
                    if(dt>0)
                    {
                        (Callback)(buffer, dt);
                    }
                    //std::cout<<" end  "<<clk.toc()<<std::endl;
                }
            }

            //Clock::staticDelay(2000000);
            //perror(" -pool  :");


        }
        pthread_exit(NULL);
    }


    static void *threadConverter(void *context)
    {
        return static_cast<SerialPort*>(context)->ReadThread();
    }


    void startReadStream( void (*readCallback)(char *, int)){

        //  char data[MAX_LENGTH];
        isThreaded=true;
        Callback=readCallback;

        //std::cout<<fd<<std::endl;

        //std::cout<<"Callback  "<<&Callback<<" readCallback  "<<&readCallback<<std::endl;

        pthread_t tempid;
        pthread_attr_t atributes; //atributos da thread
        pthread_attr_init(&atributes); //inicializa a zero
        pthread_create(&tempid,&atributes,threadConverter,this);


    }

};

#endif // SERIALPORT_H
