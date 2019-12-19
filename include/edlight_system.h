#pragma once

#include <edsystem.h>

class edtimer;

const double PROCESS_TIMEOUT = 0.083; // 83 microseconds for 100 points every half cycle (60 Hz)

class edlight_system : public edsystem
{
  public:
    edlight_system();

    ~edlight_system();

    void init();

    void release();

    void update();
    
    void process();
    
    std::string typestr();
    
    static std::string TypeString();
  private:
    edtimer * process_timer;
};
