
#include "global.h"
#include "output_polygon.h"
#include "point_field.h"
#include "primitives.h"

Polygon3D::Polygon3D(uint64_t area_id, vector<double> &coords) {
	this->points_count = 0;
	this->is_valid = this->addPart(area_id, coords);
}

bool Polygon3D::addPart(uint64_t area_id, vector<double> &coords) {
	assert(coords.size()%3 == 0);

	stringstream str;
	str.precision(12);
	size_t area_points = 0;

	bool at_least_two_same_points = false;
	for (size_t i = 0; i < coords.size(); i += 3) {
		if (i > 0 && coords[i] == coords[i-3] && coords[i+1] == coords[i+1-3] && coords[i+2] == coords[i+2-3]) {
			at_least_two_same_points = true;
			continue;		//two same points
		}
		str << ",<" << coords[i] << "," << coords[i+1] << "," << coords[i+2] << ">";
		area_points++;
	}
	if (at_least_two_same_points) cerr << "Polygon with id " << area_id << " has at least two same points next to other." << endl;

	if (coords.size() >= 6 && (coords[0] != coords[coords.size()-3] || coords[1] != coords[coords.size()-2] || coords[2] != coords[coords.size()-1])) {
		str << ",<" << coords[0] << "," << coords[1] << "," << coords[2] << ">";
		area_points++;
		cerr << "Polygon with id " << area_id << " isn't closed, closing." << endl;
	}

	if (area_points < 4) {
		cerr << "Polygon with id " << area_id << " has only " << (area_points-1) << " different points, ignore it." << endl;
		return false;
	}

	this->points_count += area_points;
	if (this->coords_output != "") this->coords_output += str.str();
	else this->coords_output += str.str().substr(1);		//without coma at beginning
	return true;
}

void Polygon3D::addHole(uint64_t area_id, vector<double> &coords) {
	this->addPart(area_id, coords);
}

MultiPolygon::MultiPolygon(const Relation *relation) {
	this->is_done = false;
	this->is_valid = false;
	this->relation = relation;
}

MultiPolygon::~MultiPolygon() {
	for (list<vector<const XY*> >::const_iterator it = this->outer_parts.begin(); it != this->outer_parts.end(); it++) {
		for (vector<const XY*>::const_iterator it2 = it->begin(); it2 != it->end(); it2++) {
			delete *it2;
		}
	}
	for (list<vector<const XY*> >::const_iterator it = this->holes.begin(); it != this->holes.end(); it++) {
		for (vector<const XY*>::const_iterator it2 = it->begin(); it2 != it->end(); it2++) {
			delete *it2;
		}
	}
	for (vector<const Triangle*>::const_iterator it = this->triangles.begin(); it != this->triangles.end(); it++) {
		delete *it;
	}
}

double ComputeArea(const vector<const XY*> *polygon) {
	double result = 0.0f;

	for (size_t p = polygon->size() - 1, q = 0; q < polygon->size(); p = q++) {
		result += (*polygon)[p]->x * (*polygon)[q]->y - (*polygon)[q]->x * (*polygon)[p]->y;
	}

	return result * 0.5f;
}

