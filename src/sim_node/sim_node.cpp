#include <unistd.h>
#include <cmath>

#include "vutura_common/config.hpp"
#include "vutura_common/helper.hpp"
//#include "vutura_common/event_loop.hpp"
//#include "vutura_common/timer.hpp"

#include "vutura_common.pb.h"

#include "sim_node.hpp"

int get_relative_coordinates(double lat_0, double lon_0, double lat_i, double lon_i, double *x, double *y)
{

	const double lat_0_rad = lat_0 * M_PI / 180.0;
	const double lon_0_rad = lon_0 * M_PI / 180.0;
	const double sin_lat_0 = std::sin(lat_0_rad);
	const double cos_lat_0 = std::cos(lat_0_rad);

	const double lat_i_rad = lat_i * M_PI / 180.0;
	const double lon_i_rad = lon_i * M_PI / 180.0;
	const double sin_lat_i = std::sin(lat_i_rad);
	const double cos_lat_i = std::cos(lat_i_rad);

	const double cos_d_lon = std::cos(lon_i_rad - lon_0_rad);

	double arg = sin_lat_0 * sin_lat_i + cos_lat_0 * cos_lat_i * cos_d_lon;
	if (arg > 1.0) {
		arg = 1.0;
	}
	if (arg < -1.0) {
		arg = -1.0;
	}
	const double c = std::acos(arg);

	double k = 1.0;

	if (fabs(c) > 0) {
		k = (c / std::sin(c));
	}

	*x = k * (cos_lat_0 * sin_lat_i - sin_lat_0 * cos_lat_i * cos_d_lon) * 6371000;
	*y = k * cos_lat_i * std::sin(lon_i_rad - lon_0_rad) * 6371000;

	return 0;
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

SimNode::SimNode(int instance) :
	_periodic_timer(this),
	_gps_pub(socket_name(SOCK_PUBSUB_GPS_POSITION, instance)),
	_angle(0.0f),
	_instance(instance)
{

}

SimNode::~SimNode()
{

}

void SimNode::init()
{
	update_position_params(_pos, 52.1693774, 4.4121613, 45.0);

	_periodic_timer.set_timeout_callback(std::bind(&SimNode::handle_periodic_timer, this));
	_periodic_timer.start_periodic(500);
}

int SimNode::handle_periodic_timer()
{

	_angle += 1.0 *  M_PI / 180;
	if (_angle >= 360 * M_PI / 180) {
		_angle = 0.0;
	}


	// Fly Circle
	float lat_0 = 52.17006269856014; // valkenburg
	float lon_0 = 4.4194865226745605;

	float lon = lon_0 + 0.003 * std::cos(_angle);
	float lat = lat_0 + 0.002 * std::sin(_angle);

	// Fly straight line
	latdlond start_position = {52.1693774, 4.4121613};
	latdlond end_position = {52.1638250, 4.4160479};
	// calculate delta x,y
	double x, y;
	get_relative_coordinates(start_position.latd, start_position.lond, end_position.latd, end_position.lond, &x, &y);
	// normalize
	double dx, dy, length;
	length = sqrt(x * x + y * y);
	dx = x / length;
	dy = y / length;

	float dt = 0.5;
	float groundspeed = 7.0;
	std::cout << std::to_string(dx) << " " << std::to_string(dy) << std::endl;

	n_e_coordinate target = {groundspeed * dx * dt, groundspeed * dy * dt};
	latdlond new_position = calc_latdlond_from_reference(_pos, target);
	update_position_params(_pos, new_position.latd, new_position.lond, 45.0);

	GPSMessage gps_message;
	uint16_t len;
	gps_message.set_timestamp(0);
	gps_message.set_lat(new_position.latd*1e7);
	gps_message.set_lon(new_position.lond*1e7);

	// temp field
	gps_message.set_lat((51.991282409719666 - 0.00026426547214697393 * _instance) * 1e7);
	gps_message.set_lon((4.377976655960083 + 0.00021457672119140625 * _instance) * 1e7);

	gps_message.set_alt_msl(45000);
	gps_message.set_alt_agl(45000);

	std::string message_string = gps_message.SerializeAsString();

	_gps_pub.publish(message_string);

	std::cout << "lat: " << lat << "\tlon: " << lon << std::endl;

	return 0;
}
