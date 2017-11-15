#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
#include <math.h>
#include <algorithm>
#include <stdlib.h>

using namespace std;

class Node;
class Edge;
double Distance(Node one, Node two);

class Node {
  public:
    uint64_t id;
    double lat, lon;
    vector<Edge*> edges;

    Node() {}
    Node(uint64_t _id, double _lat, double _lon) {id = _id; lat = _lat; lon = _lon;}
    Edge* findEdge(Node* other);
};

class Edge {
  public:
    Node* one;
    Node* two;
    uint64_t weight;
    double distance;
    bool used;

    Edge() {}
    Edge(Node* _one, Node* _two, uint64_t _weight) {
        one = _one->id < _two->id ? _one : _two;
        two = _one->id < _two->id ? _two : _one;
        weight = _weight;
        distance = Distance(*one, *two);
        used = false;
    }

    Node* otherEnd(uint64_t query) {return query == one->id ? two : one;}
};

/* Helper function to look for a particular edge */
Edge* Node::findEdge(Node* other) {
    for (Edge* edge : this->edges) {
        if (edge->one == this && edge->two == other) {return edge;}
        if (edge->one == other && edge->two == this) {return edge;}
    }
    return NULL;
}

/* Comparator function for sorting */
bool compare(Edge* one, Edge* two) {return one->weight >= two->weight;}

/* Sort the edges by weight */
void sortEdges(vector<Edge*> edges) {sort(edges.begin(), edges.end(), compare);}

/* Get vector of edges from vector of node ids */
vector<Edge*> getEdgeVector(const vector<uint64_t> & nodeIds, unordered_map<uint64_t, Node*> & nodes) {
    vector<Edge*> edgeVector;

    for (int i = 0; i < nodeIds.size()-1; i++) {
        uint64_t id = nodeIds[i];
        uint64_t otherId = nodeIds[i+1];
        Node* node = nodes[id];
        Node* other = nodes[otherId];
        Edge* edge = node->findEdge(other);
        if (edge == NULL) {cerr << "\nEDGE IS NULL, SHOULD NEVER HAPPEN: this=" << id << "  other=" << otherId << "\n"; continue;}
        edgeVector.push_back(edge);
    }

    return edgeVector;
}

/* Get total weight of edge vector */
uint64_t weighEdgeVector(const vector<Edge*> & edges) {
    uint64_t weight = 0;
    for (Edge* edge : edges) {weight += edge->weight;}
    return weight;
}

/*
 * OSTREAM OPERATORS
 */

ostream& operator<<(ostream & stream, const vector<Edge*> & edges) {
    for (Edge * edge : edges) {
        stream << edge->weight << " " << edge->one->id << " " << edge->two->id << " " << edge->distance << " " << edge->used << "\n";
    }
    return stream;
} 

ostream& operator<<(ostream & stream, const Node* node) {
    stream << node->id << "\n" << node->edges << "\n";
    return stream;
}

ostream& operator<<(ostream & stream, const vector<uint64_t> & v) {
    for (uint64_t i : v) {
        stream << i << " ";
    }
    return stream;
}

ostream& operator<<(ostream & stream, const vector<vector<uint64_t>> & v) {
    for (vector<uint64_t> v2 : v) {
        stream << v2 << "\n";
    }
    return stream;
}

/*
 * HTML GENERATION
 */