bool AddPolygonToList(const list<const Way*> &ways, list<vector<const XY*> > *output_list) {
	assert(!ways.empty());

	list<const Way*> remaining_ways = ways;
	bool success_overall = false;

	while (!remaining_ways.empty()) {
		list<vector<const Node*> > outer_parts_now;

		{			//insert first way to outer parts
			const vector<const Node*> *nodes = (*remaining_ways.begin())->getNodes();
			outer_parts_now.push_back(vector<const Node*>());
			vector<const Node*> *outer_part = &(*outer_parts_now.rbegin());
			for (size_t i = 0; i < nodes->size(); i++)
				outer_part->push_back((*nodes)[i]);
			remaining_ways.erase(remaining_ways.begin());
		}

		NEW_WAY_PROBING:		//now try to join all possible ways with start way
		for (list<const Way*>::iterator it = remaining_ways.begin(); it != remaining_ways.end(); it++) {

				//may I join it with some existing way?
			for (list<vector<const Node*> >::iterator it2 = outer_parts_now.begin(); it2 != outer_parts_now.end(); it2++) {
				uint64_t last_id = (*it2->rbegin())->getId();
				const vector<const Node*> *nodes = (*it)->getNodes();

				if (last_id == (*nodes)[0]->getId()) {			//way continues on previous in normal order
					for (size_t i = 1; i < nodes->size(); i++)
						it2->push_back((*nodes)[i]);

					remaining_ways.erase(it);
					goto NEW_WAY_PROBING;
				}
				else if (last_id == (*nodes)[nodes->size()-1]->getId()) {			//the same loop in reverse order
					for (size_t i = nodes->size()-2; true; i--) {
						it2->push_back((*nodes)[i]);
						if (i == 0) break;
					}
					remaining_ways.erase(it);
					goto NEW_WAY_PROBING;
				}
			}
		}

		for (list<vector<const Node*> >::const_iterator it = outer_parts_now.begin(); it != outer_parts_now.end(); it++) {
			output_list->push_back(vector<const XY*>());
			vector<const XY*> *output_part = &(*output_list->rbegin());
			size_t count_of_two_same_points = 0;

			for (size_t i = 0; i < it->size(); i++) {
				const double x = (*it)[i]->getLon();
				const double y = (*it)[i]->getLat();

				if (i > 0 && x == (*it)[i-1]->getLon() && y == (*it)[i-1]->getLat()) {
					count_of_two_same_points++;		//two same points
				}
				else output_part->push_back(new XY(x, y));
			}

			if (count_of_two_same_points > 0) cerr << "Polygon (way or relation of ways) with way with id " << (*ways.begin())->getId() << " has " << count_of_two_same_points << " duplicites (two same nodes (or nodes with the same coords) next to other)." << endl;

			if (output_part->size() >= 2 &&
				((*output_part)[0]->x != (*output_part)[output_part->size()-1]->x || (*output_part)[0]->y != (*output_part)[output_part->size()-1]->y)) {

				output_part->push_back(new XY((*output_part)[0]->x, (*output_part)[0]->y));
				cerr << "Polygon (way or relation of ways) with way with id " << (*ways.begin())->getId() << " isn't closed, closing." << endl;
			}

			bool success = true;

			if (success && output_part->size() < 4) {
				cerr << "Polygon (way or relation of ways) with way with id " << (*ways.begin())->getId() << " has only " << (output_list->size()-1) << " different points, ignore it." << endl;
				success = false;
			}

			// Check if points are in clockwise order
			if (success && ComputeArea(output_part) > 0) {
				//it's very often cerr << "Polygon (way or relation of ways) with way with id " << (*ways.begin())->getId() << " is oriented counterclockwise, turn it clockwise." << endl;
				for (size_t i = 0; i < output_part->size()/2; i++) {
					const XY *tmp = (*output_part)[i];
					(*output_part)[i] = (*output_part)[output_part->size()-1-i];
					(*output_part)[output_part->size()-1-i] = tmp;
				}
			}

			if (success) success_overall = true;
			else {
				for (vector<const XY*>::const_iterator it = output_part->begin(); it != output_part->end(); it++) {
					delete *it;
				}
				output_list->pop_back();
			}
		}
	}

	return success_overall;
}

void MultiPolygon::addOuterPart(const Way *outer_part) {
	assert(!this->is_done);
	this->outer_ways.push_back(outer_part);
}

void MultiPolygon::addHole(const Way *hole) {
	assert(!this->is_done);

	list<const Way*> hole_list;
	hole_list.push_back(hole);
	AddPolygonToList(hole_list, &this->holes);
}

//all holes and outer parts are inserted to multipolygon. Now reconstruct whole outer way from its parts
void MultiPolygon::setDone() {
	assert(!this->is_done);
	assert(!this->outer_ways.empty());

	if (this->outer_ways.size() > 1) {			//if there more than 1 outer way, try to merge it together
		list<const Way*> remaining_ways = this->outer_ways;
		uint64_t first_id = (*remaining_ways.begin())->getFirstNodeId();
		uint64_t last_id = (*remaining_ways.begin())->getLastNodeId();
		list<const Way*> final_ways;
		final_ways.push_back(*remaining_ways.begin());
		remaining_ways.erase(remaining_ways.begin());

		while (!remaining_ways.empty()) {		//I have first way, now find remaining ones
			for (list<const Way*>::iterator it = remaining_ways.begin(); it != remaining_ways.end(); it++) {
				const uint64_t first_id_now = (*it)->getFirstNodeId();
				const uint64_t last_id_now = (*it)->getLastNodeId();

				if (first_id_now == last_id) {		//append to end, normal order
					last_id = last_id_now;
					final_ways.push_back(*it);
					remaining_ways.erase(it);
					goto WAY_FOUND;
				}
				if (last_id_now == last_id) {		//append to end, reverse order
					last_id = first_id_now;
					final_ways.push_back(*it);
					remaining_ways.erase(it);
					goto WAY_FOUND;
				}
				if (last_id_now == first_id) {		//append to start, normal order
					first_id = first_id_now;
					final_ways.push_front(*it);
					remaining_ways.erase(it);
					goto WAY_FOUND;
				}
				if (first_id_now == first_id) {		//append to start, reverse order
					first_id = last_id_now;
					final_ways.push_front(*it);
					remaining_ways.erase(it);
					goto WAY_FOUND;
				}
			}
			goto ERROR;

			WAY_FOUND:;
		}
		if (first_id != last_id) {
			ERROR:
			cerr << "Way in relation with start way with id " << (*this->outer_ways.begin())->getId() << " cannot be joined in all nodes; cannot join in nodes " << first_id << " and " << last_id << "." << endl;
		}

		this->is_valid = AddPolygonToList(final_ways, &this->outer_parts);
	}
	else {
		this->is_valid = AddPolygonToList(this->outer_ways, &this->outer_parts);
	}

	this->is_done = true;
}

