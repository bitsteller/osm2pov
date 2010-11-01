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


int main(int argc, const char **argv) {
	if (argc != 3 && argc != 5) {
		cout << "Osm2Pov " << VERSION;
		cout << "\tAuthor Aleš Janda | See http://osm.kyblsoft.cz/3dmapa/info for details" << endl << endl;
		cout << "Using:\t" << argv[0] << " input.osm output.pov [X Y]" << endl << endl;
		cout << "By default, area is computed from OSM file. If you can use it for render part of bigger map, set X and Y parameters. These are coords of tiles of zoom 12, where Y is divided by 2." << endl;
		cout << "Currently, zoom of output model (and image) is always the same." << endl;
		return 1;
	}

	Primitives primitives;
        bool fix_size_to_square = true;

	if (argc == 5) {
		primitives.setBoundsByXY(atol(argv[3]), atol(argv[4]));
                fix_size_to_square = false;
	}

	cout << "Loading input file" << endl;

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
	if (!primitives.loadFromXml(argv[1])) return 1;

	cout << "Writing POV file" << endl;
	PovWriter pov_writer(argv[2], primitives.getMinLat(), primitives.getMinLon(), primitives.getMaxLat(), primitives.getMaxLon(), fix_size_to_square);
	if (!pov_writer.isOpened()) return 1;

	Osm2PovConverter osm2pov_converter(&primitives, &pov_writer);

	//generating objects
	osm2pov_converter.drawWaysWithBorder("highway", "motorway", 10, 0.06, "highway", 10, "highway_secondary_border");
	osm2pov_converter.drawWaysWithBorder("highway", "motorway_link", 5, 0.06, "highway", 10, "highway_border");
	osm2pov_converter.drawWaysWithBorder("highway", "primary", 8, 0.06, "highway", 10, "highway_secondary_border");
  osm2pov_converter.drawWaysWithBorder("highway", "primary_link", 5, 0.06, "highway", 10, "highway_border");
  osm2pov_converter.drawWaysWithBorder("highway", "trunk", 7, 0.06, "highway", 10, "highway_secondary_border");
	osm2pov_converter.drawWaysWithBorder("highway", "trunk_link", 4, 0.06, "highway", 10, "highway_border");
	osm2pov_converter.drawWaysWithBorder("highway", "secondary", 6, 0.06, "highway", 10, "highway_secondary_border");
	osm2pov_converter.drawWaysWithBorder("highway", "tertiary", 5.5, 0.06, "highway", 10, "highway_border");
	osm2pov_converter.drawWaysWithBorder("highway", "unclassified", 5, 0.06, "highway", 10, "highway_border");
	osm2pov_converter.drawWaysWithBorder("highway", "road", 5, 0.06, "highway", 10, "highway_border");

	osm2pov_converter.drawWays("highway", "residential", 5, 0.06, "highway", true);
	osm2pov_converter.drawWays("highway", "living_street", 5, 0.06, "highway", true);
	osm2pov_converter.drawWays("highway", "service", 4, 0.06, "highway", true);
	osm2pov_converter.drawWays("highway", "pedestrian", 4, 0.06, "highway", true);

	osm2pov_converter.drawWays("highway", "track", 3, 0.045, "path", true);
	osm2pov_converter.drawWays("highway", "footway", 1.2, 0.03, "footway", true);
	osm2pov_converter.drawWays("highway", "steps", 1.2, 0.03, "footway", true);
	osm2pov_converter.drawWays("highway", "cycleway", 1.5, 0.03, "footway", true);
	osm2pov_converter.drawWays("highway", "path", 1, 0.015, "path", true);
	osm2pov_converter.drawAreas("amenity", "parking", 0.0009, "highway_area");

	osm2pov_converter.drawWays("railway", "rail", 4.5, 0.09, "railway", false);
	osm2pov_converter.drawWays("railway", "abandoned", 4.5, 0.09, "railway", false);
	osm2pov_converter.drawWays("railway", "disused", 4.5, 0.09, "railway", false);
	osm2pov_converter.drawWays("railway", "tram", 2.25, 0.12, "railway_tram", true);

	osm2pov_converter.drawAreas("leisure", "playground", 0.013, "playground");
	osm2pov_converter.drawAreas("leisure", "pitch", 0.013, "playground");

	osm2pov_converter.drawWays("aeroway", "runway", 40, 0.07, "highway", true);
	osm2pov_converter.drawWays("aeroway", "taxiway", 7, 0.07, "highway", true);

	osm2pov_converter.drawAreas("landuse", "cemetery", 0.012, "cemetery");
	osm2pov_converter.drawAreas("natural", "beach", 0.012, "beach");

	osm2pov_converter.drawBuildings("building", NULL, 4.5, "building", "building_roof", "building_religious_roof");

	osm2pov_converter.drawBuildings("leisure", "stadium", 12, "man_made_tower", "man_made_tower");
	osm2pov_converter.drawTowers("artwork_type", "obelisk", 4, 25, "man_made_tower"); //FIXME: for testing only
	osm2pov_converter.drawTowers("man_made", "tower", 4, 25, "man_made_tower");
	osm2pov_converter.drawTowers("amenity", "tower", 4, 25, "man_made_tower");
	osm2pov_converter.drawBuildings("man_made", "tower", 25, "man_made_tower", "building_roof");
	osm2pov_converter.drawBuildings("amenity", "tower", 25, "man_made_tower", "building_roof");
	osm2pov_converter.drawWays("barrier", "wall", 0.3, 3, "wall", true);

	osm2pov_converter.drawObjects("power_source", "wind", "windpower", 1.5, 1, 1);
	osm2pov_converter.drawObjects("amenity", "post_box", "postbox", 0.1, 1, 1);
	osm2pov_converter.drawObjects("natural", "tree", "tree", 0.2, 1, 6);
	osm2pov_converter.drawForests("natural", "wood", 0.00011, "forest", "tree", 1, 1, 6);
	osm2pov_converter.drawForests("landuse", "forest", 0.00011, "forest", "tree", 1, 1, 6);
	osm2pov_converter.drawForests("landuse", "wood", 0.00011, "forest", "tree", 1, 1, 6);

	osm2pov_converter.drawAreas("nature", "scrub", 0.00012, "greenplace");
	osm2pov_converter.drawForests("leisure", "park", 0.00012, "greenplace", "tree", 1, 1, 6);
	osm2pov_converter.drawAreas("landuse", "village_green", 0.00012, "greenplace");
	osm2pov_converter.drawAreas("landuse", "allotments", 0.00012, "greenplace");

	osm2pov_converter.drawAreas("landuse", "industrial", 0.00009, "landuse_industrial");
	osm2pov_converter.drawAreas("landuse", "residential", 0.00009, "landuse_residential");

	osm2pov_converter.drawWays("waterway", "stream", 1, 0.00013, "river", true);
	osm2pov_converter.drawWays("waterway", "canal", 2.5, 0.00013, "river", true);
	osm2pov_converter.drawWays("waterway", "river", 5, 0.00013, "river", true);
	osm2pov_converter.drawAreas("waterway", "riverbank", 0.00013, "river");
	osm2pov_converter.drawAreas("natural", "water", 0.00013, "river");
	osm2pov_converter.drawAreas("landuse", "basin", 0.00013, "river");
	osm2pov_converter.drawAreas("landuse", "reservoir", 0.00013, "river");

/*	{		//info about not used tags. Now isn't used :-)
		multimap<size_t,string> disused_attributes;
		primitives.getDisusedAttributes(&disused_attributes);
		if (!disused_attributes.empty()) {
			cout << "Disused attributes:" << endl;
			for (multimap<size_t,string>::reverse_iterator it = disused_attributes.rbegin(); it != disused_attributes.rend(); it++) {
				cout << it->first << "x " << it->second << endl;
			}
		}
	}
*/
	cout << "Done." << endl;
}
