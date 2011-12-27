
#pragma once

#include <cmath>
#ifndef M_PI		//under Cygwin M_PI not found (??)
 #define M_PI 3.14159265358979323846
#endif

struct Rect {
	double minlat;
	double minlon;
	double maxlat;
	double maxlon;

	void enlargeByPercent(double percent) {		//enlarge region to all directions by x percent
		double lat_diff = this->maxlat-this->minlat;
		this->minlat -= lat_diff * percent/100;
		this->maxlat += lat_diff * percent/100;

		double lon_diff = this->maxlon-this->minlon;
		this->minlon -= lon_diff * percent/100;
		this->maxlon += lon_diff * percent/100;
	}
};

class Primitive {
	private:
	uint64_t id;
	unordered_map<string,string> tags;

	public:
	Primitive(uint64_t id) : id(id) { }
	virtual ~Primitive() { }
	uint64_t getId() const { return this->id; }
	const char *getAttribute(const char *key) const {
		unordered_map<string,string>::const_iterator it = this->tags.find(key);
		if (it == this->tags.end()) return NULL;
		else return it->second.c_str();
	}
	bool hasAttribute(const char *key, const char *value) const {
		unordered_map<string,string>::const_iterator it = this->tags.find(key);
		if (it == this->tags.end()) return false;
		if (value == NULL) return true;
		else return (strcmp(it->second.c_str(), value) == 0);
	}
	void setAttribute(const char *key, const char *value) {
		this->tags[key] = value;
	}
};

class Node : public Primitive {
	private:
	float lat;
	float lon;

	public:
	Node(uint64_t id, float lat, float lon) : Primitive(id), lat(lat), lon(lon) { }
	virtual ~Node() { }
	const float getLat() const { return this->lat; }
	const float getLon() const { return this->lon; }
};

class Relation;

class Way : public Primitive {
	private:
	vector<const Node*> nodes;
	vector<const Relation*> relations;

	public:
	Way(uint64_t id) : Primitive(id) { }
	virtual ~Way() { }
	void addNodeToWay(const Node *node) {
		this->nodes.push_back(node);
	}
	void addWayToRelation(const Relation *relation) {
		this->relations.push_back(relation);
	}
	const vector<const Node*> &getNodes() const {
		return this->nodes;
	}
	const vector<const Relation*> &getRelations() const {
		return this->relations;
	}
	uint64_t getFirstNodeId() const { return this->nodes.at(0)->getId(); }
	uint64_t getLastNodeId() const { return this->nodes.at(this->nodes.size()-1)->getId(); }
};

struct PrimitiveRole {
	const Primitive &primitive;
	string role;
	PrimitiveRole(const Primitive &primitive, string role) : primitive(primitive), role(role) { }
};

class Relation : public Primitive {
	private:
	vector<const PrimitiveRole*> members;

	public:
	Relation(uint64_t id) : Primitive(id) { }
	virtual ~Relation() {
		for (vector<const PrimitiveRole*>::iterator it = this->members.begin(); it != this->members.end(); it++) delete *it;
	}
	void addMemberToRelation(const Primitive &primitive, const char *role) {
		PrimitiveRole *primitive_role = new PrimitiveRole(primitive, role);
		this->members.push_back(primitive_role);
	}
	const char *getRoleForId(uint64_t id) const {
		for (vector<const PrimitiveRole*>::const_iterator it = this->members.begin(); it != this->members.end(); it++) {
			if (id == (*it)->primitive.getId()) return (*it)->role.c_str();
		}
		assert(false);		//not found
	}
	const vector<const PrimitiveRole*> &getRelationMembers() const {
		return this->members;
	}
};

class Primitives {
	private:
	bool bounds_set_by_x_y;
	bool bounds_set;
	Rect view_rect;
	Rect interest_rect;
	unordered_map<uint64_t,Node*> nodes;
	unordered_map<uint64_t,Way*> ways;
	unordered_map<uint64_t,Relation*> relations;
	unordered_map<string,const char*> ignored_attributes;
	unordered_map<string,const char*> lightly_ignored_attributes;
	unordered_map<string,size_t> disused_attributes;

	//copied from OpenStreetMap wiki
	double lon2tilex(double lon, int z) { return (((lon + 180.0) / 360.0 * pow(2.0, z))); }
	double lat2tiley(double lat, int z) { return ((1.0 - log(tan(lat * M_PI / 180.0) + 1.0 / cos(lat * M_PI / 180.0)) / M_PI) / 2.0 * pow(2.0, z));	}
	double tilex2lon(int x, int z)	{ return x / pow(2.0, z) * 360.0 - 180; }
	double tiley2lat(int y, int z) { double n = M_PI - 2.0 * M_PI * y / pow(2.0, z); return 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n))); }

	double halftiley2lat(int y, int z) { return tiley2lat(2*y, z); }

	double lon2relativex(double lon, int z) {
		double tile_coord = lon2tilex(lon, z);
		return (ceil(tile_coord) - tile_coord);
	}
	double lat2relativey(double lat, int z) {
		double tile_coord = lat2tiley(lat, z);
		return (ceil(tile_coord) - tile_coord);
	}

	void setInterestRectByViewRect();

	public:
	Primitives();
	~Primitives();
	void setBoundsByXY(size_t tile_x, size_t tile_y);
	void setIgnoredAttribute(const char *key, const char *value);
	bool isAttributeIgnored(const char *key, const char *value) const;
	void setLightlyIgnoredAttribute(const char *key, const char *value);
	bool isAttributeLightlyIgnored(const char *key, const char *value) const;
	void setExistingAttribute(const char *key, const char *value);
	bool loadFromXml(const char *filename);
	bool isBoundsSet() const { return (this->bounds_set || this->bounds_set_by_x_y); }
	Rect getViewRect() const { return this->view_rect; }
	void setBounds(double minlat, double minlon, double maxlat, double maxlon);
	void addNode(uint64_t id, Node *node) {
		this->nodes[id] = node;
	}
	void addWay(uint64_t id, Way *way) {
		this->ways[id] = way;
	}
	void addRelation(uint64_t id, Relation *relation) {
		this->relations[id] = relation;
	}
	Node *getNode(uint64_t id) const {
		unordered_map<uint64_t,Node*>::const_iterator it = this->nodes.find(id);
		if (it == this->nodes.end()) return NULL;
		else return it->second;
	}
	Way *getWay(uint64_t id) const {
		unordered_map<uint64_t,Way*>::const_iterator it = this->ways.find(id);
		if (it == this->ways.end()) return NULL;
		else return it->second;
	}
	void getNodesWithAttribute(list<const Node*> *output, const char *key, const char *value);
	void getWaysWithAttribute(list<const Way*> *output, const char *key, const char *value);
	void getMultiPolygonsWithAttribute(list<class MultiPolygon*> *output, const char *key, const char *value);
	void getDisusedAttributes(multimap<size_t,string> *output) const;
};

