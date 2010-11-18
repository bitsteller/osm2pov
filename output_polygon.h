
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
	XY(const XY *xy) : x(xy->x), y(xy->y) { }
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

void ComputeRegularInsidePoints(const vector<Triangle> *triangles, vector<PointFieldItem*> *output_objects, class PointField *point_field, size_t tree_style_min, size_t tree_style_max);

class MultiPolygon {
	private:
	bool is_valid;
	bool is_done;
	list<vector<const XY*> > outer_parts;
	list<const Way*> outer_ways;
	list<vector<const XY*> > holes;
	const class Relation *relation;  //NULL if isn't in any relation
	const class Rect *interest_rect;

	public:
	MultiPolygon(const Relation *relation, const Rect *interest_rect);
	~MultiPolygon();
	bool isValid() const { assert(this->is_done); return this->is_valid; }
	bool isDone() const { return this->is_valid; }
	void addOuterPart(const Way *outer_part);
	void addHole(const Way *hole);
	bool hasAnyOuterPart() const { return !this->outer_ways.empty(); }
	void setDone();
	const list<vector<const XY*> > *getOuterParts() const { return &this->outer_parts; }
	const list<vector<const XY*> > *getHoles() const { return &this->holes; }
	const char *getAttribute(const char *key) const;
	bool hasAttribute(const char *key, const char *value) const;
	uint64_t getId() const;
	size_t getPointsCount() const;
	void convertToTriangles(vector<Triangle> *triangles) const;
};

#endif /* OUTPUT_POLYGON_H_ */
