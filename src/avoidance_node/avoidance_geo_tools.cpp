#include <cmath>
#include <iostream>
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

geofence_sector calc_geofence_sector_from_n_e(std::array<n_e_coordinate,2>& n_e)
{
	geofence_sector geofence_s;
	geofence_s.north_east		= n_e;
	geofence_s.a			= geofence_s.north_east[0];

	n_e_coordinate diff;
	diff.north			= n_e[1].north - n_e[0].north;
	diff.east			= n_e[1].east  - n_e[0].east;
	double len_diff			= sqrt(pow(diff.north,2) + pow(diff.east, 2));

	geofence_s.n.north		= diff.north / len_diff;
	geofence_s.n.east		= diff.east  / len_diff;

	double dot_a_n			= geofence_s.a.north * geofence_s.n.north + geofence_s.a.east * geofence_s.n.east;
	geofence_s.distv.north		= geofence_s.a.north - dot_a_n * geofence_s.n.north;
	geofence_s.distv.east		= geofence_s.a.east  - dot_a_n * geofence_s.n.east;

	geofence_s.dist			= sqrt(pow(geofence_s.distv.north,2) + pow(geofence_s.distv.east,2));

	geofence_s.rotation		= atan2(geofence_s.distv.north, geofence_s.distv.east) - 0.5 * M_PI;
	return geofence_s;
}

double calc_distance_from_reference_and_target_latdlond(position_params& reference_pos, latdlond& target_latdlond)
{
	n_e_coordinate target_ne = calc_northeast_from_reference(reference_pos, target_latdlond);
	double distance = sqrt(pow(target_ne.east,2) + pow(target_ne.north,2));
	return distance;
}

signed long long Scale_to_clipper(double coord)
{
	signed long long clipper_coord = static_cast<signed long long>(coord * pow(2., 31));
	return clipper_coord;
}

double Scale_from_clipper(double coord)
{
	double double_coord = static_cast<double>(coord * pow(2., -31));
	return double_coord;
}

bool latdlond_inside_geofence(position_params &reference_pos, std::vector<std::vector<double>>& geofence_ll, latdlond& target_latdlond)
{
	ClipperLib::Clipper c;
	ClipperLib::Path geofence;
	for (size_t i = 0; i < geofence_ll.size(); ++i)
	{
		latdlond geofence_point_latdlond;
		geofence_point_latdlond.latd = geofence_ll.at(i).at(1);
		geofence_point_latdlond.lond = geofence_ll.at(i).at(0);
		n_e_coordinate geofence_point_ne = calc_northeast_from_reference(reference_pos, geofence_point_latdlond);

		geofence << ClipperLib::IntPoint(Scale_to_clipper(geofence_point_ne.east), Scale_to_clipper(geofence_point_ne.north));
	}
	n_e_coordinate target_point_ne = calc_northeast_from_reference(reference_pos, target_latdlond);
	ClipperLib::IntPoint target_point_clipper;
	target_point_clipper.X = Scale_to_clipper(target_point_ne.east);
	target_point_clipper.Y = Scale_to_clipper(target_point_ne.north);

	return ClipperLib::PointInPolygon(target_point_clipper, geofence);
}
