#include <edlight_system.h>
#include <edlogger.h>
#include <edtimer.h>

edlight_system::edlight_system():
    process_timer(new edtimer)
{}

edlight_system::~edlight_system()
{
    delete process_timer;
}

void edlight_system::init()
{
    using namespace std::placeholders;
    std::function<void(edtimer*)> cb_func = std::bind(&edlight_system::process, this, _1);
    process_timer->set_callback(cb_func);
    process_timer->set_callback_mode(edtimer::continous_shot);
    process_timer->set_callback_delay(PROCESS_TIMEOUT);
}

void edlight_system::release()
{}

void edlight_system::update()
{
    process_timer->update();
}

void edlight_system::process()
{
    static int counter = 0;
    static int seconds = 0;
    ++counter;
    if (counter == 12000)
    {
        dlog("Reached second! total seconds elapsed: {}",second);
        ++seconds;
        counter = 0;
    }
}

std::string edlight_system::typestr()
{
    return TypeString();
}

std::string edlight_system::TypeString()
{
    return "edlight_system";
}
