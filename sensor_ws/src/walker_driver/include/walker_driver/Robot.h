
//Revisao a 30/Mar/2016:  New config


#ifndef ROBOT_H
#define ROBOT_H
#include "walker_driver/Pose.h"
//#include "walker_driver/Velocity.h"


class Robot {

public:

    Pose<double> Odometry;
    //Velocity Actual;
    double PulsesPerWheelRevolution;
    double b;
    double R;
    double MtoW;
    double WtoM;
    int limitMaxRPM;
    int limitMinRPM;
    int MaxRPM;
    int MinRPM;
    double Vx;
    double Vy;
    double W;

    bool swapLeftRight;
    bool invertLeft;
    bool invertRight;
    bool swapEncoders;


    void print(){

        std::cout<<"PulsesPerWheelRevolution"<<"  "<<PulsesPerWheelRevolution<<std::endl;
        std::cout<<"b"<<"  "<< b<<std::endl;
        std::cout<<"R"<<"  "<< R<<std::endl;
        std::cout<<"MtoW"<<"  "<<MtoW<<std::endl;
        std::cout<<"WtoM"<<"  "<<WtoM<<std::endl;
        std::cout<<"limitMaxRPM"<<"  "<<limitMaxRPM<<std::endl;
        std::cout<<"limitMinRPM"<<"  "<<limitMinRPM<<std::endl;
        std::cout<<"MaxRPM"<<"  "<<MaxRPM<<std::endl;
        std::cout<<"MinRPM"<<"  "<<MinRPM<<std::endl;
        std::cout<<"Vx"<<"  "<<Vx<<std::endl;
        std::cout<<"Vy"<<"  "<<Vy<<std::endl;
        std::cout<<"W"<<"  "<<W<<std::endl;
        std::cout<<"swapLeftRight"<<"  "<<swapLeftRight<<std::endl;
        std::cout<<"invertLeft"<<"  "<<invertLeft<<std::endl;
        std::cout<<"invertRight"<<"  "<<invertRight<<std::endl;
        std::cout<<"swapEncoders"<<"  "<<swapEncoders<<std::endl;


    }




    double pi(){
        return 3.141592653589793;
    }



    Robot(double PulsesPerWheelRevolution_,double MtoW_,double WtoM_,double b_,double R_,int limitMaxRPM_,int limitMinRPM_,int MaxRPM_,int MinRPM_){

        PulsesPerWheelRevolution=PulsesPerWheelRevolution_ ;  //500*4*10; //500*4 por causa da quadratura
        MtoW=MtoW_;
        WtoM=WtoM_;
        b=b_;  // dizem ser 60cm, mas medi 59,5cm
        R=R_;
        limitMaxRPM=limitMaxRPM_; //para ja limitar a 50%+-  ??
        limitMinRPM=limitMinRPM_;
        MaxRPM=MaxRPM_;
        MinRPM=MinRPM_;
        swapLeftRight=false;
        invertLeft=false;
        invertRight=false;
        swapEncoders=false;

    }

    void set(double PulsesPerWheelRevolution_,double MtoW_,double WtoM_,double b_,double R_,int limitMaxRPM_,int limitMinRPM_,int MaxRPM_,int MinRPM_){

        PulsesPerWheelRevolution=PulsesPerWheelRevolution_ ;  //500*4*10; //500*4 por causa da quadratura
        MtoW=MtoW_;
        WtoM=WtoM_;
        b=b_;  // dizem ser 60cm, mas medi 59,5cm
        R=R_;
        limitMaxRPM=limitMaxRPM_; //para ja limitar a 50%+-  ??
        limitMinRPM=limitMinRPM_;
        MaxRPM=MaxRPM_;
        MinRPM=MinRPM_;


    }

    Robot(){

        // this is for interbot!!!
        PulsesPerWheelRevolution=980;  //500*4*10; //500*4 por causa da quadratura
        // MtoW=49.0/1.0;
        // WtoM=1.0/49.0;

        MtoW=1.0;
        WtoM=1.0;

        b=0.48;  // dizem ser 60cm, mas medi 59,5cm
        R=0.10;
        limitMaxRPM=500; //para ja limitar a 50%+-  ??
        limitMinRPM=-500;
        MaxRPM=100;
        MinRPM=-100;
        swapLeftRight=false;
        invertLeft=false;
        invertRight=false;
        swapEncoders=false;

    }


