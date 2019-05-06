#include <cmath>
#include <iostream>
#include "sys/time.h"

#include "avoidance_intruder.hpp"

Avoidance_intruder::Avoidance_intruder(std::string ac_id, double latd,
                                       double lond, double alt, double hdg,
                                       double gs, double recorded_time) :
            _aircaft_id(ac_id),
            _latd(latd),
            _lat(latd / 180. * M_PI),
            _lond(lond),
            _lon(lond / 180. * M_PI),
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

void Avoidance_intruder::updateRelPos(double lat_own, double lon_own, double alt_own, double r)
{
        double diff_lat = _lat - lat_own;
        double diff_lon = _lon - lon_own;
        double coslat = cos(_lat);

        _rel_pos.n = r * diff_lat;
        _rel_pos.e = r * diff_lon * coslat;
        _rel_pos.d = alt_own - _alt;
        _rel_pos.dist = sqrt(pow(_rel_pos.n, 2) + pow(_rel_pos.e, 2));
        _rel_pos.bearing = atan2(_rel_pos.e, _rel_pos.n);

        std::cout << "Traffic \t ID: " << _aircaft_id << " \t (n, e): (" << _rel_pos.n << ", " << _rel_pos.e << ") \t dist: " << _rel_pos.dist << " \t bearing: " << _rel_pos.bearing << std::endl;
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
        _lat = latd / 180. * M_PI;
        _lond = lond;
        _lon = lond / 180. * M_PI;
        _alt = alt;
        _heading = hdg;
        _groundspeed = gs;
        _recorded_time = recorded_time;
        _received_time = getTimeStamp();
}