const char *MultiPolygon::getAttribute(const char *key) const {
	assert(this->is_done);
	if (this->relation != NULL) {
		const char *value = this->relation->getAttribute(key);
		if (value != NULL) return value;
	}
	for (list<const Way*>::const_iterator it = this->outer_ways.begin(); it != this->outer_ways.end(); it++) {
		const char *value = (*it)->getAttribute(key);
		if (value != NULL) return value;
	}
	return NULL;
}

bool MultiPolygon::hasAttribute(const char *key, const char *value) const {
	assert(this->is_done);
	if (this->relation != NULL) {
		if (this->relation->hasAttribute(key, value)) return true;
	}
	for (list<const Way*>::const_iterator it = this->outer_ways.begin(); it != this->outer_ways.end(); it++) {
		if ((*it)->hasAttribute(key, value)) return true;
	}
	return false;
}

uint64_t MultiPolygon::getId() const {
	assert(this->is_done);
	return (*this->outer_ways.begin())->getId();
}

size_t MultiPolygon::getPointsCount() const {
	assert(this->is_done);
	assert(this->isValid());

	size_t count = 0;
	for (list<vector<const XY*> >::const_iterator it = this->outer_parts.begin(); it != this->outer_parts.end(); it++) {
		count += it->size();
	}
	for (list<vector<const XY*> >::const_iterator it = this->holes.begin(); it != this->holes.end(); it++) {
		count += it->size();
	}

	return count;
}

struct MultiPolygonPoint {
	XY *xy;
	size_t nodes_pos;
	size_t coords_pos;
};

bool isPointInTriangle(const XY *point, const XY *tr_point1, const XY *tr_point2, const XY *tr_point3) {
	// check bounds
	if (point->y > tr_point1->y && point->y > tr_point2->y && point->y > tr_point3->y) return false;
	if (point->y < tr_point1->y && point->y < tr_point2->y && point->y < tr_point3->y) return false;
	if (point->x > tr_point1->x && point->x > tr_point2->x && point->x > tr_point3->x) return false;
	if (point->x < tr_point1->x && point->x < tr_point2->x && point->x < tr_point3->x) return false;

	// check planes
	if ((point->x - tr_point1->x)*(tr_point2->y - tr_point1->y) <= (tr_point2->x - tr_point1->x)*(point->y - tr_point1->y)) return false;
	if ((point->x - tr_point2->x)*(tr_point3->y - tr_point2->y) <= (tr_point3->x - tr_point2->x)*(point->y - tr_point2->y)) return false;
	if ((point->x - tr_point3->x)*(tr_point1->y - tr_point3->y) <= (tr_point1->x - tr_point3->x)*(point->y - tr_point3->y)) return false;

	return true;
}

bool MultiPolygon::isPointInsidePolygon(const vector<const Triangle*> *triangles, const XY *point) const {
	for (vector<const Triangle*>::const_iterator it = triangles->begin(); it != triangles->end(); it++) {
		if (isPointInTriangle(point, (*it)->getXY(0), (*it)->getXY(1), (*it)->getXY(2))) {
			return true;
		}
	}
	return false;
}

struct Point {
	const XY *xy;
	size_t node_pos;
	size_t left_pos, right_pos;
	bool finished;
	bool is_in_hole;

	Point() : finished(false), is_in_hole(false) { }
	Point(const XY *xy, size_t node_pos, size_t left_pos, size_t right_pos, bool is_in_hole) :
		xy(xy), node_pos(node_pos), left_pos(left_pos), right_pos(right_pos), is_in_hole(is_in_hole) { }
};

