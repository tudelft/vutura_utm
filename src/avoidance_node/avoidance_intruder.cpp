#include "sys/time.h"

#include "avoidance_intruder.hpp"

Avoidance_intruder::Avoidance_intruder(std::string ac_id, double latd,
                                       double lond, double alt, double hdg,
                                       double gs, double recorded_time) :
    _aircaft_id(ac_id),
    _latd(latd),
    _lond(lond),
    _alt(alt),
    _heading(hdg),
    _groundspeed(gs),
    _recorded_time(recorded_time),
    _received_time(getTimeStamp())
{

};

double Avoidance_intruder::getTimeStamp()
{
        struct timeval tp;
        gettimeofday(&tp, NULL);
    return tp.tv_sec + tp.tv_usec * 1e-6;
}

std::string Avoidance_intruder::getAircraftId()
{
    return _aircaft_id;
}

double Avoidance_intruder::getReceivedTime()
{
    return _received_time;
}

void Avoidance_intruder::setData(double latd, double lond,
                                 double alt, double hdg,
                                 double gs, double recorded_time)
{
    _latd = latd;
    _lond = lond;
    _alt = alt;
    _heading = hdg;
    _groundspeed = gs;
    _recorded_time = recorded_time;
    _received_time = getTimeStamp();
}
