
#include <cmath>

#include "global.h"
#include "point_field.h"

void PointField::addPoint(double x, double y, double distance) {
	distance /= 2000;
	map<double,map<double,double> >::const_iterator x_it = this->field.find(x);
	this->field[x].insert(make_pair(y, distance));	//TODO magic constant, unify units!

	if (distance > this->max_used_distance)
		this->max_used_distance = distance;
}

void PointField::addPointsInDistance(double x_from_excluding, double y_from_excluding, double to_x, double to_y, double distance) {
	distance /= 2000;

	{
		double dist_between = distance / 2;
		double x_delta = to_x - x_from_excluding;
		double y_delta = to_y - y_from_excluding;
		double margin_distance = sqrt(x_delta*x_delta + y_delta*y_delta);
		if (margin_distance > dist_between) {
			//adding some points between marginal points

			size_t add_points = static_cast<size_t>(ceil(margin_distance/dist_between))-1;
			double step = 1/(static_cast<double>(add_points)+1);

			double x_now = x_from_excluding + x_delta*step;
			double y_now = y_from_excluding + y_delta*step;
			for (size_t i = 0; i < add_points; i++) {
				this->addPoint(x_now, y_now, distance*2000);//!
				x_now += x_delta*step;
				y_now += y_delta*step;
			}
		}
	}

	this->addPoint(to_x, to_y, distance*2000);//! ugly hack, TODO something with units
}

bool PointField::isPointNearOther(double x, double y) const {
	for (map<double,map<double,double> >::const_iterator x_it = this->field.lower_bound(x - this->max_used_distance); x_it != this->field.end(); x_it++) {
		double x_delta = x_it->first - x;
		if (x_delta > this->max_used_distance) break;

		for (map<double,double>::const_iterator y_it = x_it->second.lower_bound(y - this->max_used_distance); y_it != x_it->second.end(); y_it++) {
			double y_delta = y_it->first - y;
			if (y_delta > this->max_used_distance) break;

			if (sqrt(x_delta*x_delta + y_delta*y_delta) <= y_it->second) {
				return true;
			}
		}

	}

	return false;
}
