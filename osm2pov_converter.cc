
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

double Osm2PovConverter::computeWayWidth(const Way &way, double default_width) {
	double real_width = default_width; //use default width, when no other tag specified
	const char *width_str = way.getAttribute("width");
	const char *lanes_str = way.getAttribute("lanes");

	if (width_str != NULL) { //use width tag
		if (readDimension(width_str) > 0) real_width = readDimension(width_str);
	}
	else if (lanes_str != NULL) { //use lanes tag
		double lane_width = default_width/2; //assuming the default width is for 2 lanes
		if (lane_width > 5) lane_width = 5; //but use max. 4m per lane
		if (atof(lanes_str) > 0) real_width = lane_width * atof(lanes_str);
	}
	return real_width;
}

Osm2PovConverter::BuildingType Osm2PovConverter::getBuildingType(const MultiPolygon &building, double height, double min_height) {
	const char *use_str_p = building.getAttribute("building");
	if (use_str_p == NULL || strcmp(use_str_p, "yes") == 0) {   //building=yes says none about type
		use_str_p = building.getAttribute("building:use");   //...so try read building:use
	}
	
	//First try to parse use tag, if specified (see http://wiki.openstreetmap.org/wiki/Key:building)
	if (use_str_p != NULL) {
		string use_str = use_str_p;
		if (use_str == "house"
				|| use_str == "residential"
				|| use_str == "hut") {
			return BuildingType::living_building;
		}
		else if (use_str == "church") {
			return BuildingType::worship_building;
		}
		else if (building.hasAttribute("amenity", "place_of_worship")) {
			return BuildingType::worship_building;
		}
		else if (use_str == "bunker"
				|| use_str == "collapsed"
				|| use_str == "commercial"
				|| use_str == "detached"
				|| use_str == "entrance"
				|| use_str == "farm"
				|| use_str == "garage"
				|| use_str == "greenhouse"
				|| use_str == "hangar"
				|| use_str == "industrial"
				|| use_str == "manufacture"
				|| use_str == "office"
				|| use_str == "public"
				|| use_str == "retail"
				|| use_str == "roof"
				|| use_str == "school"
				|| use_str == "service"
				|| use_str == "storage_tank"
				|| use_str == "terasse"
				|| use_str == "transportation"
				|| use_str == "train_station"
				|| use_str == "university") {
			return BuildingType::nonliving_building;
		}
	}
	if (building.getAttribute("amenity") != NULL) {
		return BuildingType::nonliving_building;
	}

			//If nothing found, try to guess by building size		
	if (height > 10 || min_height != 0
		|| building.computeAreaSize() > 0.00000008) {	//empirically determined magic number
		return BuildingType::nonliving_building;
	}
	else return BuildingType::living_building;
}

void Osm2PovConverter::drawTowers(const char *key, const char *value, double width, double default_height, const char *style) {
	list<const Node*> nodes;
	this->primitives.getNodesWithAttribute(&nodes, key, value);
	for (list<const Node*>::iterator it = nodes.begin(); it != nodes.end(); it++) {
		{
			stringstream s;
			s << "Node " << (*it)->getId() << " (tag " << key;
			if (value != NULL) s << "=" << value;
			s << ")";
			this->pov_writer.writeComment(s.str().c_str());
		}

		double x = this->pov_writer.convertLonToCoord((*it)->getLon());
		double y = this->pov_writer.convertLatToCoord((*it)->getLat());

		this->point_field.addPoint((*it)->getLon(), (*it)->getLat(), metres2unit(width+1.5)*2);

		double height = default_height;
		const char *str = (*it)->getAttribute("height");
		if (str != NULL) height = readDimension(str);

		this->pov_writer.writeCylinder(x, y, metres2unit(width)/2, metres2unit(height), style);
	}
}