void MultiPolygon::convertToTriangles(vector<const Triangle*> *triangles) const {
	// At first, I sort nodes by X coord
	Point *points;
	{
		size_t array_size = 0;
		for (list<vector<const XY*> >::const_iterator it = this->outer_parts.begin(); it != this->outer_parts.end(); it++) {
			array_size += it->size()-1;
		}
		for (list<vector<const XY*> >::const_iterator it = this->holes.begin(); it != this->holes.end(); it++) {
			array_size += it->size()-1;
		}
		points = new Point[array_size * 2];		// * 2 - reserve for adding nodes when simplify polygon
	}

	for (list<vector<const XY*> >::const_iterator outer_part_it = this->outer_parts.begin(); outer_part_it != this->outer_parts.end(); outer_part_it++) {
		size_t points_cnt;
		multimap<double,Point*> opened_points;

		{
			size_t i;
			for (i = 0; i < outer_part_it->size()-1; i++) {
				points[i].xy = (*outer_part_it)[i];
				points[i].node_pos = i;
				points[i].left_pos = (i == 0 ? outer_part_it->size()-2 : i-1);
				points[i].right_pos = (i == outer_part_it->size()-2 ? 0 : i+1);

				opened_points.insert(make_pair(points[i].xy->x, &points[i]));
			}

			for (list<vector<const XY*> >::const_iterator it = this->holes.begin(); it != this->holes.end(); it++) {
				for (size_t j = 0; j < it->size()-1; j++) {
					points[i+j].xy = (*it)[j];
					points[i+j].node_pos = i+j;
					points[i+j].left_pos = i+(j == 0 ? it->size()-2 : j-1);
					points[i+j].right_pos = i+(j == it->size()-2 ? 0 : j+1);
					points[i+j].is_in_hole = true;

					opened_points.insert(make_pair(points[i+j].xy->x, &points[i+j]));
				}
				i += it->size()-1;
			}
			points_cnt = i;
		}

		while (opened_points.begin() != opened_points.end()) {
			const Point *point = opened_points.begin()->second;
			if (point->finished) {
				opened_points.erase(opened_points.begin());
				continue;
			}

			// create triangle
			size_t start_pos_now = point->node_pos;
			size_t left_pos_now = point->left_pos;
			size_t right_pos_now = point->right_pos;

			const XY *node = point->xy;
			const XY *left_node = points[left_pos_now].xy;
			const XY *right_node = points[right_pos_now].xy;

			assert(!points[left_pos_now].finished);
			assert(!points[right_pos_now].finished);

			if (point->is_in_hole) {
				//it's ok; hole can be in other outer part  cerr << "Bad inner/outer relations in polygon with outer way " << this->getId() << endl;
				points[left_pos_now].right_pos = right_pos_now;
				points[right_pos_now].left_pos = left_pos_now;
				points[start_pos_now].finished = true;
				continue;
			}

			// check if some of other nodes isn't in this triangle
			NEW_TRIANGLE:
			for (multimap<double,Point*>::iterator it = opened_points.begin(); it != opened_points.end(); it++) {
				if (it->second->finished) continue;
				if (it->second->node_pos == start_pos_now) continue;
				if (it->second->node_pos == left_pos_now) continue;
				if (it->second->node_pos == right_pos_now) continue;

				if (isPointInTriangle(it->second->xy, left_node, node, right_node)) {
					right_pos_now = it->second->node_pos;
					right_node = points[right_pos_now].xy;
					goto NEW_TRIANGLE;
				}

				if (it->second->xy->x >= left_node->x && it->second->xy->x >= left_node->y)
					break;
			}

			// now we can 3 points to valid triangle

			//check if don't lie in line - if not, add to triangles
			if (left_node->x == node->x && right_node->x == node->x);
			else if ((left_node->y - node->y)/(left_node->x - node->x) == (right_node->y - node->y)/(right_node->x - node->x));
			else triangles->push_back(new Triangle(left_node, node, right_node));

			// now, remove triangle from polygon

			if (points[right_pos_now].left_pos != points[left_pos_now].right_pos
			 && points[right_pos_now].right_pos != point->left_pos) {

				if (points[right_pos_now].is_in_hole) {
					//one point is part of hole - so extend polygon way by hole way and remove hole

					//turn way in hole (legft is on right and right is on left)
					for (size_t pos = right_pos_now; true; ) {
						size_t next_pos = points[pos].right_pos;
						points[pos].right_pos = points[pos].left_pos;
						points[pos].left_pos = next_pos;
						points[pos].is_in_hole = false;
						if (next_pos == right_pos_now) break;
						pos = next_pos;
					}
				}

				//some point against; so polygon is now split to two polygons

				points[points_cnt].xy = points[right_pos_now].xy;
				points[points_cnt].node_pos = points_cnt;
				points[points_cnt].left_pos = points[right_pos_now].left_pos;
				points[points[right_pos_now].left_pos].right_pos = points_cnt;
				points[points_cnt].right_pos = start_pos_now;
				points[start_pos_now].left_pos = points_cnt;

				points[right_pos_now].left_pos = left_pos_now;
				points[left_pos_now].right_pos = right_pos_now;

				opened_points.insert(make_pair(points[points_cnt].xy->x, &points[points_cnt]));
				points_cnt++;
			}
			else if (points[right_pos_now].left_pos == points[left_pos_now].right_pos
			 && points[right_pos_now].right_pos == point->left_pos) {
				//this is last triangle in this (sub)polygon, erase points...
				points[start_pos_now].finished = true;
				points[left_pos_now].finished = true;
				points[right_pos_now].finished = true;
			}
			else {
					//remove start node
				if (point->left_pos == left_pos_now && point->right_pos == right_pos_now) {
					points[left_pos_now].right_pos = right_pos_now;
					points[right_pos_now].left_pos = left_pos_now;
					points[start_pos_now].finished = true;
				}	//remove left node
				else if (points[left_pos_now].left_pos == right_pos_now && points[left_pos_now].right_pos == start_pos_now) {
					points[right_pos_now].right_pos = start_pos_now;
					points[start_pos_now].left_pos = right_pos_now;
					points[left_pos_now].finished = true;
				}
				else assert(false);
			}
		}
	}

	delete[] points;
}

