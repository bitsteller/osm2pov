
#include "global.h"
#include "output_polygon.h"
#include "pov_writer.h"
#include "primitives.h"


PovWriter::PovWriter(const char *filename, double minlat, double minlon, double maxlat, double maxlon) {
	this->minlat = minlat;
	this->minlon = minlon;
	this->maxlat = maxlat;
	this->maxlon = maxlon;

	{			//fix coords to make area square
		const double weighted_lat_diff = (this->maxlat - this->minlat)/LAT_WEIGHT;
		const double weighted_lon_diff = (this->maxlon - this->minlon)/LON_WEIGHT;
		if (weighted_lat_diff > weighted_lon_diff) {
			const double lon_center = (this->minlon + this->maxlon)/2;
			const double new_diff_to_center = (lon_center-this->minlon)*(weighted_lat_diff/weighted_lon_diff);
			this->minlon = lon_center - new_diff_to_center;
			this->maxlon = lon_center + new_diff_to_center;
		}
		else {
			const double lat_center = (this->minlat + this->maxlat)/2;
			const double new_diff_to_center = (lat_center-this->minlat)*(weighted_lon_diff/weighted_lat_diff);
			this->minlat = lat_center - new_diff_to_center;
			this->maxlat = lat_center + new_diff_to_center;
		}
	}

	this->fs.open(filename);
	if (!this->fs) {
		cerr << "Cannot open " << filename << "!" << endl;
		return;
	}

	this->fs.precision(12);

	this->fs << "camera { orthographic location <0,0,-230> direction <0,0,13> up <0," << ((this->maxlat-this->minlat)*69.71/LAT_WEIGHT) << ",0> right <" << ((this->maxlon-this->minlon)*100/LON_WEIGHT) << ",0,0> look_at <0,0,0> translate <100,0,0> rotate <45,0,0> }" << endl;
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

void PovWriter::writeTriangle(const Triangle *triangle, double height, const char *style) {
	this->fs << "triangle { ";

	for (size_t i = 0; i < 3; i++) {
		this->fs << (i == 0 ? "<" : ",<") << this->convertLonToCoord(triangle->getX(i)) << "," << this->metres2unit(height) << "," << this->convertLatToCoord(triangle->getY(i)) << ">";
	}

	this->fs << " texture { " << style << " } ";
	this->fs << "}" << endl;
}

void PovWriter::writePolygon(MultiPolygon *polygon, double height, const char *style) {
	assert(polygon->isValid());

			//now, all polygons are decomponed into triangles. It's most causes faster rendering
	if (polygon->getPointsCount() > 1) {
		const vector<const Triangle*> *triangles = polygon->getPolygonTriangles();
		for (vector<const Triangle*>::const_iterator it = triangles->begin(); it != triangles->end(); it++) {
			this->writeTriangle(*it, height, style);
		}
		return;
	}

	// !! this isn't never used (because polygon count is always > 1)
	//next code is only when you (for any reason) want write polygon as polygon primitive, not set of triangles

	this->fs << "polygon { ";
	this->fs << polygon->getPointsCount() << " ";

	{
		const list<vector<const XY*> > *outer_parts = polygon->getOuterParts();
		bool first = true;
		for (list<vector<const XY*> >::const_iterator it = outer_parts->begin(); it != outer_parts->end(); it++) {
			for (vector<const XY*>::const_iterator it2 = it->begin(); it2 != it->end(); it2++) {
				this->fs << (first ? "<" : ",<") << this->convertLonToCoord((*it2)->x) << "," << this->metres2unit(height) << "," << this->convertLatToCoord((*it2)->y) << ">";
				first = false;
			}
		}
	}
	{
		const list<vector<const XY*> > *holes = polygon->getHoles();
		for (list<vector<const XY*> >::const_iterator it = holes->begin(); it != holes->end(); it++) {
			for (vector<const XY*>::const_iterator it2 = it->begin(); it2 != it->end(); it2++) {
				this->fs << ",<" << this->convertLonToCoord((*it2)->x) << "," << this->metres2unit(height) << "," << this->convertLatToCoord((*it2)->y) << ">";
			}
		}
	}

	this->fs << " texture { " << style << " } ";
	this->fs << "}" << endl;
}

void PovWriter::writePolygon(const Polygon3D *polygon, const char *style) {
	if (!polygon->isValidPolygon()) return;

	this->fs << "polygon { ";
	this->fs << polygon->getPointsCount() << " ";
	this->fs << polygon->getCoordsOutput();
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

void PovWriter::writeSprite(double x, double y, const char *sprite_style, size_t sprite_style_number) {
	this->fs << "plane { ";
	this->fs << "z, 0 hollow on clipped_by { box { <0,0,-1>, <1,1,1> } } ";
	this->fs << "texture { " << sprite_style << sprite_style_number << " } ";
	this->fs << "translate <-0.5,0,0> ";
	this->fs << "scale 0.3 ";		//TODO modificable scale
	this->fs << "translate <" << (this->convertLonToCoord(x)) << ", 0, " << this->convertLatToCoord(y) << ">";
	this->fs << "}" << endl;
}