void Osm2PovConverter::drawWay(const vector<const Node*> &nodes, double width, double height, const char *style, bool including_links, bool links_also_in_margin) {
	double x_before, y_before;
	double lon_before, lat_before;
	for (size_t i = 0; i < nodes.size(); i++) {
		const Node *node = nodes[i];
		double x = this->pov_writer.convertLonToCoord(node->getLon());
		double y = this->pov_writer.convertLatToCoord(node->getLat());

		if (i == 0) {
			this->point_field.addPoint(node->getLon(), node->getLat(), metres2unit(width+1.5)*2);
		}
		else {
			this->point_field.addPointsInDistance(lon_before, lat_before, node->getLon(), node->getLat(), metres2unit(width+1.5)*2);
			double x_delta = x-x_before, y_delta = y-y_before;
			double angle = (-(atan2(y_delta,x_delta) * 180 / M_PI)) + 180;
			double length = sqrt(x_delta*x_delta+y_delta*y_delta);

			this->pov_writer.writeBox(x, y, metres2unit(width), metres2unit(height), length, angle, style);
		}

		if (including_links) {
			if (links_also_in_margin || (i > 0 && i < nodes.size()-1))
				this->pov_writer.writeCylinder(x, y, metres2unit(width)/2, metres2unit(height), style);
		}

		x_before = x; y_before = y;
		lon_before = node->getLon(); lat_before = node->getLat();
	}
}

void Osm2PovConverter::drawWays(const char *key, const char *value, double width, double height, const char *style, bool including_links, bool area_possible) {
	list<const Way*> ways;
	this->primitives.getWaysWithAttribute(&ways, key, value);
	for (list<const Way*>::iterator it = ways.begin(); it != ways.end(); it++) {
		if (area_possible && (*it)->hasAttribute("area", "yes")) {
			stringstream s;
			s << "Area (closed way with area=yes) " << (*it)->getId();
			this->pov_writer.writeComment(s.str().c_str());

			this->drawArea((*it)->getId(), (*it)->getNodes(), height+0.0001, strcmp(style,"highway") == 0 ? "highway_area" : style);
			continue;
		}
		const char *extra_layer_str = (*it)->getAttribute("layer");
		double extra_layer = (extra_layer_str == NULL ? 0 : atof(extra_layer_str)/500);
		if (extra_layer < 0) continue; //skip objects under the ground
		
		double real_width = computeWayWidth(**it, width);

		{
			stringstream s;
			s << "Way " << (*it)->getId() << " (tag " << key;
			if (value != NULL) s << "=" << value;
			s << ", width: " << real_width << "m" << ")";
			this->pov_writer.writeComment(s.str().c_str());
		}


		this->drawWay((*it)->getNodes(), real_width, height+extra_layer, style, including_links, true);
	}
}

void Osm2PovConverter::drawWaysWithBorder(const char *key, const char *value, double width, double height, const char *style, double border_width_percent, const char *border_style) {
	list<const Way*> ways;
	this->primitives.getWaysWithAttribute(&ways, key, value);
	for (list<const Way*>::iterator it = ways.begin(); it != ways.end(); it++) {
		if ((*it)->hasAttribute("area", "yes")) {
			stringstream s;
			s << "Area (closed way with area=yes) " << (*it)->getId() << " (tag " << key;
			if (value != NULL) s << "=" << value;
			s << ")";
			this->pov_writer.writeComment(s.str().c_str());

			drawArea((*it)->getId(), (*it)->getNodes(), height+0.0001, strcmp(style,"highway") == 0 ? "highway_area" : style);
			continue;
		}
		const bool is_tunnel = (*it)->hasAttribute("tunnel", "yes");
		const char *extra_layer_str = (*it)->getAttribute("layer");
		double extra_layer = (extra_layer_str == NULL ? 0 : atof(extra_layer_str)/500);
		if (extra_layer < 0 && !is_tunnel) continue; //skip objects under the ground
		if (is_tunnel) height /= 2;	//!
		
		double real_width = computeWayWidth(**it, width);

		{
			stringstream s;
			s << "Way " << (*it)->getId() << " with border (tag " << key;
			if (value != NULL) s << "=" << value;
			s << ", width: " << real_width << "m" << ")";
			this->pov_writer.writeComment(s.str().c_str());
		}
		drawWay((*it)->getNodes(), real_width, height-0.0011+extra_layer, border_style, true, extra_layer == 0);
		drawWay((*it)->getNodes(), real_width-border_width_percent*real_width/100*2, height+extra_layer, is_tunnel && strcmp(style, "highway") == 0 ? "highway_tunnel" : style, true, true);
	}
}

