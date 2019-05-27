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

latdlond calc_latdlond_from_reference(position_params& reference_pos, n_e_coordinate& target_ne)
{
	double ref_cos_lat	= reference_pos.coslat;
	double ref_r		= reference_pos.r;

	double diff_lat		= asin(target_ne.north/ref_r);
	double diff_lon		= asin(target_ne.east/(ref_r * ref_cos_lat));


	double diff_latd	= diff_lat / M_PI * 180.;
	double diff_lond	= diff_lon / M_PI * 180.;

	latdlond target_latdlond;
	target_latdlond.latd	= reference_pos.latd + diff_latd;
	target_latdlond.lond	= reference_pos.lond + diff_lond;
	return target_latdlond;
}

n_e_coordinate calc_northeast_from_reference(position_params& reference_pos, latdlond& target_latdlond)
{
	double ref_lat		= reference_pos.lat;
	double ref_coslat	= reference_pos.coslat;
	double ref_lon		= reference_pos.lon;
	double ref_r		= reference_pos.r;

	double target_lat	= target_latdlond.latd / 180. * M_PI;
	double target_lon	= target_latdlond.lond / 180. * M_PI;

	double diff_lat		= target_lat - ref_lat;
	double diff_lon		= target_lon - ref_lon;

	n_e_coordinate target_ne;
	target_ne.north		= diff_lat * ref_r;
	target_ne.east		= diff_lon * ref_r * ref_coslat;

	return target_ne;
}