string html_prefix(double lat, double lon) {
    stringstream s;
    s.precision(10);
    s << "<!DOCTYPE html>\n<html>\n\t<head>\n\t\t<meta name=\"viewport\" content=\"initial-scale=1.0, user-scalable=no\">\n\t\t<meta charset=\"utf-8\">\n\t\t<title>Simple Polylines</title>\n\t\t<style>\n\t\t\t#map {\n\t\t\t\theight: 100%;\n\t\t\t}\n\n\t\t\thtml, body {\n\t\t\t\theight: 100%;\n\t\t\t\tmargin: 0;\n\t\t\t\tpadding: 0;\n\t\t\t}\n\t\t</style>\n\t</head>\n\t<body>\n\t\t<div id=\"map\"></div>\n\t\t<script>\n\t\t\tfunction getRandomColor() {\n\t\t\t\tvar letters = '0123456789ABCDEF';\n\t\t\t\tvar color = '#';\n\t\t\t\tfor (var i = 0; i < 6; i++) {\n\t\t\t\t\tcolor += letters[Math.floor(Math.random() * 16)];\n\t\t\t\t}\n\t\t\t\treturn color;\n\t\t\t}\n\n\t\t\tfunction initMap() {\n\t\t\t\tvar map = new google.maps.Map(document.getElementById('map'), {\n\t\t\t\t\tzoom: 11,\n\t\t\t\t\tcenter: {lat: " << lat << ", lng: " << lon << "},\n\t\t\t\t\tmapTypeId: 'terrain'\n\t\t\t\t});\n\n\t\t\t\tvar routes = [\n";
    return s.str();
}

string html_suffix() {
    return "\t\t\t}\n\t\t</script>\n\n\t\t<script async defer\n\t\t\tsrc=\"https://maps.googleapis.com/maps/api/js?key=AIzaSyBSCP1vdj1J3Nm4ta-KX47aik0XbZZbQ0U&callback=initMap\">\n\t\t</script>\n\t</body>\n</html>\n";
}

string html_route(const vector<uint64_t> & ids, unordered_map<uint64_t, Node*> & nodes, int i) {
    stringstream s;
    s.precision(10);
    s << "\t\t\t\t\t[";

    for (i = 0; i < ids.size(); i++) {
        s << "{lat: " << nodes[ids[i]]->lat << ", lng: " << nodes[ids[i]]->lon << "}";
        if (i != ids.size()-1) {s << ", ";}
    }

    s << "]";

    return s.str();
}

string randColor() {
    string colors = "0123456789ABCDEF";
    string s = "";
    for (int i = 0; i < 6; i++) {s += colors[rand() % 16];}
    return s;
}

string html_display_route(int max_i) {
    stringstream s;
    s.precision(10);
    
    s << "\t\t\t\tvar highlightedPath;\n";
    s << "\t\t\t\tvar oldColor;\n\n";

    s << "\t\t\t\tfor (var i = 0; i < " << max_i << "; i++) {\n";
    s << "\t\t\t\t\tvar Path = new google.maps.Polyline({\n";
    s << "\t\t\t\t\t\tpath: routes[i],\n";
    s << "\t\t\t\t\t\tgeodesic: true,\n";
    s << "\t\t\t\t\t\tstrokeColor: getRandomColor(),\n";
    s << "\t\t\t\t\t\tstrokeOpacity: 1.0,\n";
    s << "\t\t\t\t\t\tstrokeWeight: 3,\n";
    s << "\t\t\t\t\t\tzIndex: 1,\n";
    s << "\t\t\t\t\t\tmap: map\n";
    s << "\t\t\t\t\t});\n\n";

    s << "\t\t\t\t\tPath.addListener('mouseover', function(latlng) {\n";
    s << "\t\t\t\t\t\tif (highlightedPath != null) {\n";
    s << "\t\t\t\t\t\t\thighlightedPath.setOptions({strokeColor: oldColor});\n";
    s << "\t\t\t\t\t\t\thighlightedPath.setOptions({zIndex: 1});\n";
    s << "\t\t\t\t\t\t}\n";
    s << "\t\t\t\t\t\toldColor = this.strokeColor;\n";
    s << "\t\t\t\t\t\tthis.setOptions({strokeColor: '#FF0000'});\n";
    s << "\t\t\t\t\t\tthis.setOptions({zIndex: 2});\n";
    s << "\t\t\t\t\t\thighlightedPath = this;\n";
    s << "\t\t\t\t\t});\n";

    s << "\t\t\t\t}\n\n";

    return s.str();
}

string html_origin(double lat, double lon) {
    stringstream s;
    s.precision(10);
    s << "\t\t\t\tvar marker = new google.maps.Marker({\n";
    s << "\t\t\t\t\tposition: {lat: " << lat << ", lng: " << lon << "},\n";
    s << "\t\t\t\t\tmap: map\n";
    s << "\t\t\t\t});\n\n";
    return s.str();
}

