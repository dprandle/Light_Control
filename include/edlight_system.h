#pragma once

#include <edsystem.h>

class edlight_system : public edsystem
{
  public:
    edlight_system();

    ~edlight_system();

    void init();

    void release();

    void update();

	std::string typestr();
	
	static std::string TypeString();
};