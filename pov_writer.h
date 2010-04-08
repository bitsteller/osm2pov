
#ifndef POV_WRITER_H_
#define POV_WRITER_H_

class PovWriter {
	private:
	ofstream fs;
	double minlat, minlon, maxlat, maxlon;

	double convertLatToCoord(double lat) const {
		return (lat - this->maxlat) / (this->maxlat-this->minlat) * 100 + 50;
	}
	double convertLonToCoord(double lon) const {
		return (lon - this->minlon) / (this->maxlon-this->minlon) * 100 + 50;
	}
	double metres2unit(double metres) {
		return metres / 60;
	}

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
};


#endif /* POV_WRITER_H_ */
