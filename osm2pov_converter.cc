
#include "global.h"
#include "point_field.h"
#include "output_polygon.h"
#include "osm2pov_converter.h"
#include "pov_writer.h"
#include "primitives.h"

/*
 * Extracts the height or width from a tag value as meters.
 */
double Osm2PovConverter::readDimension(const char *dimension_text) {
	double dimension = atof(dimension_text);

	//we assume the units to be meters unless we find information suggesting otherwise
	if (strlen(dimension_text) > 3) {
		if (strcmp(dimension_text+strlen(dimension_text)-3," ft") == 0) dimension *= 0.3048; //feets
		else if (strcmp(dimension_text+strlen(dimension_text)-3," yd") == 0) dimension *= 0.9144; //yards
	}

	return dimension;
}

void Osm2PovConverter::drawTowers(const char *key, const char *value, double width, double default_height, const char *style) {
	list<const Node*> nodes;
	this->primitives->getNodesWithAttribute(&nodes, key, value);
	for (list<const Node*>::iterator it = nodes.begin(); it != nodes.end(); it++) {
		{
			stringstream s;
			s << "Node " << (*it)->getId() << " (tag " << key;
			if (value != NULL) s << "=" << value;
			s << ")";
			this->pov_writer->writeComment(s.str().c_str());
		}

		double x = this->pov_writer->convertLonToCoord((*it)->getLon());
		double y = this->pov_writer->convertLatToCoord((*it)->getLat());

		this->point_field.addPoint((*it)->getLon(), (*it)->getLat(), this->pov_writer->metres2unit(width+1.5)*2);

		double height = default_height;
		const char *str = (*it)->getAttribute("height");
		if (str != NULL) height = readDimension(str);

		this->pov_writer->writeCylinder(x, y, this->pov_writer->metres2unit(width)/2, this->pov_writer->metres2unit(height), style);
	}
}

void Osm2PovConverter::drawWay(const vector<const Node*> *nodes, double width, double height, const char *style, bool including_links) {
	bool first = true;
	double x_before, y_before;
	double lon_before, lat_before;
	for (vector<const Node*>::const_iterator it = nodes->begin(); it != nodes->end(); it++) {
		double x = this->pov_writer->convertLonToCoord((*it)->getLon());
		double y = this->pov_writer->convertLatToCoord((*it)->getLat());

		if (first) {
			this->point_field.addPoint((*it)->getLon(), (*it)->getLat(), this->pov_writer->metres2unit(width+1.5)*2);
			first = false;
		}
		else {
			this->point_field.addPointsInDistance(lon_before, lat_before, (*it)->getLon(), (*it)->getLat(), this->pov_writer->metres2unit(width+1.5)*2);
			double x_delta = x-x_before, y_delta = y-y_before;
			double angle = (-(atan2(y_delta,x_delta) * 180 / M_PI)) + 180;
			double length = sqrt(x_delta*x_delta+y_delta*y_delta);

			this->pov_writer->writeBox(x, y, this->pov_writer->metres2unit(width), this->pov_writer->metres2unit(height), length, angle, style);
		}

		if (including_links)
                        this->pov_writer->writeCylinder(x, y, this->pov_writer->metres2unit(width)/2, this->pov_writer->metres2unit(height), style);

		x_before = x; y_before = y;
		lon_before = (*it)->getLon(); lat_before = (*it)->getLat();
	}
}

void Osm2PovConverter::drawWays(const char *key, const char *value, double width, double height, const char *style, bool including_links) {
	list<const Way*> ways;
	this->primitives->getWaysWithAttribute(&ways, key, value);
	for (list<const Way*>::iterator it = ways.begin(); it != ways.end(); it++) {
		if ((*it)->hasAttribute("area", "yes")) {
			stringstream s;
			s << "Area (closed way with area=yes) " << (*it)->getId();
			this->pov_writer->writeComment(s.str().c_str());

			this->drawArea((*it)->getId(), (*it)->getNodes(), height+0.0001, strcmp(style,"highway") == 0 ? "highway_area" : style);
			continue;
		}
		const char *extra_layer_str = (*it)->getAttribute("layer");
		double extra_layer = (extra_layer_str == NULL ? 0 : atof(extra_layer_str)/500);
		if (extra_layer < 0) extra_layer = 0;//!

		{
			stringstream s;
			s << "Way " << (*it)->getId() << " (tag " << key;
			if (value != NULL) s << "=" << value;
			s << ")";
			this->pov_writer->writeComment(s.str().c_str());
		}

		//overide default width if it is defined in the width tag
		const char *width_str = (*it)->getAttribute("width");
		if (width_str != NULL) width = readDimension(width_str);

		this->drawWay((*it)->getNodes(), width, height+extra_layer, style, including_links);
	}
}

