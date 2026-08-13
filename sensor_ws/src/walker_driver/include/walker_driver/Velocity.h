#ifndef VELOCITY_H
#define VELOCITY_H

#include "walker_driver/Robot.h"
#include <sstream>
class Velocity{
public:

    int leftVelocity;
    int rightVelocity;


    Velocity(){

        leftVelocity=0;
        rightVelocity=0;
    }
    Velocity(double linear,double angular,Robot ISRobot){

        double tleftVelocity=(linear-(ISRobot.b/2.0)*angular)/(ISRobot.R);
        double trightVelocity=(linear+(ISRobot.b/2.0)*angular)/(ISRobot.R);
        std::cout<<tleftVelocity<<"   "<<trightVelocity<<"   "<<std::endl;
        leftVelocity=limits(((tleftVelocity/(2.0*ISRobot.pi()))*60.0)*(ISRobot.MtoW),ISRobot);
        rightVelocity=limits(((trightVelocity/(2.0*ISRobot.pi()))*60.0)*(ISRobot.MtoW),ISRobot);
    }

    std::string toString(){
        std::stringstream str;

        str<< "Left :  "<<leftVelocity<<"   Right :   "<<rightVelocity;

        return str.str();

    }
private:
    int limits(double data,Robot ISRobot){

        int out=(int)data;
        if(data>ISRobot.limitMaxRPM){
            out=ISRobot.limitMaxRPM;
        }else if(data<ISRobot.limitMinRPM){
            out=ISRobot.limitMinRPM;
        }
        //std::cout<<"Antes Robot Conversion :"<<out<<std::endl;
        return RobotConversion(out,ISRobot);
    }
    int RobotConversion(int data,Robot ISRobot){

        if(data<0){
            return (-(data*1000)/ISRobot.MinRPM);
        }else{
            return  ((data*1000)/ISRobot.MaxRPM);
        }

    }

};

#endif // VELOCITY_H
