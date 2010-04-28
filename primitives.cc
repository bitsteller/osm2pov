
#include <expat.h>

#include "global.h"
#include "output_polygon.h"
#include "primitives.h"


Primitives::Primitives() {
	this->bounds_set = false;
	this->bounds_set_by_x_y = false;
}

Primitives::~Primitives() {
	for (unordered_map<uint64_t,Node*>::iterator it = this->nodes.begin(); it != this->nodes.end(); it++) {
		delete it->second;
	}
	for (unordered_map<uint64_t,Way*>::iterator it = this->ways.begin(); it != this->ways.end(); it++) {
		delete it->second;
	}
	for (unordered_map<uint64_t,Relation*>::iterator it = this->relations.begin(); it != this->relations.end(); it++) {
		delete it->second;
	}
}

void Primitives::setBoundsByXY(size_t tile_x, size_t tile_y) {
	this->bounds_set_by_x_y = true;
	this->minlat = this->halftiley2lat(tile_y+1, 12);
	this->maxlat = this->halftiley2lat(tile_y, 12);
	this->minlon = this->tilex2lon(tile_x, 12);
	this->maxlon = this->tilex2lon(tile_x+1, 12);
}

void Primitives::setIgnoredAttribute(const char *key, const char *value) {
	this->ignored_attributes[key] = value;
}

bool Primitives::isAttributeIgnored(const char *key, const char *value) const {
	unordered_map<string,const char*>::const_iterator it = this->ignored_attributes.find(key);
	if (it == this->ignored_attributes.end()) return false;
	else if (value == NULL || it->second == NULL || strcmp(it->second,value) == 0) return true;
	else return false;
}

void Primitives::setLightlyIgnoredAttribute(const char *key, const char *value) {
	this->lightly_ignored_attributes[key] = value;
}

bool Primitives::isAttributeLightlyIgnored(const char *key, const char *value) const {
	unordered_map<string,const char*>::const_iterator it = this->lightly_ignored_attributes.find(key);
	if (it == this->ignored_attributes.end()) return false;
	else if (value == NULL || it->second == NULL || strcmp(it->second,value) == 0) return true;
	else return false;
}

void Primitives::setExistingAttribute(const char *key, const char *value) {
	string itkey = string(key) + "=" + value;
	unordered_map<string,size_t>::iterator it = this->disused_attributes.find(itkey);
	if (it == this->disused_attributes.end()) this->disused_attributes[itkey] = 1;
	else it->second++;
}

void Primitives::setBounds(double minlat, double minlon, double maxlat, double maxlon) {
	this->minlat = minlat;
	this->minlon = minlon;
	this->maxlat = maxlat;
	this->maxlon = maxlon;
	this->bounds_set = true;
}

void Primitives::getNodesWithAttribute(list<const Node*> *output, const char *key, const char *value) {
	for (unordered_map<uint64_t,Node*>::const_iterator it = this->nodes.begin(); it != this->nodes.end(); it++) {
		const char *value_now = it->second->getAttribute(key);
		if (value_now != NULL) {
			if (value == NULL || strcmp(value, value_now) == 0) output->push_back(it->second);
		}
	}

	unordered_map<string,size_t>::const_iterator it = this->disused_attributes.find(string(key)+"="+value);
	if (it != this->disused_attributes.end()) this->disused_attributes.erase(it);
}

void Primitives::getWaysWithAttribute(list<const Way*> *output, const char *key, const char *value) {
	for (unordered_map<uint64_t,Way*>::const_iterator it = this->ways.begin(); it != this->ways.end(); it++) {
		const char *value_now = it->second->getAttribute(key);
		if (value_now != NULL) {
			if (value == NULL || strcmp(value, value_now) == 0) output->push_back(it->second);
		}
	}

	unordered_map<string,size_t>::const_iterator it = this->disused_attributes.find(string(key)+"="+value);
	if (it != this->disused_attributes.end()) this->disused_attributes.erase(it);
}