    double getVx(double h){return Vx/h;}
    double getVy(double h){return Vy/h;}
    double getW(double h){return W/h;}

    void update(int LeftData,int RightData){




        double Left=((double)LeftData);
        double Right=((double)RightData);



        double displacement =((Right/PulsesPerWheelRevolution)*2.0*pi()*R + (Left/PulsesPerWheelRevolution)*2.0*pi()*R)/ 2.0;
        double angulardisplacement = ((Right/PulsesPerWheelRevolution)*2.0*pi()*R - (Left/PulsesPerWheelRevolution)*2.0*pi()*R)/(b);
        Vx=displacement;
        Vy=0;
        W=angulardisplacement;




        Odometry.X= Odometry.X+displacement*cos(Odometry.Teta);
        Odometry.Y= Odometry.Y+displacement*sin(Odometry.Teta);
        Odometry.Teta=Odometry.Teta+angulardisplacement;
        Odometry.normalizeAngle();




    }

    std::vector<double> parseVelocity(std::string data){

        std::vector<double> output;
        std::string result;

        // Primeiro Remover
        result=data.substr(data.find('=')+1).c_str();
        size_t npos=0;
        while(npos!=std::string::npos){

            npos=result.find(';');

            output.push_back(atof(result.substr(0,npos).c_str()));
            result=result.substr(npos+1).c_str();
        }

        while(output.size()<2){
            output.push_back(1000);

        }

        return output;
    }


    std::vector<int> parseCommand(std::string data){

        std::vector<int> output;
        std::string result;

        // Primeiro Remover
        result=data.substr(data.find('=')+1).c_str();
        size_t npos=0;
        while(npos!=std::string::npos){

            npos=result.find(';');

            output.push_back(atoi(result.substr(0,npos).c_str()));
            result=result.substr(npos+1).c_str();
        }

        return output;

    }




    double getB() const
    {
        return b;
    }

    void setB(double value)
    {
        b = value;
    }

    double getR() const
    {
        return R;
    }

    void setR(double value)
    {
        R = value;
    }

    double getMtoW() const
    {
        return MtoW;
    }

    void setMtoW(double value)
    {
        MtoW = value;
    }

    double getWtoM() const
    {
        return WtoM;
    }

    void setWtoM(double value)
    {
        WtoM = value;
    }

    int getLimitMaxRPM() const
    {
        return limitMaxRPM;
    }

    void setLimitMaxRPM(int value)
    {
        limitMaxRPM = value;
    }

    int getLimitMinRPM() const
    {
        return limitMinRPM;
    }

    void setLimitMinRPM(int value)
    {
        limitMinRPM = value;
    }

    int getMaxRPM() const
    {
        return MaxRPM;
    }

    void setMaxRPM(int value)
    {
        MaxRPM = value;
    }

    int getMinRPM() const
    {
        return MinRPM;
    }

    void setMinRPM(int value)
    {
        MinRPM = value;
    }

    double getPulsesPerWheelRevolution() const
    {
        return PulsesPerWheelRevolution;
    }

    void setPulsesPerWheelRevolution(double value)
    {
        PulsesPerWheelRevolution = value;
    }






    bool getSwapLeftRight() const
    {
        return swapLeftRight;
    }

    void setSwapLeftRight(bool value)
    {
        swapLeftRight = value;
    }



    bool getInvertLeft() const
    {
        return invertLeft;
    }

    void setInvertLeft(bool value)
    {
        invertLeft = value;
    }

    bool getInvertRight() const
    {
        return invertRight;
    }

    void setInvertRight(bool value)
    {
        invertRight = value;
    }

    void setSwapEncoders(bool value){

        swapEncoders = value;
    }

    bool getSwapEncoders(){

        return  swapEncoders;
    }



};

#endif // ROBOT_H