void Osm2PovConverter::drawArea(uint64_t area_id, const vector<const Node*> &nodes, double height, const char *style) {
	vector<double> coords;

	for (vector<const Node*>::const_iterator it2 = nodes.begin(); it2 != nodes.end(); it2++) {
		double lat = this->pov_writer.convertLatToCoord((*it2)->getLat());
		double lon = this->pov_writer.convertLonToCoord((*it2)->getLon());

		coords.push_back(lon);
		coords.push_back(metres2unit(height));
		coords.push_back(lat);
	}

	Polygon3D polygon(area_id, coords);
	this->pov_writer.writePolygon(polygon, style);
}

void Osm2PovConverter::drawAreas(const char *key, const char *value, double height, const char *style) {
	list<MultiPolygon*> multipolygons;
	this->primitives.getMultiPolygonsWithAttribute(&multipolygons, key, value);
	for (list<MultiPolygon*>::iterator it = multipolygons.begin(); it != multipolygons.end(); it++) {
		const char *extra_layer_str = (*it)->getAttribute("layer");
		double extra_layer = (extra_layer_str == NULL ? 0 : atof(extra_layer_str)/500);
		if (extra_layer < 0) extra_layer = 0;

		{
			stringstream s;
			s << "Area (closed way) " << (*it)->getId() << " (tag " << key;
			if (value != NULL) s << "=" << value;
			s << ")";
			this->pov_writer.writeComment(s.str().c_str());
		}

		vector<Triangle> triangles;
		(*it)->convertToTriangles(&triangles);
		this->pov_writer.writePolygon((*it)->getId(), triangles, height+extra_layer, style);

		delete *it;
	}
}

void Osm2PovConverter::drawForests(const char *key, const char *value, double floor_height, const char *floor_style, const char *tree_style_basic, size_t tree_style_coniferous_min, size_t tree_style_coniferous_max, size_t tree_style_overall_max) {
	list<MultiPolygon*> multipolygons;
	this->primitives.getMultiPolygonsWithAttribute(&multipolygons, key, value);
	for (list<MultiPolygon*>::iterator it = multipolygons.begin(); it != multipolygons.end(); it++) {
		const char *extra_layer_str = (*it)->getAttribute("layer");
		double extra_layer = (extra_layer_str == NULL ? 0 : atof(extra_layer_str)/500);
		if (extra_layer < 0) extra_layer = 0;

		{
			stringstream s;
			s << "Forest with id " << (*it)->getId() << " - outline (tag " << key;
			if (value != NULL) s << "=" << value;
			s << ")";
			this->pov_writer.writeComment(s.str().c_str());
		}

		vector<Triangle> triangles;
		(*it)->convertToTriangles(&triangles);
		this->pov_writer.writePolygon((*it)->getId(), triangles, floor_height+extra_layer, floor_style);
		{
			stringstream s;
			s << "Forest with id " << (*it)->getId() << " - trees (tag " << key;
			if (value != NULL) s << "=" << value;
			s << ")";
			this->pov_writer.writeComment(s.str().c_str());
		}

		const char *wood_style = (*it)->getAttribute("wood");
		size_t tree_style_min = tree_style_coniferous_min;
		size_t tree_style_max = tree_style_overall_max;
		if (wood_style != NULL) {
			if (strcmp(wood_style, "coniferous") == 0) tree_style_max = tree_style_coniferous_max;
			else if (strcmp(wood_style, "deciduous") == 0) tree_style_min = tree_style_coniferous_max+1;
		}

		vector<PointFieldItem*> trees;
		ComputeRegularInsidePoints(&triangles, &trees, &this->point_field, tree_style_min, tree_style_max);

		for (vector<PointFieldItem*>::iterator it2 = trees.begin(); it2 != trees.end(); it2++) {
			this->pov_writer.writeSprite((*it2)->xy->x, (*it2)->xy->y, tree_style_basic, (*it2)->item_type, 0.3);
			delete (*it2)->xy;
			delete *it2;
		}

		delete *it;
	}
}

