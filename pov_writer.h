
#ifndef POV_WRITER_H_
#define POV_WRITER_H_


#define LAT_WEIGHT 0.1138
#define LON_WEIGHT 0.0878

#include "primitives.h"

class PovWriter {
	private:
	ofstream fs;
	Rect view_rect;			 //visible rectangle

	public:
	PovWriter(const char *filename, const Rect &view_rect, bool fix_size_to_square);
	~PovWriter();
	bool isOpened() const {
		return this->fs;
	}
	void writeComment(const char *comment);
	void writeTriangle(uint64_t id, const Triangle *triangle, double height, const char *style);
	void writePolygon(uint64_t id, const vector<Triangle> *triangles, double height, const char *style);
	void writePolygon(MultiPolygon *polygon, double height, const char *style);
	void writePolygon(const Polygon3D *polygon, const char *style);
	void writeBox(double x, double y, double width, double height, double length, double angle, const char *style);
	void writeCylinder(double x, double y, double radius, double height, const char *style);
	void writeSprite(double x, double y, const char *sprite_style, size_t sprite_style_number, double scale);

	double convertLatToCoord(double lat) const {
		return (lat - (this->view_rect.minlat+this->view_rect.maxlat)/2) / LAT_WEIGHT * 100;
	}
	double convertLonToCoord(double lon) const {
		return (lon - (this->view_rect.minlon+this->view_rect.maxlon)/2) / LON_WEIGHT * 100 + 100;
	}
	double metres2unit(double metres) {
		return metres / 60;
	}
};


#endif /* POV_WRITER_H_ */
