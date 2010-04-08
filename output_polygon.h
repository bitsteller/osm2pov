
#ifndef OUTPUT_POLYGON_H_
#define OUTPUT_POLYGON_H_

class Polygon3D {
	private:
	string coords_output;
	size_t points_count;
	bool is_valid;

	bool addPart(uint64_t area_id, vector<double> &coords);

	public:
	Polygon3D(uint64_t area_id, vector<double> &coords);
	bool isValidPolygon() const { return this->is_valid; }
	void addHole(uint64_t area_id, vector<double> &coords);
	size_t getPointsCount() const { assert(this->is_valid); return this->points_count; }
	string getCoordsOutput() const { assert(this->is_valid); return this->coords_output; }
};

class Way;

struct XY {
	XY() { }
	XY(double x, double y) : x(x), y(y) { }
	double x, y;
};

class Triangle {
	public:
	const XY *points[3];

	public:
	Triangle(const XY *tr1, const XY *tr2, const XY *tr3) {
		this->points[0] = tr1;
		this->points[1] = tr2;
		this->points[2] = tr3;
	}
	double getX(size_t point_pos) const { return this->points[point_pos]->x; }
	double getY(size_t point_pos) const { return this->points[point_pos]->y; }
	const XY *getXY(size_t point_pos) const { return this->points[point_pos]; }
};

struct PointFieldItem {
	XY *xy;
	size_t item_type;
};

class MultiPolygon {
	private:
	bool is_valid;
	vector<const XY*> outer_points;
	const Way *main_way;
	list<const vector<const XY*>*> holes;
	vector<const Triangle*> triangles;

	bool addPolygon(const Way *way, vector<const XY*> *output);
	void convertToTriangles(vector<const Triangle*> *triangles) const;
	static double computeArea(const vector<const XY*> *polygon);

	public:
	MultiPolygon(const Way *main_way);
	~MultiPolygon();
	bool isValid() const { return this->is_valid; }
	void addHole(const Way *hole);
	const vector<const XY*> *getOuterPoints() const { return &this->outer_points; }
	const list<const vector<const XY*>*> *getHoles() const { return &this->holes; }
	const char *getAttribute(const char *key) const;
	bool hasAttribute(const char *key, const char *value) const;
	uint64_t getId() const;
	size_t getPointsCount() const;
	bool isPointInsidePolygon(const vector<const Triangle*> *triangles, const XY *xy) const;
	const vector<const Triangle*> *getPolygonTriangles();
	void computeRegularInsidePoints(vector<PointFieldItem*> *objects, class PointField *point_field, size_t tree_style_min, size_t tree_style_max);
};

#endif /* OUTPUT_POLYGON_H_ */