void Osm2PovConverter::drawObjects(const char *key, const char *value, const char *style_basic, double scale, int min_variation, int max_variation) {
	list<const Node*> nodes;
	this->primitives.getNodesWithAttribute(&nodes, key, value);
	for (list<const Node*>::const_iterator it = nodes.begin(); it != nodes.end(); it++) {
		{
			stringstream s;
			s << "Node " << (*it)->getId() << " (tag " << key;
			if (value != NULL) s << "=" << value;
			s << ")";
			this->pov_writer.writeComment(s.str().c_str());
		}

		this->pov_writer.writeSprite((*it)->getLon(), (*it)->getLat(), style_basic, rand() % (max_variation-min_variation+1) + min_variation, scale);
	}
}

void Osm2PovConverter::drawBuildingWalls(const vector<XY> &points, double min_height, double height, const char *style) {
	bool first = true;
	double x_before, y_before;

	double lon_before, lat_before;
	for (vector<XY>::const_iterator it = points.begin(); it != points.end(); it++) {
		double x = this->pov_writer.convertLonToCoord(it->x);
		double y = this->pov_writer.convertLatToCoord(it->y);

		if (first) {
			this->point_field.addPoint(it->x, it->y, metres2unit(3.5)*2);
			first = false;
		}
		else {
			this->point_field.addPointsInDistance(lon_before, lat_before, it->x, it->y, metres2unit(3.5)*2);

			vector<double> coords;

			coords.push_back(x);
			coords.push_back(metres2unit(min_height));
			coords.push_back(y);

			coords.push_back(x);
			coords.push_back(metres2unit(height));
			coords.push_back(y);

			coords.push_back(x_before);
			coords.push_back(metres2unit(height));
			coords.push_back(y_before);

			coords.push_back(x_before);
			coords.push_back(metres2unit(min_height));
			coords.push_back(y_before);

			coords.push_back(x);
			coords.push_back(metres2unit(min_height));
			coords.push_back(y);

			Polygon3D polygon(0, coords);

			this->pov_writer.writePolygon(polygon, style);
		}

		x_before = x; y_before = y;
		lon_before = it->x; lat_before = it->y;
	}
}

void Osm2PovConverter::drawBuilding(const MultiPolygon &multipolygon, double min_height, double height, const char *style, const char *roof_style) {
	{		//outer walls of building
		const list<vector<XY> > &outer_parts = multipolygon.getOuterParts();
		for (list<vector<XY> >::const_iterator it = outer_parts.begin(); it != outer_parts.end(); it++) {
			this->drawBuildingWalls(*it, min_height, height, style);
		}
	}
	{		//inner walls of building (if building have any)
		const list<vector<XY> > &holes = multipolygon.getHoles();
		for (list<vector<XY> >::const_iterator it = holes.begin(); it != holes.end(); it++) {
			this->drawBuildingWalls(*it, min_height, height, style);
		}
	}

	if (roof_style != NULL) {		//roof
		vector<Triangle> triangles;
		multipolygon.convertToTriangles(&triangles);
		this->pov_writer.writePolygon(multipolygon.getId(), triangles, height, roof_style);
	}

	if (min_height != 0) {		//when building isn't at floor, render floor too
		vector<Triangle> triangles;
		multipolygon.convertToTriangles(&triangles);
		this->pov_writer.writePolygon(multipolygon.getId(), triangles, min_height, style);
	}
}

