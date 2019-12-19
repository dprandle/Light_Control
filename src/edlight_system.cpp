#include <edlight_system.h>
#include <edlogger.h>

edlight_system::edlight_system()
{}

edlight_system::~edlight_system()
{}

void edlight_system::init()
{}

void edlight_system::release()
{}

void edlight_system::update()
{
}

std::string edlight_system::typestr()
{
    return TypeString();
}

std::string edlight_system::TypeString()
{
    return "edlight_system";
}
