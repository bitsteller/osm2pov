
#ifndef OSM2POV_CONVERTER_H_
#define OSM2POV_CONVERTER_H_

class Osm2PovConverter {
	private:
	class Primitives *primitives;
	class PovWriter *pov_writer;
	PointField point_field;

	static double readDimension(const char *dimension_text);
	void drawWay(const vector<const class Node*> *nodes, double width, double height, const char *style, bool including_links, bool links_also_in_margin);
	void drawBuilding(class MultiPolygon *multipolygon, double height, const char *style, const char *roof_style);
	void drawBuildingWalls(const vector<const class XY*> *points, double height, const char *style);
	void drawArea(uint64_t area_id, const vector<const class Node*> *nodes, double height, const char *style);

	public:
	Osm2PovConverter(Primitives *primitives, PovWriter *pov_writer) : primitives(primitives), pov_writer(pov_writer) { }
	void drawTowers(const char *key, const char *value, double width, double default_height, const char *style);
	void drawWays(const char *key, const char *value, double width, double height, const char *style, bool including_links, bool area_possible);
	void drawWaysWithBorder(const char *key, const char *value, double width, double height, const char *style, double border_width_percent, const char *border_style);
	void drawAreas(const char *key, const char *value, double height, const char *style);
	void drawForests(const char *key, const char *value, double floor_height, const char *floor_style, const char *tree_style_basic, size_t tree_style_coniferous_min, size_t tree_style_coniferous_max, size_t tree_style_ovwerall_max);
	void drawObjects(const char *key, const char *value, const char *style_basic, double scale, int min_variation, int max_variation);
	void drawBuildings(const char *key, const char *value, double default_height, const vector<const char*> &style, const vector<const char*> &roof_style_living, const vector<const char*> &roof_style_nonliving, const vector<const char*> &roof_style_religious);
	void drawSpecialBuildings(const char *key, const char *value, double default_height, const char *style, const char *roof_style);
};


#endif /* OSM2POV_CONVERTER_H_ */