void Osm2PovConverter::drawBuildings(const char *key, const char *value, double default_height, const vector<const char*> &style, const vector<const char*> &roof_style_living, const vector<const char*> &roof_style_nonliving, const vector<const char*> &roof_style_religious) {
	list<MultiPolygon*> multipolygons;
	this->primitives.getMultiPolygonsWithAttribute(&multipolygons, key, value);
	for (list<MultiPolygon*>::iterator it = multipolygons.begin(); it != multipolygons.end(); it++) {
		const char *str;
		str = (*it)->getAttribute("layer");
		double extra_layer = (str == NULL ? 0 : atof(str)/500);
		if (extra_layer < 0) continue; //skip objects under the ground

		 //don't render towers twice (later as special building) (this isn't nice piece of code - duplicating)
		if ((*it)->hasAttribute("man_made", "tower")) continue;
		if ((*it)->hasAttribute("amenity", "tower")) continue;
		if ((*it)->hasAttribute("man_made", "chimney")) continue;


		double height = default_height;
		
		bool height_defined = false;
		str = (*it)->getAttribute("building:levels");
		if (str != NULL) {
			height = 4 + (atof(str)-1) * 3;
			height_defined = true;
		}
		
		str = (*it)->getAttribute("building:height");
		if (str == NULL) str = (*it)->getAttribute("height");
		if (str != NULL) {
			height = readDimension(str);
			height_defined = true;
		}
		double min_height = 0;
		str = (*it)->getAttribute("min_height");		//building that haven't on floor
		if (str != NULL) {
			min_height = atof(str);
			extra_layer = 0;			//when has building min_height, it's more precise than layer
		}
		
		{
			stringstream s;
			s << "Building " << (*it)->getId() << " (tag " << key;
			if (value != NULL) s << "=" << value;
			s << ")";
			this->pov_writer.writeComment(s.str().c_str());
		}

		BuildingType building_type = getBuildingType(**it, height, min_height);

		const vector<const char*> *roof_style;
		switch (building_type) {
			case living_building:
				roof_style = &roof_style_living;
				if (!height_defined) height += (double)((*it)->getId() % 5)/2 - 1;   //building has slightly random height
				break;
			case nonliving_building:
				roof_style = &roof_style_nonliving;
				break;
			case worship_building:
				roof_style = &roof_style_religious;
				if (!height_defined) height *= 2;
				break;
		}

		this->drawBuilding(**it, min_height, height+extra_layer, style[(*it)->getId() % style.size()], (*roof_style)[(*it)->getId() % roof_style->size()]);
		delete *it;
	}
}

void Osm2PovConverter::drawSpecialBuildings(const char *key, const char *value, double default_height, const char *style, const char *roof_style) {
	list<MultiPolygon*> multipolygons;
	this->primitives.getMultiPolygonsWithAttribute(&multipolygons, key, value);
	for (list<MultiPolygon*>::iterator it = multipolygons.begin(); it != multipolygons.end(); it++) {
		const char *str;
		str = (*it)->getAttribute("layer");
		double extra_layer = (str == NULL ? 0 : atof(str)/500);
		if (extra_layer < 0) continue; //skip objects under the ground

		double height = default_height;
		str = (*it)->getAttribute("building:levels");
		if (str != NULL) height = 4 + (atof(str)-1) * 3;
		str = (*it)->getAttribute("building:height");
		if (str == NULL) str = (*it)->getAttribute("height");
		if (str != NULL) height = readDimension(str);
		
		double min_height = 0;
		str = (*it)->getAttribute("min_height");		//building that isn't on floor
		if (str != NULL) {
			min_height = atof(str);
			extra_layer = 0;			//when has building min_height, it's more precise than layer
		}

		{
			stringstream s;
			s << "Building " << (*it)->getId() << " (tag " << key;
			if (value != NULL) s << "=" << value;
			s << ")";
			this->pov_writer.writeComment(s.str().c_str());
		}

		this->drawBuilding(**it, min_height, height+extra_layer, style, roof_style);
		delete *it;
	}
}
