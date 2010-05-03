
#ifndef POV_WRITER_H_
#define POV_WRITER_H_


#define LAT_WEIGHT 0.1138
#define LON_WEIGHT 0.0878

class PovWriter {
	private:
	ofstream fs;
	double minlat, minlon, maxlat, maxlon;

	public:
	PovWriter(const char *filename, double minlat, double minlon, double maxlat, double maxlon);
	~PovWriter();
	bool isOpened() const {
		return this->fs;
	}
	void writeComment(const char *comment);
	void writeTriangle(const Triangle *triangle, double height, const char *style);
	void writePolygon(MultiPolygon *polygon, double height, const char *style);
	void writePolygon(const Polygon3D *polygon, const char *style);
	void writeBox(double x, double y, double width, double height, double length, double angle, const char *style);
	void writeCylinder(double x, double y, double radius, double height, const char *style);
	void writeSprite(double x, double y, const char *sprite_style, size_t sprite_style_number);

	double convertLatToCoord(double lat) const {
		return (lat - (this->minlat+this->maxlat)/2) / LAT_WEIGHT * 100;
	}
	double convertLonToCoord(double lon) const {
		return (lon - (this->minlon+this->maxlon)/2) / LON_WEIGHT * 100 + 100;
	}
	double metres2unit(double metres) {
		return metres / 60;
	}
};


#endif /* POV_WRITER_H_ */