/*
 *
 */

double radians(double in) {
    return in / 180.0 * M_PI;
}

double Distance(Node one, Node two) {
    double lat1 = one.lat;
    double lat2 = two.lat;
    double lon1 = one.lon;
    double lon2 = two.lon;

    double dlon = lon2 - lon1;
    double dlat = lat2 - lat1;
    double a = pow(sin(radians(dlat/2)), 2) + cos(radians(lat1)) * cos(radians(lat2)) * pow(sin(radians(dlon/2)), 2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    double d = 3961 * c;
    return d;
}

void Route(uint64_t cur_node, double distance, unordered_map<uint64_t, Node*> & nodes, vector<uint64_t> curPath, vector<vector<uint64_t>> & results) {
    curPath.push_back(cur_node);

    if (distance > 0) {
        vector<Edge*> edges = nodes[cur_node]->edges;

        for (Edge* edge : edges) {
            if (!edge->used) {
                Node* otherNode = edge->otherEnd(cur_node);
                edge->used = true;
                Route(otherNode->id, distance - edge->distance, nodes, curPath, results);
                edge->used = false;
            }
        }
    } else {
        results.push_back(curPath);
    }
}

int main(int argc, const char *argv[]) {
    if (argc < 4) {
        cerr << "\nUsage: " << argv[0] << " <in.heatmap> <in.nodes> <out.html>\n\n";
        return EXIT_FAILURE;
    }

    cout.precision(10);
    string line;
    string file_heatmap = argv[1];
    string file_nodes   = argv[2];
    string file_output  = argv[3];
    unordered_map<uint64_t, Node*> nodes;

    ifstream file(file_nodes);
    
    if (file.is_open()) {
        while (getline(file, line)) {
            uint64_t id;
            double lat, lon;
            stringstream str(line);
            str >> id >> lat >> lon;

            Node * node = new Node(id, lat, lon);
            nodes.insert(pair<uint64_t, Node*>(id, node));
        }
    } else {
        cout << "\nError: Couldn't open nodes file.\n\n";
        return EXIT_FAILURE; 
    }

    file.close();
    file = ifstream(file_heatmap);

    if (file.is_open()) {
        while (getline(file, line)) {
            uint64_t one, two;
            int weight;
            stringstream str(line);
            str >> one >> two >> weight;

            Node * n1 = nodes[one];
            Node * n2 = nodes[two];
            Edge * edge = new Edge(n1, n2, weight);
            n1->edges.push_back(edge);
            n2->edges.push_back(edge);
        }
    } else {
        cout << "\nError: Couldn't open heatmap file.\n\n";
        return EXIT_FAILURE; 
    }

    file.close();
  
    for (auto entry : nodes) {sortEdges(entry.second->edges);}

    uint64_t start_node = 38009912;
    double distance = 0.1;
    Edge* start_edge = nodes[start_node]->edges[0];
    //start_edge->used = true;
    vector<uint64_t> curPath;
    vector<vector<uint64_t>> results;

    Route(start_node, distance, nodes, curPath, results);
    //cout << nodes[38009912];

    ofstream outfile(file_output);
    outfile.precision(10);
    double tmp_lat = nodes[results[0][0]]->lat;
    double tmp_lon = nodes[results[0][0]]->lon;

    outfile << html_prefix(tmp_lat, tmp_lon);
    
    cout << results.size() << "\n";
    
    for (int i = 0; i < results.size(); i++) {
        vector<uint64_t> result = results[i];
        vector<Edge*> edgeVector = getEdgeVector(result, nodes);
        uint64_t totalWeight = weighEdgeVector(edgeVector);
        cout << totalWeight << " - " << result << "\n";
        outfile << html_route(result, nodes, i);
        if (i != results.size()-1) {
            outfile << ",\n";
        } else {
            outfile << "\n\t\t\t\t];\n\n";
        }
        //cout << edgeVector << "\n\n";
    }

    outfile << html_display_route(results.size());
    outfile << html_origin(tmp_lat, tmp_lon);
    outfile << html_suffix();

    return EXIT_SUCCESS;
}
