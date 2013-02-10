/**
 * Osm2Pov v0.1 - Copyleft Aleš Janda
 * This software is under GPL v3
 * See http://osm.kyblsoft.cz/3dmapa/info for more information
 */

#ifndef VERSION
 #define VERSION "undefined version"
#endif

#include <math.h>

#include "global.h"
#include "point_field.h"
#include "osm2pov_converter.h"
#include "output_polygon.h"
#include "pov_writer.h"
#include "primitives.h"

bool g_quiet_mode = false;

static void PrintHelpAndExit() {
	cout << "Osm2Pov " << VERSION;
	cout << "\tAuthor Aleš Janda | See http://osm.kyblsoft.cz/3dmapa/info for details" << endl << endl;
	cout << "Using:\tosm2pov [-q] input.osm output.pov [X Y]" << endl;
	cout << "\t-q means \"quiet\" - suppress common errors and no standard output" << endl << endl;
	cout << "By default, area is computed from OSM file. If you can use it for render part of bigger map, set X and Y parameters. These are coords of tiles of zoom 12, where Y is divided by 2." << endl;
	cout << "Currently, zoom of output model (and image) is always the same." << endl;
	exit(1);
}

int main(int argc, const char **argv) {
	int argc_i = 1;
	if (argc_i >= argc) PrintHelpAndExit();
	if (strcmp(argv[argc_i], "-q") == 0) {
		g_quiet_mode = true;
		argc_i++;
		if (argc_i >= argc) PrintHelpAndExit();
	}
	const char *input_filename = argv[argc_i++];
	if (argc_i >= argc) PrintHelpAndExit();
	const char *output_filename = argv[argc_i++];

	Primitives primitives;
	bool fix_size_to_square = true;

	if (argc_i+2 == argc) {
		const int x = atoi(argv[argc_i++]);
		const int y = atoi(argv[argc_i++]);

		primitives.setBoundsByXY(x, y);
		fix_size_to_square = false;
	}
	else if (argc_i != argc)
		PrintHelpAndExit();


	if (!g_quiet_mode) cout << "Loading input file" << endl;

	//setting attributes that are ignored when read OSM file. These attributes will not used (for memory saving)
	primitives.setIgnoredAttribute("addr:city", NULL);
	primitives.setIgnoredAttribute("addr:conscriptionnumber", NULL);
	primitives.setIgnoredAttribute("addr:country", NULL);
	primitives.setIgnoredAttribute("addr:housenumber", NULL);
	primitives.setIgnoredAttribute("addr:provisionalnumber", NULL);
	primitives.setIgnoredAttribute("addr:postcode", NULL);
	primitives.setIgnoredAttribute("addr:street", NULL);
	primitives.setIgnoredAttribute("addr:streetnumber", NULL);
	primitives.setIgnoredAttribute("admin_level", NULL);
	primitives.setIgnoredAttribute("alt_name", NULL);
	primitives.setIgnoredAttribute("bicycle", NULL);
	primitives.setIgnoredAttribute("complete", NULL);
	primitives.setIgnoredAttribute("created_by", NULL);
	primitives.setIgnoredAttribute("description", NULL);
	primitives.setIgnoredAttribute("dibavod:id", NULL);
	primitives.setIgnoredAttribute("is_in", NULL);
	primitives.setIgnoredAttribute("is_in:continent", NULL);
	primitives.setIgnoredAttribute("is_in:country_code", NULL);
	primitives.setIgnoredAttribute("FIXME", NULL);
	primitives.setIgnoredAttribute("foot", NULL);
	primitives.setIgnoredAttribute("full_name", NULL);
	primitives.setIgnoredAttribute("kct_blue", NULL);
	primitives.setIgnoredAttribute("kct_yellow", NULL);
	primitives.setIgnoredAttribute("maxspeed", NULL);
	primitives.setIgnoredAttribute("motorcycle", NULL);
	primitives.setIgnoredAttribute("motorcar", NULL);
	primitives.setIgnoredAttribute("name", NULL);
	primitives.setIgnoredAttribute("note", NULL);
	primitives.setIgnoredAttribute("name", NULL);
	primitives.setIgnoredAttribute("old_name", NULL);
	primitives.setIgnoredAttribute("operator", NULL);
	primitives.setIgnoredAttribute("place", NULL);
	primitives.setIgnoredAttribute("ref", NULL);
	primitives.setIgnoredAttribute("ref:bridge", NULL);
	primitives.setIgnoredAttribute("route", NULL);
	primitives.setIgnoredAttribute("source", NULL);
	primitives.setIgnoredAttribute("source:addr", NULL);
	primitives.setIgnoredAttribute("source:name", NULL);
	primitives.setIgnoredAttribute("source:loc", NULL);
	primitives.setIgnoredAttribute("source:position", NULL);
	primitives.setIgnoredAttribute("source:ref", NULL);
	primitives.setIgnoredAttribute("street", NULL);	//?
	primitives.setIgnoredAttribute("todo", NULL);
	primitives.setIgnoredAttribute("uir_adr:ADRESA_KOD", NULL);
	primitives.setIgnoredAttribute("voltage", NULL);
	primitives.setIgnoredAttribute("wifi", NULL);

	// set attributes that it remember but it will not be searched, only using as specify other tags
	// this all is for warnings what tags wasn't used
	primitives.setLightlyIgnoredAttribute("bridge", "yes");
	primitives.setLightlyIgnoredAttribute("building:height", NULL);
	primitives.setLightlyIgnoredAttribute("building:levels", NULL);
	primitives.setLightlyIgnoredAttribute("have_riverbank", "yes");
	primitives.setLightlyIgnoredAttribute("height", NULL);
	primitives.setLightlyIgnoredAttribute("layer", NULL);
	primitives.setLightlyIgnoredAttribute("type", "multipolygon");
	primitives.setLightlyIgnoredAttribute("wood", NULL);

	//loading from file
	if (!primitives.loadFromXml(input_filename)) return 1;

	if (!g_quiet_mode) cout << "Writing POV file" << endl;
	PovWriter pov_writer(output_filename, primitives.getViewRect(), fix_size_to_square);
	if (!pov_writer.isOpened()) return 1;

	Osm2PovConverter osm2pov_converter(primitives, pov_writer);

	//generating objects:
	
	//ground level
	osm2pov_converter.drawAreas("landuse", "farmland", 0.001, "landuse_farmland");
	osm2pov_converter.drawAreas("landuse", "farm", 0.001, "landuse_farmland");
	osm2pov_converter.drawAreas("landuse", "farmyard", 0.001, "landuse_farmland");
	osm2pov_converter.drawForests("landuse", "forest", 0.001, "forest", "tree", 1, 1, 6);
	osm2pov_converter.drawForests("landuse", "wood", 0.001, "forest", "tree", 1, 1, 6);
	osm2pov_converter.drawForests("natural", "wood", 0.001, "forest", "tree", 1, 1, 6);
	
	osm2pov_converter.drawAreas("landuse", "residential", 0.002, "landuse_residential");
	osm2pov_converter.drawAreas("landuse", "industrial", 0.002, "landuse_industrial");
    osm2pov_converter.drawAreas("landuse", "commercial", 0.002, "landuse_industrial");
    osm2pov_converter.drawAreas("landuse", "retail", 0.002, "landuse_industrial");
    osm2pov_converter.drawAreas("landuse", "railway", 0.002, "landuse_industrial");
	osm2pov_converter.drawAreas("amenity", "parking", 0.002, "highway_area");
	
	osm2pov_converter.drawAreas("landuse", "allotments", 0.003, "greenplace");
	osm2pov_converter.drawAreas("landuse", "meadow", 0.003, "greenplace");
	osm2pov_converter.drawAreas("landuse", "greenfield", 0.003, "greenplace");
    osm2pov_converter.drawAreas("landuse", "vineyard", 0.003, "greenplace");
	osm2pov_converter.drawAreas("nature", "scrub", 0.003, "greenplace");
	osm2pov_converter.drawForests("leisure", "park", 0.003, "greenplace", "tree", 1, 1, 6);
	osm2pov_converter.drawForests("leisure", "garden", 0.003, "greenplace", "tree", 1, 1, 6);
	osm2pov_converter.drawAreas("natural", "beach", 0.003, "beach");

	osm2pov_converter.drawAreas("landuse", "village_green", 0.004, "greenplace");
	osm2pov_converter.drawAreas("landuse", "cemetery", 0.004, "cemetery");
	osm2pov_converter.drawAreas("leisure", "playground", 0.004, "playground");
	osm2pov_converter.drawAreas("leisure", "pitch", 0.004, "playground");
	
	//way level
	osm2pov_converter.drawWays("waterway", "drain", 1, 0.01, "river", true, true);
	osm2pov_converter.drawWays("waterway", "stream", 2, 0.01, "river", true, true);
	osm2pov_converter.drawWays("waterway", "canal", 2.5, 0.01, "river", true, true);
	osm2pov_converter.drawWays("waterway", "river", 5, 0.01, "river", true, true);
	osm2pov_converter.drawAreas("waterway", "dock", 0.01, "river");
	osm2pov_converter.drawAreas("waterway", "riverbank", 0.01, "river");
	osm2pov_converter.drawAreas("natural", "water", 0.01, "river");
	osm2pov_converter.drawAreas("landuse", "basin", 0.01, "river");
	osm2pov_converter.drawAreas("landuse", "reservoir", 0.01, "river");
	
	osm2pov_converter.drawWays("highway", "path", 1.2, 0.02, "path", true, false);
	osm2pov_converter.drawWays("highway", "track", 3, 0.02, "path", true, false);
	
	osm2pov_converter.drawWays("highway", "pedestrian", 4, 0.03, "highway", true, true);
	osm2pov_converter.drawWays("highway", "footway", 2, 0.03, "footway", true, false);
	osm2pov_converter.drawWays("highway", "steps", 2, 0.03, "footway", true, false);
	osm2pov_converter.drawWays("highway", "cycleway", 2.5, 0.03, "footway", true, false);
	
	osm2pov_converter.drawWays("railway", "preserved", 3, 0.04, "railway", false, false);
	osm2pov_converter.drawWays("aeroway", "runway", 40, 0.04, "highway", true, true);
	osm2pov_converter.drawWays("aeroway", "taxiway", 7, 0.04, "highway", true, true);
	
	osm2pov_converter.drawWays("highway", "residential", 5, 0.05, "highway", true, true);
	osm2pov_converter.drawWays("highway", "living_street", 5, 0.05, "highway", true, true);
	osm2pov_converter.drawWays("highway", "service", 4, 0.05, "highway", true, true);
	
	osm2pov_converter.drawWaysWithBorder("highway", "unclassified", 6, 0.06, "highway", 10, "highway_border");
	osm2pov_converter.drawWaysWithBorder("highway", "road", 6, 0.06, "highway", 10, "highway_border");
	osm2pov_converter.drawWaysWithBorder("highway", "tertiary", 6.5, 0.06, "highway", 10, "highway_border");
	osm2pov_converter.drawWaysWithBorder("highway", "secondary", 7, 0.06, "highway", 10, "highway_secondary_border");
	osm2pov_converter.drawWaysWithBorder("highway", "primary", 8, 0.06, "highway", 10, "highway_secondary_border");
	osm2pov_converter.drawWaysWithBorder("highway", "primary_link", 5.5, 0.06, "highway", 10, "highway_border");
	osm2pov_converter.drawWaysWithBorder("highway", "trunk", 7, 0.06, "highway", 10, "highway_secondary_border");
	osm2pov_converter.drawWaysWithBorder("highway", "trunk_link", 5, 0.06, "highway", 10, "highway_border");
	osm2pov_converter.drawWaysWithBorder("highway", "motorway", 10, 0.06, "highway", 10, "highway_secondary_border");
	osm2pov_converter.drawWaysWithBorder("highway", "motorway_link", 5.5, 0.06, "highway", 10, "highway_border");
	
	osm2pov_converter.drawWays("railway", "abandoned", 3, 0.07, "railway", false, false);
	osm2pov_converter.drawWays("railway", "disused", 3, 0.07, "railway", false, false);
	osm2pov_converter.drawWays("railway", "narrow_gauge", 3, 0.07, "railway", false, false);
	osm2pov_converter.drawWays("railway", "rail", 5, 0.07, "railway", false, false);
	
	osm2pov_converter.drawWays("railway", "tram", 2.25, 0.08, "railway_tram", true, false);
	
	//buildings level
	osm2pov_converter.drawObjects("power_source", "wind", "windpower", 1.5, 1, 1);
	osm2pov_converter.drawObjects("amenity", "post_box", "postbox", 0.1, 1, 1);
	osm2pov_converter.drawObjects("natural", "tree", "tree", 0.2, 1, 6);
	
	osm2pov_converter.drawBuildings("building", NULL, 4.5, { "building" }, { "building_living_roof1", "building_living_roof2", "building_living_roof3", "building_living_roof4" }, { "building_nonliving_roof1", "building_nonliving_roof2" }, { "building_religious_roof" });

	osm2pov_converter.drawSpecialBuildings("leisure", "stadium", 12, "man_made_tower", "man_made_tower");
	osm2pov_converter.drawSpecialBuildings("building:part", NULL, 3, "building", "building");
	osm2pov_converter.drawTowers("artwork_type", "obelisk", 4, 25, "man_made_tower"); //FIXME: for testing only
	osm2pov_converter.drawTowers("man_made", "tower", 4, 25, "man_made_tower");
	osm2pov_converter.drawTowers("amenity", "tower", 4, 25, "man_made_tower");
	osm2pov_converter.drawSpecialBuildings("man_made", "tower", 25, "man_made_tower", "building_nonliving_roof1");
	osm2pov_converter.drawSpecialBuildings("amenity", "tower", 25, "man_made_tower", "building_nonliving_roof1");
	osm2pov_converter.drawSpecialBuildings("man_made", "chimney", 50, "man_made_tower", NULL);
	osm2pov_converter.drawWays("barrier", "wall", 0.3, 3, "wall", true, false);
	
	if (!g_quiet_mode) cout << "Done." << endl;
}
