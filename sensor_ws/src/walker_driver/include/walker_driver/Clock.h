#ifndef CLOCK_H
#define CLOCK_H

#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <sstream>




class Clock
{

#ifdef __MACH__
#include <sys/time.h>
#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 0
    //clock_gettime is not implemented on OSX
    int clock_gettime(int /*clk_id*/, struct timespec* t) {
        struct timeval now;
        int rv = gettimeofday(&now, NULL);
        if (rv) return rv;
        t->tv_sec  = now.tv_sec;
        t->tv_nsec = now.tv_usec * 1000;
        return 0;
    }
#endif

private:
    struct timespec init;
    struct timespec end;
    double initTime;
public:
    Clock(){

        clock_gettime(CLOCK_REALTIME, &init );
        initTime= init.tv_sec*1000.0 + init.tv_nsec/1000000.0;

    }

    void start(){

        clock_gettime(CLOCK_REALTIME, &init );
        //initTime= init.tv_sec*1000.0 + init.tv_nsec/1000000.0;
        tic();

    }
    double elapsed(){

        clock_gettime(CLOCK_REALTIME, &end );

        double dSeconds = (end.tv_sec - init.tv_sec)*1000.0;

        double dNanoSeconds = (double)( end.tv_nsec - init.tv_nsec ) / 1000000.0;

        return (dSeconds + dNanoSeconds);

    }



    void delay(unsigned long int ns){

        end=init;

        end.tv_nsec+=ns;

        while (end.tv_nsec >= 1000000000) {
            end.tv_nsec -= 1000000000;
            end.tv_sec++;
        }
#ifdef __MACH__
        nanosleep(&end, NULL);

#else

        clock_nanosleep(CLOCK_REALTIME,TIMER_ABSTIME,&end, NULL);
#endif
    }

    void delayr(unsigned long int ns){

        end=init;

        end.tv_nsec+=ns;

        while (end.tv_nsec >= 1000000000) {
            end.tv_nsec -= 1000000000;
            end.tv_sec++;
        }

#ifdef __MACH__
        nanosleep(&end, NULL);

#else

        clock_nanosleep(CLOCK_REALTIME,TIMER_ABSTIME,&end, NULL);
#endif
        init=end;

    }


   static void staticDelay(unsigned long int ns){

           struct timespec init1;
           struct timespec end1;

           clock_gettime(CLOCK_MONOTONIC, &init1 );

        end1=init1;

        end1.tv_nsec+=ns;

        while (end1.tv_nsec >= 1000000000) {
            end1.tv_nsec -= 1000000000;
            end1.tv_sec++;
        }

#ifdef __MACH__
        nanosleep(&end1, NULL);

#else

        clock_nanosleep(CLOCK_REALTIME,TIMER_ABSTIME,&end1, NULL);
#endif
        init1=end1;

    }



    void tic()
    {
        struct timeval first;
        struct timezone tzp;
        gettimeofday(&first, &tzp);

        initTime= first.tv_sec*1000.0 + first.tv_usec/1000.0;


    }

    double toc()
    {
        struct timeval first;
        struct timezone tzp;
        gettimeofday(&first, &tzp);

        double endTime= first.tv_sec*1000.0 + first.tv_usec/1000.0;
        return endTime-initTime;

    }


    static double getTimestamp(){

        struct timeval first;
        struct timezone tzp;
        double timestamp;

        gettimeofday(&first, &tzp);
        timestamp = first.tv_sec*1000.0 + first.tv_usec/1000.0;
        return timestamp;

    }

    static std::string getTimestampString(){

        struct timeval first;
        struct timezone tzp;
        double timestamp;

        gettimeofday(&first, &tzp);
        timestamp = first.tv_sec*1000.0 + first.tv_usec/1000.0;
        std::stringstream ss;
        ss.precision(24);
        ss<<timestamp;
        return ss.str();

    }

    static void printDate(){

        time_t t = time(NULL);
        tm* timePtr = localtime(&t);

        std::cout << "seconds = " << timePtr->tm_sec << std::endl;
        std::cout << "minutes = " << timePtr->tm_min << std::endl;
        std::cout << "hours = " << timePtr->tm_hour << std::endl;
        std::cout << "day of month = " << timePtr->tm_mday << std::endl;
        std::cout << "month of year = " << timePtr->tm_mon << std::endl;
        std::cout << "year = " << 1900+timePtr->tm_year << std::endl;
        std::cout << "weekday = " << timePtr->tm_wday << std::endl;
        std::cout << "day of year = " << timePtr->tm_yday << std::endl;
        std::cout << "daylight savings = " << timePtr->tm_isdst << std::endl;
    }

    static std::string date(std::string yearSeparator,std::string clockseparator){

        time_t t = time(NULL);
        tm* timedata = localtime(&t);

        std::stringstream cd;

        cd  <<1900+timedata->tm_year<<yearSeparator;
        cd << timedata->tm_mon<<yearSeparator;
        cd << timedata->tm_mday<<"_";
        cd << timedata->tm_hour<<clockseparator;
        cd << timedata->tm_min<<clockseparator;
        cd << timedata->tm_sec;

        return cd.str();
    }

    static double getGlobalTimestamp(){

        struct timeval first;
        struct timezone tzp;
        gettimeofday(&first, &tzp);
        return first.tv_sec*1000.0 + first.tv_usec/1000.0;

    }

};

#endif // CLOCK_H
