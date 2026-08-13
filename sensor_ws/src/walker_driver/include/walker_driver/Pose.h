#ifndef POSE_H
#define POSE_H


#include <iostream>
#include <sstream>
template <typename T>
class Pose{
public:
    T X;
    T Y;
    T Teta;

    Pose(){
        X=0;
        Y=0;
        Teta=0;
    }
    Pose(T x_,T y_,T teta){
        X=x_;
        Y=y_;
        Teta=teta;
    }
    void set(T x_,T y_,T teta){
        X=x_;
        Y=y_;
        Teta=teta;
    }
    void print(){
        std::cout<<"X : "<<X<<"   Y : "<<Y<<"   Teta : "<<Teta<<"\n";
    }
    std::string toString(){
        std::stringstream str;

        str<<X<<" "<<Y<<" "<<Teta;

        return str.str();

    }
    Pose<T>& operator=(const Pose<T> &rhs) {
        if (this == &rhs)
            return *this;


        X=rhs.X;
        Y=rhs.Y;
        Teta=rhs.Teta;
        return *this;
    }
    void normalizeAngle(){

        Teta= atan2(sin(Teta), cos(Teta));

    }

    void reset(){

        X=0;
        Y=0;
        Teta=0;

    }

};



#endif // POSE_H