void Primitives::getMultiPolygonsWithAttribute(list<MultiPolygon*> *output, const char *key, const char *value) {
	unordered_set<uint64_t> ids_used_in_relations;

	for (unordered_map<uint64_t,Relation*>::const_iterator it = this->relations.begin(); it != this->relations.end(); it++) {
		const char *value_now = it->second->getAttribute(key);
		if (value_now != NULL && (value == NULL || strcmp(value, value_now) == 0)) {
			MultiPolygon *multipolygon = new MultiPolygon(it->second);
			const vector<const PrimitiveRole*> *members = it->second->getRelationMembers();

			for (vector<const PrimitiveRole*>::const_iterator it2 = members->begin(); it2 != members->end(); it2++) {
				if ((*it2)->role == "outer") {
					const Way *way = dynamic_cast<const Way*>((*it2)->primitive);
					if (way == NULL) cerr << "Primitive with id " << (*it2)->primitive->getId() << " has role=outer and isn't way, ignoring." << endl;
					else {
						multipolygon->addOuterPart(way);
						ids_used_in_relations.insert((*it2)->primitive->getId());
					}
				}
				else if ((*it2)->role == "inner") {
					const Way *way = dynamic_cast<const Way*>((*it2)->primitive);
					if (way == NULL) cerr << "Primitive with id " << (*it2)->primitive->getId() << " has role=inner and isn't way, ignoring." << endl;
					else multipolygon->addHole(way);
				}
			}

			if (!multipolygon->hasAnyOuterPart()) cerr << "Relation with id " << it->second->getId() << " hasn't any \"outer\" element, ignoring." << endl;
			else {
				multipolygon->setDone();
				if (multipolygon->isValid()) output->push_back(multipolygon);
			}
		}
	}
	for (unordered_map<uint64_t,Way*>::const_iterator it = this->ways.begin(); it != this->ways.end(); it++) {
		const char *value_now = it->second->getAttribute(key);
		if (value_now != NULL && (value == NULL || strcmp(value, value_now) == 0)) {
			const vector<const Relation*> *relations = it->second->getRelations();

			for (vector<const Relation*>::const_iterator it2 = relations->begin(); it2 != relations->end(); it2++) {
				const char *role = (*it2)->getRoleForId(it->second->getId());
				if (strcmp(role, "outer") == 0) {
					if (ids_used_in_relations.find(it->second->getId()) == ids_used_in_relations.end()) {		//isn't used in relation already
						MultiPolygon *multipolygon = new MultiPolygon(*it2);
						const vector<const PrimitiveRole*> *members = (*it2)->getRelationMembers();
						for (vector<const PrimitiveRole*>::const_iterator it3 = members->begin(); it3 != members->end(); it3++) {
							if ((*it3)->role == "outer") {		//exist more outer ways for this polygon
								const Way *way = dynamic_cast<const Way*>((*it3)->primitive);
								if (way == NULL) cerr << "Outer element other than way in relation " << (*it2)->getId() << ", ignoring." << endl;
								else if ((*it3)->primitive->getId() < it->second->getId()) {		//I make it only once; when processing way with lowest id
									delete multipolygon;
									goto NEXT_WAY;
								}
								else multipolygon->addOuterPart(way);
							}
							else if ((*it3)->role == "inner") {
								const Way *way = dynamic_cast<const Way*>((*it3)->primitive);
								if (way == NULL) cerr << "Inner element other than way in relation " << (*it2)->getId() << ", ignoring." << endl;
								else multipolygon->addHole(way);
							}
						}
						multipolygon->setDone();
						if (multipolygon->isValid()) output->push_back(multipolygon);
						goto NEXT_WAY;
					}
				}
/*				else if (strcmp(role, "inner") == 0) {
					//if exists some way with the same searched attributes and have "outer" role, ignore this "inner" way
					const vector<const PrimitiveRole*> *members = (*it2)->getRelationMembers();
					for (vector<const PrimitiveRole*>::const_iterator it3 = members->begin(); it3 != members->end(); it3++) {
						if ((*it3)->role == "outer") {
							if ((*it3)->primitive->hasAttribute(key, value)) goto NEXT_WAY;
						}
					}
				}
*/			}

			//isn't in any relation, so add as common way
			MultiPolygon *multipolygon = new MultiPolygon(NULL);
			multipolygon->addOuterPart(it->second);
			multipolygon->setDone();
			if (multipolygon->isValid()) output->push_back(multipolygon);
			else delete multipolygon;
		}
		NEXT_WAY:;
	}

	if (value != NULL) {
		unordered_map<string,size_t>::const_iterator it = this->disused_attributes.find(string(key)+"="+value);
		if (it != this->disused_attributes.end()) this->disused_attributes.erase(it);
	}
}

