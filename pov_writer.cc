
#include "global.h"
#include "output_polygon.h"
#include "pov_writer.h"
#include "primitives.h"


double metres2unit(double metres) {
	return metres / 60;
}

PovWriter::PovWriter(const char *filename, const Rect &view_rect, bool fix_size_to_square) {
	this->view_rect = view_rect;

	if (fix_size_to_square) {			//fix coords to make area square
		const double weighted_lat_diff = (this->view_rect.maxlat - this->view_rect.minlat)/LAT_WEIGHT;
		const double weighted_lon_diff = (this->view_rect.maxlon - this->view_rect.minlon)/LON_WEIGHT;
		if (weighted_lat_diff > weighted_lon_diff) {
			const double lon_center = (this->view_rect.minlon + this->view_rect.maxlon)/2;
			const double new_diff_to_center = (lon_center-this->view_rect.minlon)*(weighted_lat_diff/weighted_lon_diff);
			this->view_rect.minlon = lon_center - new_diff_to_center;
			this->view_rect.maxlon = lon_center + new_diff_to_center;
		}
		else {
			const double lat_center = (this->view_rect.minlat + this->view_rect.maxlat)/2;
			const double new_diff_to_center = (lat_center-this->view_rect.minlat)*(weighted_lon_diff/weighted_lat_diff);
			this->view_rect.minlat = lat_center - new_diff_to_center;
			this->view_rect.maxlat = lat_center + new_diff_to_center;
		}
	}

	this->fs.open(filename);
	if (!this->fs) {
		cerr << "Cannot open " << filename << "!" << endl;
		return;
	}

	this->fs.precision(12);

	this->fs << "camera { orthographic location <0,0,-230> direction <0,0,13>";
	this->fs << " up <0," << ((this->view_rect.maxlat-this->view_rect.minlat)*76.54/LAT_WEIGHT) << ",0>";
	this->fs << " right <" << ((this->view_rect.maxlon-this->view_rect.minlon)*100/LON_WEIGHT) << ",0,0>";
	this->fs << " look_at <0,0,0> translate <100,0,0> rotate <22.5,0,0> }" << endl;
	this->fs << "#include \"osm2pov-styles.inc\"" << endl;
}

PovWriter::~PovWriter() {
	if (this->fs) {
		this->writeComment("End of file");
		this->fs.close();
	}
}

void PovWriter::writeComment(const char *comment) {
	this->fs << " // " << comment << endl;
}

void PovWriter::writeTriangle(uint64_t id, const Triangle &triangle, double height, const char *style) {
	double x[3], y[3];
	for (size_t i = 0; i < 3; i++) {
		x[i] = this->convertLonToCoord(triangle.getX(i));
		y[i] = this->convertLatToCoord(triangle.getY(i));
	}

	//check if 2 points aren't the same (it's when some points are the same but no side-by-side (this is checked previously))
	//in that cases Pov-Ray takes it as infinite objects and warn

	for (size_t i = 0; i < 3; i++) {
		if (x[i] <= x[(i+1)%3]+COMP_PRECISION && x[i] >= x[(i+1)%3]-COMP_PRECISION
		 && y[i] <= y[(i+1)%3]+COMP_PRECISION && y[i] >= y[(i+1)%3]-COMP_PRECISION) {
			cerr << "Skipping triangle in area with id " << id << "." << endl;
			return;
		}
	}

	this->fs << "triangle { ";

	for (size_t i = 0; i < 3; i++) {
		this->fs << (i == 0 ? "<" : ",<") << x[i] << "," << metres2unit(height) << "," << y[i] << ">";
	}

	this->fs << " texture { " << style << " } ";
	this->fs << "}" << endl;
}

//When all polygons are decomposed into triangles. It is in most cases faster rendering
void PovWriter::writePolygon(uint64_t id, const vector<Triangle> &triangles, double height, const char *style) {
	for (vector<Triangle>::const_iterator it = triangles.begin(); it != triangles.end(); it++) {
		this->writeTriangle(id, *it, height, style);
	}
}

// !! this is never used (because polygon count is always > 1)
//next code is only when you (for any reason) want write polygon as polygon primitive, not set of triangles
void PovWriter::writePolygon(const MultiPolygon &polygon, double height, const char *style) {
	assert(polygon.isValid());

	this->fs << "polygon { ";
	this->fs << polygon.getPointsCount() << " ";

	{
		const list<vector<XY> > &outer_parts = polygon.getOuterParts();
		bool first = true;
		for (list<vector<XY> >::const_iterator it = outer_parts.begin(); it != outer_parts.end(); it++) {
			for (vector<XY>::const_iterator it2 = it->begin(); it2 != it->end(); it2++) {
				this->fs << (first ? "<" : ",<") << this->convertLonToCoord(it2->x) << "," << metres2unit(height) << "," << this->convertLatToCoord(it2->y) << ">";
				first = false;
			}
		}
	}
	{
		const list<vector<XY> > &holes = polygon.getHoles();
		for (list<vector<XY> >::const_iterator it = holes.begin(); it != holes.end(); it++) {
			for (vector<XY>::const_iterator it2 = it->begin(); it2 != it->end(); it2++) {
				this->fs << ",<" << this->convertLonToCoord(it2->x) << "," << metres2unit(height) << "," << this->convertLatToCoord(it2->y) << ">";
			}
		}
	}

	this->fs << " texture { " << style << " } ";
	this->fs << "}" << endl;
}

void PovWriter::writePolygon(const Polygon3D &polygon, const char *style) {
	if (!polygon.isValidPolygon()) return;

	this->fs << "polygon { ";
	this->fs << polygon.getPointsCount() << " ";
	this->fs << polygon.getCoordsOutput();
	this->fs << " texture { " << style << " } ";
	this->fs << "}" << endl;
}

void PovWriter::writeBox(double x, double y, double width, double height, double length, double angle, const char *style) {
	this->fs << "box { ";
	this->fs << "<0,0," << -(width/2) << ">, ";
	this->fs << "<" << length << "," << height << "," << (width/2) << "> ";
	this->fs << "texture { " << style << " } ";
	this->fs << "rotate <0," << angle << ",0> ";
	this->fs << "translate <" << x << ",0," << y << "> ";
	this->fs << "}" << endl;
}

void PovWriter::writeCylinder(double x, double y, double radius, double height, const char *style) {
	this->fs << "cylinder { ";
	this->fs << "<0," << height << ",0>, ";
	this->fs << "<0,-1,0>, ";		//-1 because when it has too small height, povray don't render it
	this->fs << radius << " ";
	this->fs << "texture { " << style << " } ";
	this->fs << "translate <" << x << ",0," << y << "> ";
	this->fs << "}" << endl;
}

void PovWriter::writeSprite(double x, double y, const char *sprite_style, size_t sprite_style_number, double scale) {
	this->fs << "plane { ";
	this->fs << "z, 0 hollow on clipped_by { box { <0,0,-1>, <1,1,1> } } ";
	this->fs << "texture { " << sprite_style << sprite_style_number << " } ";
	this->fs << "translate <-0.5,0,0> ";
	this->fs << "scale " << scale << " ";
	this->fs << "translate <" << (this->convertLonToCoord(x)) << ", 0, " << this->convertLatToCoord(y) << ">";
	this->fs << "}" << endl;
}