void Osm2PovConverter::drawWaysWithBorder(const char *key, const char *value, double width, double height, const char *style, double border_width_percent, const char *border_style) {
	list<const Way*> ways;
	primitives->getWaysWithAttribute(&ways, key, value);
	for (list<const Way*>::iterator it = ways.begin(); it != ways.end(); it++) {
		if ((*it)->hasAttribute("area", "yes")) {
			stringstream s;
			s << "Area (closed way with area=yes) " << (*it)->getId() << " (tag " << key;
			if (value != NULL) s << "=" << value;
			s << ")";
			this->pov_writer->writeComment(s.str().c_str());

			drawArea((*it)->getId(), (*it)->getNodes(), height+0.0001, strcmp(style,"highway") == 0 ? "highway_area" : style);
			continue;
		}
		const char *extra_layer_str = (*it)->getAttribute("layer");
		double extra_layer = (extra_layer_str == NULL ? 0 : atof(extra_layer_str)/500);
		if ((*it)->hasAttribute("tunnel", "yes")) height /= 2;	//!
		if (extra_layer < 0) extra_layer = 0;//!

		{
			stringstream s;
			s << "Way " << (*it)->getId() << " with border (tag " << key;
			if (value != NULL) s << "=" << value;
			s << ")";
			this->pov_writer->writeComment(s.str().c_str());
		}
		drawWay((*it)->getNodes(), width, height-0.0011+extra_layer, border_style, true);
		drawWay((*it)->getNodes(), width-border_width_percent*width/100*2, height+extra_layer, (*it)->hasAttribute("tunnel", "yes") && strcmp(style, "highway") == 0 ? "highway_tunnel" : style, true);
	}
}

void Osm2PovConverter::drawArea(uint64_t area_id, const vector<const Node*> *nodes, double height, const char *style) {
	vector<double> coords;

	for (vector<const Node*>::const_iterator it2 = nodes->begin(); it2 != nodes->end(); it2++) {
		double lat = this->pov_writer->convertLatToCoord((*it2)->getLat());
		double lon = this->pov_writer->convertLonToCoord((*it2)->getLon());

		coords.push_back(lon);
		coords.push_back(this->pov_writer->metres2unit(height));
		coords.push_back(lat);
	}

	Polygon3D polygon(area_id, coords);
	this->pov_writer->writePolygon(&polygon, style);
}

void Osm2PovConverter::drawAreas(const char *key, const char *value, double height, const char *style) {
	list<MultiPolygon*> multipolygons;
	this->primitives->getMultiPolygonsWithAttribute(&multipolygons, key, value);
	for (list<MultiPolygon*>::iterator it = multipolygons.begin(); it != multipolygons.end(); it++) {
		const char *extra_layer_str = (*it)->getAttribute("layer");
		double extra_layer = (extra_layer_str == NULL ? 0 : atof(extra_layer_str)/500);
		if (extra_layer < 0) extra_layer = 0;//!

		{
			stringstream s;
			s << "Area (closed way) " << (*it)->getId() << " (tag " << key;
			if (value != NULL) s << "=" << value;
			s << ")";
			this->pov_writer->writeComment(s.str().c_str());
		}
		this->pov_writer->writePolygon(*it, height+extra_layer, style);
		delete *it;
	}
}

void Osm2PovConverter::drawForests(const char *key, const char *value, double floor_height, const char *floor_style, const char *tree_style_basic, size_t tree_style_coniferous_min, size_t tree_style_coniferous_max, size_t tree_style_overall_max) {
	list<MultiPolygon*> multipolygons;
	this->primitives->getMultiPolygonsWithAttribute(&multipolygons, key, value);
	for (list<MultiPolygon*>::iterator it = multipolygons.begin(); it != multipolygons.end(); it++) {
		const char *extra_layer_str = (*it)->getAttribute("layer");
		double extra_layer = (extra_layer_str == NULL ? 0 : atof(extra_layer_str)/500);
		if (extra_layer < 0) extra_layer = 0;//!

		{
			stringstream s;
			s << "Forest with id " << (*it)->getId() << " - outline (tag " << key;
			if (value != NULL) s << "=" << value;
			s << ")";
			pov_writer->writeComment(s.str().c_str());
		}
		this->pov_writer->writePolygon(*it, floor_height+extra_layer, floor_style);
		{
			stringstream s;
			s << "Forest with id " << (*it)->getId() << " - trees (tag " << key;
			if (value != NULL) s << "=" << value;
			s << ")";
			pov_writer->writeComment(s.str().c_str());
		}

		const char *wood_style = (*it)->getAttribute("wood");
		size_t tree_style_min = tree_style_coniferous_min;
		size_t tree_style_max = tree_style_overall_max;
		if (wood_style != NULL) {
			if (strcmp(wood_style, "coniferous") == 0) tree_style_max = tree_style_coniferous_max;
			else if (strcmp(wood_style, "deciduous") == 0) tree_style_min = tree_style_coniferous_max+1;
		}

		vector<PointFieldItem*> trees;
		(*it)->computeRegularInsidePoints(&trees, &this->point_field, tree_style_min, tree_style_max);
		for (vector<PointFieldItem*>::iterator it2 = trees.begin(); it2 != trees.end(); it2++) {
			this->pov_writer->writeSprite((*it2)->xy->x, (*it2)->xy->y, tree_style_basic, (*it2)->item_type, 0.3);
			delete (*it2)->xy;
			delete *it2;
		}

		delete *it;
	}
}

