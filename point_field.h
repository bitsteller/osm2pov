
#ifndef POINT_FIELD_H_
#define POINT_FIELD_H_

class PointField {
	private:
	map<double,map<double,double> > field;
	double max_used_distance;

	public:
	PointField() : max_used_distance(0) { }
	void addPoint(double x, double y, double distance);
	void addPointsInDistance(double x_from_excluding, double y_from_excluding, double to_x, double to_y, double distance);
	bool isPointNearOther(double x, double y) const;
};

#endif /* POINT_FIELD_H_ */