void Primitives::getDisusedAttributes(multimap<size_t,string> *output) const {
	for (unordered_map<string,size_t>::const_iterator it = this->disused_attributes.begin(); it != this->disused_attributes.end(); it++) {
		output->insert(make_pair(it->second, it->first));
	}
}

struct LoadXmlStruct {
	Primitives *primitives;
	Primitive *current_primitive;
};

void XmlStartElement(void *user_data, const char *name, const char **attributes) {
	LoadXmlStruct* data = static_cast<LoadXmlStruct*>(user_data);

	if (strcmp(name, "node") == 0) {
		uint64_t id;
		float lat, lon;
		bool id_set = false, lat_set = false, lon_set = false;

		for (size_t i = 0; attributes != NULL && attributes[i] != NULL; i += 2) {
			if (strcmp(attributes[i], "id") == 0) {
				id = atol(attributes[i+1]);
				id_set = true;
			}
			else if (strcmp(attributes[i], "lat") == 0) {
				lat = atof(attributes[i+1]);
				lat_set = true;
			}
			else if (strcmp(attributes[i], "lon") == 0) {
				lon = atof(attributes[i+1]);
				lon_set = true;
			}
		}

		if (id_set && lat_set && lon_set) {
			Node *node = new Node(id, lat, lon);
			if (data->current_primitive != NULL) cerr << "Node with id " << id << " is in other element!" << endl;
			data->current_primitive = node;
			data->primitives->addNode(id, node);
		}
		else cerr << "Found <node> without mandatory fields!" << endl;
	}
	else if (strcmp(name, "way") == 0) {
		uint64_t id;
		bool id_set = false;

		for (size_t i = 0; attributes != NULL && attributes[i] != NULL; i += 2) {
			if (strcmp(attributes[i], "id") == 0) {
				id = atol(attributes[i+1]);
				id_set = true;
			}
		}

		if (id_set) {
			Way *way = new Way(id);
			if (data->current_primitive != NULL) cerr << "Way with id " << id << " is in other element!" << endl;
			data->current_primitive = way;
			data->primitives->addWay(id, way);
		}
		else cerr << "Found <way> without mandatory fields!" << endl;
	}
	else if (strcmp(name, "relation") == 0) {
		uint64_t id;
		bool id_set = false;

		for (size_t i = 0; attributes != NULL && attributes[i] != NULL; i += 2) {
			if (strcmp(attributes[i], "id") == 0) {
				id = atol(attributes[i+1]);
				id_set = true;
			}
		}

		if (id_set) {
			Relation *relation = new Relation(id);
			if (data->current_primitive != NULL) cerr << "Relation with id " << id << " is in other element!" << endl;
			data->current_primitive = relation;
			data->primitives->addRelation(id, relation);
		}
		else cerr << "Found <relation> without mandatory fields!" << endl;
	}
	else if (strcmp(name, "tag") == 0) {
		if (data->current_primitive == NULL) {
			cerr << "Element <tag> outside tags!" << endl;
		}
		else {
			const char *key = NULL, *value = NULL;

			for (size_t i = 0; attributes != NULL && attributes[i] != NULL; i += 2) {
				if (strcmp(attributes[i], "k") == 0) {
					key = attributes[i+1];
				}
				else if (strcmp(attributes[i], "v") == 0) {
					value = attributes[i+1];
				}
			}

			if (key != NULL && value != NULL) {
				if (!data->primitives->isAttributeIgnored(key, value)) {
					if (!data->primitives->isAttributeLightlyIgnored(key, value))
						data->primitives->setExistingAttribute(key, value);
					data->current_primitive->setAttribute(key, value);
				}
			}
			else cerr << "Found <way> without mandatory fields!" << endl;
		}
	}
	else if (strcmp(name, "nd") == 0) {
		if (data->current_primitive == NULL || dynamic_cast<Way*>(data->current_primitive) == NULL) {
			cerr << "Element <nd> outside <way> tag!" << endl;
		}
		else {
			uint64_t id;
			bool id_set = false;

			for (size_t i = 0; attributes != NULL && attributes[i] != NULL; i += 2) {
				if (strcmp(attributes[i], "ref") == 0) {
					id = atol(attributes[i+1]);
					id_set = true;
				}
			}

			if (id_set) {
				const Node *node = data->primitives->getNode(id);
				if (node == NULL); //it's ok cerr << "Node with id " << id << " isn't defined before defining way." << endl;
				else dynamic_cast<Way*>(data->current_primitive)->addNodeToWay(node);
			}
			else cerr << "Found <nd> with no mandatory fields!" << endl;
		}
	}
	else if (strcmp(name, "member") == 0) {
		if (data->current_primitive == NULL || dynamic_cast<Relation*>(data->current_primitive) == NULL) {
			cerr << "Element <member> outside <relation> tag!" << endl;
		}
		else {
			bool is_way;
			uint64_t member_id;
			const char *role = NULL;
			bool is_way_set = false, member_id_set = false;

			for (size_t i = 0; attributes != NULL && attributes[i] != NULL; i += 2) {
				if (strcmp(attributes[i], "type") == 0) {
					if (strcmp(attributes[i+1], "node") == 0) is_way = false;
					else if (strcmp(attributes[i+1], "way") == 0) is_way = true;
					else if (strcmp(attributes[i+1], "relation") == 0) goto AFTER_MEMBER;	//do nothing with it
					else {
						cerr << "Unknown relation member type (" << attributes[i+1] << ")." << endl;
						continue;
					}
					is_way_set = true;
				}
				else if (strcmp(attributes[i], "ref") == 0) {
					member_id = atol(attributes[i+1]);
					member_id_set = true;
				}
				else if (strcmp(attributes[i], "role") == 0) {
					role = attributes[i+1];
				}
			}

			if (is_way_set && member_id_set && role != NULL) {
				Primitive *primitive;
				if (!is_way) primitive = data->primitives->getNode(member_id);
				else {
					primitive = data->primitives->getWay(member_id);
					if (primitive != NULL) dynamic_cast<Way*>(primitive)->addWayToRelation(dynamic_cast<Relation*>(data->current_primitive));
				}
				if (primitive != NULL) dynamic_cast<Relation*>(data->current_primitive)->addMemberToRelation(primitive, role);
			}
			else cerr << "Found <member> with no mandatory fields!" << endl;
		}
		AFTER_MEMBER:;
	}
	else if (strcmp(name, "bounds") == 0) {
		double minlat, minlon, maxlat, maxlon;
		bool minlat_set = false, minlon_set = false, maxlat_set = false, maxlon_set = false;

		for (size_t i = 0; attributes != NULL && attributes[i] != NULL; i += 2) {
			if (strcmp(attributes[i], "minlat") == 0) {
				minlat = atof(attributes[i+1]);
				minlat_set = true;
			}
			else if (strcmp(attributes[i], "minlon") == 0) {
				minlon = atof(attributes[i+1]);
				minlon_set = true;
			}
			else if (strcmp(attributes[i], "maxlat") == 0) {
				maxlat = atof(attributes[i+1]);
				maxlat_set = true;
			}
			else if (strcmp(attributes[i], "maxlon") == 0) {
				maxlon = atof(attributes[i+1]);
				maxlon_set = true;
			}
		}

		if (minlat_set && minlon_set && maxlat_set && maxlon_set) {
			if (!data->primitives->isBoundsSet()) {
				//TODO do something with scaling - now it's dimensed only for area in size of zoom 12
				//so, for now, we simulate that size (very stupid)
				minlat = (minlat + maxlat)/2 - 0.0569;
				maxlat = minlat + 2*0.0569;
				minlon = (minlon + maxlon)/2 - 0.0439;
				maxlon = minlon + 2*0.0439;

				data->primitives->setBounds(minlat, minlon, maxlat, maxlon);
			}
			else cerr << "Bounds is set more than once!" << endl;
		}
		else cerr << "Found <bounds> with no mandatory fields!" << endl;
	}
}