void Osm2PovConverter::drawObjects(const char *key, const char *value, const char *style_basic, double scale, int min_variation, int max_variation) {
	list<const Node*> nodes;
	this->primitives->getNodesWithAttribute(&nodes, key, value);
	for (list<const Node*>::const_iterator it = nodes.begin(); it != nodes.end(); it++) {
		{
			stringstream s;
			s << "Node " << (*it)->getId() << " (tag " << key;
			if (value != NULL) s << "=" << value;
			s << ")";
			this->pov_writer->writeComment(s.str().c_str());
		}

		this->pov_writer->writeSprite((*it)->getLon(), (*it)->getLat(), style_basic, rand() % (max_variation-min_variation+1) + min_variation, scale);
	}
}

void Osm2PovConverter::drawBuildingWalls(const vector<const XY*> *points, double height, const char *style) {
	bool first = true;
	double x_before, y_before;

	double lon_before, lat_before;
	for (vector<const XY*>::const_iterator it = points->begin(); it != points->end(); it++) {
		double x = this->pov_writer->convertLonToCoord((*it)->x);
		double y = this->pov_writer->convertLatToCoord((*it)->y);

		if (first) {
			this->point_field.addPoint((*it)->x, (*it)->y, this->pov_writer->metres2unit(3.5)*2);
			first = false;
		}
		else {
			this->point_field.addPointsInDistance(lon_before, lat_before, (*it)->x, (*it)->y, this->pov_writer->metres2unit(3.5)*2);

			vector<double> coords;

			coords.push_back(x);
			coords.push_back(0);
			coords.push_back(y);

			coords.push_back(x);
			coords.push_back(this->pov_writer->metres2unit(height));
			coords.push_back(y);

			coords.push_back(x_before);
			coords.push_back(this->pov_writer->metres2unit(height));
			coords.push_back(y_before);

			coords.push_back(x_before);
			coords.push_back(0);
			coords.push_back(y_before);

			coords.push_back(x);
			coords.push_back(0);
			coords.push_back(y);

			Polygon3D polygon(0, coords);

			this->pov_writer->writePolygon(&polygon, style);
		}

		x_before = x; y_before = y;
		lon_before = (*it)->x; lat_before = (*it)->y;
	}
}

void Osm2PovConverter::drawBuilding(MultiPolygon *multipolygon, double height, const char *style, const char *roof_style) {
	{		//outer walls of building
		const list<vector<const XY*> > *outer_parts = multipolygon->getOuterParts();
		for (list<vector<const XY*> >::const_iterator it = outer_parts->begin(); it != outer_parts->end(); it++) {
			this->drawBuildingWalls(&(*it), height, style);
		}
	}
	{		//inner walls of building (if building have any)
		const list<vector<const XY*> > *holes = multipolygon->getHoles();
		for (list<vector<const XY*> >::const_iterator it = holes->begin(); it != holes->end(); it++) {
			this->drawBuildingWalls(&(*it), height, style);
		}
	}

			//roof
	this->pov_writer->writePolygon(multipolygon, height, roof_style);
}

void Osm2PovConverter::drawBuildings(const char *key, const char *value, double default_height, const char *style, const char *roof_style_default, const char *roof_style_religious) {
	list<MultiPolygon*> multipolygons;
	this->primitives->getMultiPolygonsWithAttribute(&multipolygons, key, value);
	for (list<MultiPolygon*>::iterator it = multipolygons.begin(); it != multipolygons.end(); it++) {
		const char *str;
		str = (*it)->getAttribute("layer");
		double extra_layer = (str == NULL ? 0 : atof(str)/500);
		if (extra_layer < 0) extra_layer = 0;//!

		double height = default_height;
		const char *roof_style = roof_style_default;
		if ((*it)->hasAttribute("amenity", "place_of_worship")) {
			height *= 2;
			roof_style = roof_style_religious;
		}
		str = (*it)->getAttribute("building:levels");
		if (str != NULL) height = 4 + (atof(str)-1) * 3;
		str = (*it)->getAttribute("building:height");
		if (str == NULL) str = (*it)->getAttribute("height");
		if (str != NULL) height = readDimension(str);

		{
			stringstream s;
			s << "Building " << (*it)->getId() << " (tag " << key;
			if (value != NULL) s << "=" << value;
			s << ")";
			this->pov_writer->writeComment(s.str().c_str());
		}

		this->drawBuilding(*it, height+extra_layer, style, roof_style);
		delete *it;
	}
}
