#pragma once
#include <string>
class entity{
public:
    virtual void update(float step){}
    virtual std::string get_name(){ return "";}
};
