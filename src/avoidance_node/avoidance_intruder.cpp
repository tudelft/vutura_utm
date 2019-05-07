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
            _headingd(hdg),
            _heading(hdg / 180. * M_PI),
            _groundspeed(gs),
            _vn(gs * cos(_heading)),
            _ve(gs * sin(_heading)),
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

void Avoidance_intruder::updateRelVar(double lat_own, double lon_own, double alt_own, double vn_own, double ve_own, double r)
{
        double diff_lat = _lat - lat_own;
        double diff_lon = _lon - lon_own;
        double coslat = cos(lat_own);

        _rel_var.pn = -r * diff_lat;
        _rel_var.pe = -r * diff_lon * coslat;
        _rel_var.pd = -alt_own - _alt;
        _rel_var.vn = vn_own - _vn;
        _rel_var.ve = ve_own - _ve;
        _rel_var.v2 = pow(_rel_var.vn, 2) + pow(_rel_var.ve, 2);
        _rel_var.v = sqrt(_rel_var.v2);
        _rel_var.dist2 = pow(_rel_var.pn, 2) + pow(_rel_var.pe, 2);
        _rel_var.dist = sqrt(_rel_var.dist2);
        _rel_var.bearing = atan2(_rel_var.pe, _rel_var.pn);

//        std::cout << "Traffic \t ID: " << _aircaft_id << " \t (n, e): (" << _rel_var.pn << ", " << _rel_var.pe << ") \t dist: " << _rel_var.dist << " \t bearing: " << _rel_var.bearing << std::endl;
}

void Avoidance_intruder::setConflictPar(bool inconf, double t_cpa, double d_cpa, double d_in, double d_los, double t_los)
{
        _conflict.inconf = inconf;
        _conflict.t_cpa = t_cpa;
        _conflict.d_cpa = d_cpa;
        _conflict.d_in = d_in;
        _conflict.d_los = d_los;
        _conflict.t_los = t_los;
}

double Avoidance_intruder::getVn()
{
        return _vn;
}

double Avoidance_intruder::getVe()
{
        return _ve;
}

double Avoidance_intruder::getReceivedTime()
{
        return _received_time;
}

double Avoidance_intruder::getRecordedTime()
{
        return _recorded_time;
}

double Avoidance_intruder::getPnRel()
{
        return _rel_var.pn;
}

double Avoidance_intruder::getPeRel()
{
        return _rel_var.pe;
}

double Avoidance_intruder::getVnRel()
{
        return _rel_var.vn;
}

double Avoidance_intruder::getVeRel()
{
        return _rel_var.ve;
}

double Avoidance_intruder::getV2Rel()
{
        return _rel_var.v2;
}

double Avoidance_intruder::getVRel()
{
        return _rel_var.v;
}

double Avoidance_intruder::getD2Rel()
{
        return _rel_var.dist2;
}

double Avoidance_intruder::getDRel()
{
        return _rel_var.dist;
}

bool Avoidance_intruder::getInConf()
{
        return _conflict.inconf;
}

void Avoidance_intruder::setData(double latd, double lond,
                                 double alt, double hdgd,
                                 double gs, double recorded_time)
{
        _latd = latd;
        _lat = latd / 180. * M_PI;
        _lond = lond;
        _lon = lond / 180. * M_PI;
        _alt = alt;
        _headingd = hdgd;
        _heading = hdgd / 180. * M_PI;
        _groundspeed = gs;
        _vn = gs * cos(_heading);
        _ve = gs * sin(_heading);
        _recorded_time = recorded_time;
        _received_time = getTimeStamp();
}
