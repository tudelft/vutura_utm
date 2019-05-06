#include <cmath>
#include "avoidance_geo_tools.h"

void update_position_params(position_params& pos, double latd, double lond, double alt)
{
    // copy inputs of function to member variables
    pos.latd = latd;
    pos.lond = lond;
    pos.alt = alt;

    // calculate other paramaters of struct
    pos.lat = latd / 180. * M_PI;
    pos.lon = lond / 180. * M_PI;
    pos.coslat = cos(pos.lat);
    pos.sinlat = sin(pos.lat);

    // wgs84 r conversion
    double a = 6378137.0;
    double b = 6356752.314245;
    double an = a * a * pos.coslat;
    double bn = b * b * pos.sinlat;
    double ad = a * pos.coslat;
    double bd = b * pos.sinlat;

    double anan = an * an;
    double bnbn = bn * bn;
    double adad = ad * ad;
    double bdbd = bd * bd;

    pos.r = sqrt((anan + bnbn) / (adad + bdbd));
}