void XmlCharacterData(void *user_data, const XML_Char *buffer, int len) {
	for (int i = 0; i < len; i++) {
		if (buffer[i] > ' ' || buffer[i] < 0) {
			cerr << "Unknown character data in XML file: " << (int)buffer[i] << ", context: "<< string(buffer, len) << endl;
			break;
		}
	}
}

void XmlEndElement(void *user_data, const char *name) {
	if (strcmp(name, "node") == 0 || strcmp(name, "way") == 0 || strcmp(name, "relation") == 0) {
		LoadXmlStruct* data = static_cast<LoadXmlStruct*>(user_data);
		if (data->current_primitive == NULL) cerr << "Internal error in XML parser (closing of not-opened tag " << name << ")!" << endl;
		data->current_primitive = NULL;
	}
}

bool Primitives::loadFromXml(const char *filename) {
	FILE *fp = fopen(filename, "rb");
	if (fp == NULL) {
		cerr << "Cannot open file " << filename << "!" << endl;
		return false;
	}
	LoadXmlStruct load_xml_struct;
	load_xml_struct.primitives = this;
	load_xml_struct.current_primitive = NULL;

	XML_Parser parser = XML_ParserCreate(NULL);
	XML_SetUserData(parser, &load_xml_struct);
	XML_SetElementHandler(parser, XmlStartElement, XmlEndElement);
	XML_SetCharacterDataHandler(parser, XmlCharacterData);

	bool success = true;

	while (true) {
		char buffer[1000];
		size_t len = fread(buffer, 1, sizeof(buffer), fp);
		bool is_final = (len != sizeof(buffer));
		if (XML_Parse(parser, buffer, len, is_final) == 0) {
			cerr << "Error parsing file " << filename << " at line " << XML_GetCurrentLineNumber(parser) << ": " << XML_ErrorString(XML_GetErrorCode(parser));
			success = false;
			break;
		}
		if (is_final) break;
	}

	if (load_xml_struct.current_primitive != NULL) cerr << "Internal error in XML parser (not-closed tag)" << endl;

	XML_ParserFree(parser);
	fclose(fp);

	if (!this->isBoundsSet()) {			//bounds are not set, so get it from data
		this->minlat = 10000;			//some nonsens
		this->maxlat = -10000;
		this->minlon = 10000;
		this->maxlon = -10000;

		for (unordered_map<uint64_t,Node*>::const_iterator it = this->nodes.begin(); it != this->nodes.end(); it++) {
			const float lat = it->second->getLat(), lon = it->second->getLon();
			if (lat < this->minlat) this->minlat = lat;
			if (lat > this->maxlat) this->maxlat = lat;
			if (lon < this->minlon) this->minlon = lon;
			if (lon > this->maxlon) this->maxlon = lon;
		}

		if (this->minlat >= this->maxlat || this->minlon >= this->maxlon) {
			cerr << "Error while computing area bounds." << endl;
			return false;
		}

		//TODO do something with scaling - now it's dimensed only for area in size of zoom 12
		//so, for now, we simulate that size (very stupid)
		this->minlat = (this->minlat + this->maxlat)/2 - 0.0569;
		this->maxlat = this->minlat + 2*0.0569;
		this->minlon = (this->minlon + this->maxlon)/2 - 0.0439;
		this->maxlon = this->minlon + 2*0.0439;
	}

	cout << "Area: LAT " << this->minlat << " - " << this->maxlat << ", LON " << this->minlon << " - " << this->maxlon << endl;
	cout << "Nodes: " << this->nodes.size() << " Ways: " << this->ways.size() << " Relations: " << this->relations.size() << endl;

	return success;
}