const vector<const Triangle*> *MultiPolygon::getPolygonTriangles() {
	if (this->triangles.empty()) {
		this->convertToTriangles(&this->triangles);
	}

	return &this->triangles;
}

void MultiPolygon::computeRegularInsidePoints(vector<PointFieldItem*> *objects, PointField *point_field, size_t tree_style_min, size_t tree_style_max) {
	assert(tree_style_min <= tree_style_max);

								//determining area of interest from outer polygon
	double minlat = 1000, minlon = 1000, maxlat = -1000, maxlon = -1000;
	for (list<vector<const XY*> >::const_iterator it = this->outer_parts.begin(); it != this->outer_parts.end(); it++) {
		for (vector<const XY*>::const_iterator it2 = it->begin(); it2 != it->end(); it2++) {
			if ((*it2)->x < minlon) minlon = (*it2)->x;
			if ((*it2)->x > maxlon) maxlon = (*it2)->x;
			if ((*it2)->y < minlat) minlat = (*it2)->y;
			if ((*it2)->y > maxlat) maxlat = (*it2)->y;
		}
	}

	const double TREES_PER_UNIT = 3000;

	size_t occuped_length = ceil((maxlon-minlon)*TREES_PER_UNIT + 1);
	double *occuped = new double[occuped_length+1];
	for (size_t i = 0; i <= occuped_length; i++) occuped[i] = 0;

	double y_step = (1 / TREES_PER_UNIT) / occuped_length;
	//cout << "Kroků: " << (maxlat-minlat)/y_step << endl;

	const vector<const Triangle*> *triangles = this->getPolygonTriangles();
	XY *xy = new XY();
	for (double current_y = minlat; current_y <= maxlat; current_y += y_step) {
		double relative = (rand() % (occuped_length*1000)) / 1000.0;
		size_t pos_x = static_cast<size_t>(floor(relative));
		double weight_right = relative - pos_x;
		double occuped_now = occuped[pos_x] * (1 - weight_right);
		occuped_now += occuped[pos_x+1] * weight_right;

		if (occuped_now < 0.3 && occuped_now * 1000 < rand()%1000) {
			xy->x = (relative / occuped_length) * (maxlon-minlon) + minlon;
			xy->y = current_y;
			if (!point_field->isPointNearOther(xy->x, xy->y)) {
				occuped[pos_x] += 1 - weight_right;
				occuped[pos_x+1] += weight_right;

				if (this->isPointInsidePolygon(triangles, xy)) {
					PointFieldItem *point = new PointFieldItem();
					point->item_type = (rand() % (tree_style_max - tree_style_min + 1)) + tree_style_min;
					point->xy = xy;
					objects->push_back(point);
					xy = new XY();
				}
			}
		}

		for (size_t i = 0; i <= occuped_length; i++) {
			if (occuped[i] > 0) {
				occuped[i] -= y_step * TREES_PER_UNIT;
				if (occuped[i] < 0) occuped[i] = 0;
			}
		}
	}

	delete xy;
	delete[] occuped;
}
