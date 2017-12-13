#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
#include <math.h>
#include <algorithm>
#include <stdlib.h>

#include "routing-epz.cpp"

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

/* Adds noise to a Node's position */
Node jitter(Node node) {
    double offset = 0.00002;
    int latMult = (rand() % 5) - 2;
    int lonMult = (rand() % 5) - 2;
    double lat = node.lat + offset * latMult;
    double lon = node.lon + offset * lonMult;
    return Node(-1, lat, lon);
}

/* Moves a step from start to end and returns the new Node */
Node step(Node start, Node end) {
    double stepSize = 0.001;
    double lat = start.lat + (end.lat - start.lat) / 2;
    double lon = start.lon + (end.lon - start.lon) / 2;
    Node node(-1, lat, lon);
    return jitter(node);
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
    s << "<!DOCTYPE html>\n<html>\n\t<head>\n\t\t<meta name=\"viewport\" content=\"initial-scale=1.0, user-scalable=no\">\n\t\t<meta charset=\"utf-8\">\n\t\t<title>Simple Polylines</title>\n\t\t<style>\n\t\t\t#map {\n\t\t\t\theight: 100%;\n\t\t\t}\n\n\t\t\thtml, body {\n\t\t\t\theight: 100%;\n\t\t\t\tmargin: 0;\n\t\t\t\tpadding: 0;\n\t\t\t}\n\t\t</style>\n\t</head>\n\t<body>\n\t\t<div id=\"map\"></div>\n\t\t<script>\n\t\t\tfunction getRandomColor() {\n\t\t\t\tvar letters = '0123456789ABCDEF';\n\t\t\t\tvar color = '#';\n\t\t\t\tfor (var i = 0; i < 6; i++) {\n\t\t\t\t\tcolor += letters[Math.floor(Math.random() * 16)];\n\t\t\t\t}\n\t\t\t\treturn color;\n\t\t\t}\n\n\t\t\tfunction initMap() {\n\t\t\t\tvar map = new google.maps.Map(document.getElementById('map'), {\n\t\t\t\t\tzoom: 15,\n\t\t\t\t\tcenter: {lat: " << lat << ", lng: " << lon << "},\n\t\t\t\t\tmapTypeId: 'terrain'\n\t\t\t\t});\n\n\t\t\t\tvar routes = [\n";
    return s.str();
}

string html_suffix() {
    return "\t\t\t}\n\t\t</script>\n\n\t\t<script async defer\n\t\t\tsrc=\"https://maps.googleapis.com/maps/api/js?key=AIzaSyBSCP1vdj1J3Nm4ta-KX47aik0XbZZbQ0U&callback=initMap\">\n\t\t</script>\n\t</body>\n</html>\n";
}

string html_route(const Route & route) {
    stringstream s;
    s.precision(10);
    s << "\t\t\t\t\t[";

    for (int i = 0; i < route.size(); i++) {
        s << "{lat: " << route[i].lat << ", lng: " << route[i].lon << "}";
        if (i != route.size()-1) {s << ", ";}
    }

    s << "]";

    return s.str();
}

string html_route(const vector<uint64_t> & ids, unordered_map<uint64_t, Node*> & nodes) {
    stringstream s;
    s.precision(10);
    s << "\t\t\t\t\t[";

    for (int i = 0; i < ids.size(); i++) {
        s << "{lat: " << nodes[ids[i]]->lat << ", lng: " << nodes[ids[i]]->lon << "}";
        if (i != ids.size()-1) {s << ", ";}
    }

    s << "]";

    return s.str();
}

string html_human_route(const vector<uint64_t> & ids, unordered_map<uint64_t, Node*> & nodes) {
    vector<Node*> coords;
    vector<Node> route;
    Node start, cur, end;
    double threshold = 0.01;

    for (int i = 0; i < ids.size(); i++) {
        coords.push_back(nodes[ids[i]]);
    }
    
    route.push_back(*coords[0]);

    for (int i = 0; i < coords.size()-1; i++) {
        start = *coords[i];
        end = *coords[i+1];
        cur = start;

        //while (Distance(cur, end) > threshold) {
            cur = step(cur, end);
            route.push_back(cur);
        //}

        route.push_back(jitter(end));
    }


    stringstream s;
    s.precision(10);
    s << "\t\t\t\t\t[";

    for (int i = 0; i < route.size(); i++) {
        s << "{lat: " << route[i].lat << ", lng: " << route[i].lon << "}";
        if (i != route.size()-1) {s << ", ";}
    }

    s << "]";

    return s.str();
}

string html_display_route() {
    stringstream s;
    s.precision(10);
    
    s << "\t\t\t\tvar highlightedPath;\n";
    s << "\t\t\t\tvar oldColor;\n\n";

    s << "\t\t\t\tfor (var i = 0; i < routes.length; i++) {\n";
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
    s << "\t\t\t\t\t\t// this.setOptions({zIndex: 2});\n";
    s << "\t\t\t\t\t\thighlightedPath = this;\n";
    s << "\t\t\t\t\t});\n";

    s << "\t\t\t\t}\n\n";

    return s.str();
}

string html_origin_marker(double lat, double lon) {
    stringstream s;
    s.precision(10);
    s << "\t\t\t\tvar marker = new google.maps.Marker({\n";
    s << "\t\t\t\t\tposition: {lat: " << lat << ", lng: " << lon << "},\n";
    s << "\t\t\t\t\tmap: map\n";
    s << "\t\t\t\t});\n\n";
    return s.str();
}

string html_epz_circle(double lat, double lon, double r) {
    stringstream s;
    s.precision(10);
    s << "\t\t\t\tvar circle = new google.maps.Circle({\n";
    s << "\t\t\t\t\tstrokeColor: '#FF0000',\n";
    s << "\t\t\t\t\tstrokeOpacity: 0.25,\n";
    s << "\t\t\t\t\tstrokeWeight: 1,\n";
    s << "\t\t\t\t\tfillColor: '#FF0000',\n";
    s << "\t\t\t\t\tfillOpacity: 0.15,\n";
    s << "\t\t\t\t\tmap: map,\n";
    s << "\t\t\t\t\tcenter: {lat: " << lat << ", lng: " << lon << "},\n";
    s << "\t\t\t\t\tradius: " << r << "\n";
    s << "\t\t\t\t});\n\n";
    return s.str();
}

/*
 * ROUTING FUNCTIONS
 */

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

void findRoute(uint64_t cur_node, double distance, unordered_map<uint64_t, Node*> & nodes, vector<uint64_t> curPath, vector<vector<uint64_t>> & results) {
    curPath.push_back(cur_node);

    if (distance > 0) {
        vector<Edge*> edges = nodes[cur_node]->edges;

        for (Edge* edge : edges) {
            if (!edge->used) {
                Node* otherNode = edge->otherEnd(cur_node);
                edge->used = true;
                findRoute(otherNode->id, distance - edge->distance, nodes, curPath, results);
                edge->used = false;
            }
        }
    } else {
        results.push_back(curPath);
    }
}

/*
 * OSRM Function to get NodeIds for a given route
 */
vector<uint64_t> osrmExtractRoute(string routeData) {

    vector<uint64_t> nodeIds;
    
    string filename_osrm = "../../maps/bike/chambana/chambana.osrm";

    // Configure based on a .osrm base path, and no datasets in shared mem from osrm-datastore
    EngineConfig config;
    config.storage_config = {filename_osrm};
    config.use_shared_memory = false;

    // Routing machine with several services (such as Route, Table, Nearest, Trip, Match)
    const OSRM osrm{config};

    // json object to store the result of the routing request
    json::Object result;

    // Create a RouteParameters object with Node Annotations
    RouteParameters params = createRouteParameters();

    // Parse the data of a single route and input coordinates into the RouteParameters project
    extractRouteDataIntoParams(routeData, params);

    // Execute routing request, this does the heavy lifting
    const auto status = osrm.Route(params, result);

    if (status == Status::Ok) {
        auto &routes = result.values["routes"].get<json::Array>();
        auto &route = routes.values.at(0).get<json::Object>();
        auto &legs = route.values["legs"].get<json::Array>();
            
        for(auto leg : legs.values) {
            json::Object legObject = leg.get<json::Object>();
            json::Object annotations = legObject.values["annotation"].get<json::Object>();
            json::Array nodes = annotations.values["nodes"].get<json::Array>();
            
            for (int i = 0; i < nodes.values.size(); i++) {
                uint64_t id = nodes.values[i].get<json::Number>().value;
                nodeIds.push_back(id);
            }
        }
    } else if (status == Status::Error) {
        printFailure(result);
    }

    //cout << nodeIds << "\n";
    return nodeIds;
}

/*
 * MAIN
 */

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

    /*
     * Open and read in Nodes file
     */

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

    /*
     * Open and read in Heatmap file
     */

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
  
    /*
     * Sort the edges vector inside every node (might be unnecessary)
     */
    for (auto entry : nodes) {sortEdges(entry.second->edges);}

    /*
     * Test routing code using a dummy start point
     */

    uint64_t start_node = 38009912;
    double distance = 0.625;
    Edge* start_edge = nodes[start_node]->edges[0];
    //start_edge->used = true;
    vector<uint64_t> curPath;
    vector<vector<uint64_t>> results;

    findRoute(start_node, distance, nodes, curPath, results);

    /*
     * Write results to Output file
     */
    
    ofstream outfile(file_output);
    outfile.precision(10);
    double tmp_lat = nodes[results[0][0]]->lat;
    double tmp_lon = nodes[results[0][0]]->lon;

    outfile << html_prefix(tmp_lat, tmp_lon);

    //cout << results.size() << "\n";
    int maxWeightIndex = 0;
    uint64_t maxWeight = 0;

    for (int i = 0; i < results.size(); i++) {
        vector<Edge*> edgeVector = getEdgeVector(results[i], nodes);
        uint64_t totalWeight = weighEdgeVector(edgeVector);

        if (maxWeight < totalWeight) {maxWeight = totalWeight; maxWeightIndex = i;}
    }

    //cout << results[maxWeightIndex] << "\n";

    //outfile << html_route(results[maxWeightIndex], nodes) << ",\n";
    //outfile << html_route(results[0], nodes) << ",\n";
    //outfile << html_route(results[results.size()-1], nodes);

    //outfile << html_human_route(results[maxWeightIndex], nodes) << ",\n";
    //outfile << html_human_route(results[0], nodes) << ",\n";
    //outfile << html_human_route(results[results.size()-1], nodes);
   
    //outfile << "\n\t\t\t\t];\n\n";

    //outfile << html_display_route();
    //outfile << html_origin(tmp_lat, tmp_lon);
    //outfile << html_suffix();
    //outfile.close();

    string routeData = "[[40.112445 -88.220564] [40.112474 -88.220578] [40.112504 -88.220596] [40.112533 -88.2206] [40.112563 -88.220588] [40.112603 -88.220594] [40.112649 -88.220587] [40.112688 -88.220581] [40.112718 -88.220581] [40.112757 -88.220566] [40.112775 -88.220529] [40.11279 -88.220464] [40.112793 -88.220418] [40.112795 -88.220361] [40.112795 -88.220306] [40.112795 -88.220253] [40.112795 -88.220196] [40.112797 -88.220128] [40.112798 -88.220079] [40.112807 -88.220035] [40.112815 -88.22] [40.112819 -88.219954] [40.11282 -88.219906] [40.112822 -88.219867] [40.112809 -88.21983] [40.112826 -88.21978] [40.112823 -88.219727] [40.112821 -88.219676] [40.112821 -88.219619] [40.11282 -88.219567] [40.112819 -88.219516] [40.112817 -88.219445] [40.112808 -88.219386] [40.112793 -88.21933] [40.112788 -88.219279] [40.112789 -88.219219] [40.112783 -88.219164] [40.112789 -88.219121] [40.112801 -88.219074] [40.112815 -88.21904] [40.112819 -88.218982] [40.112821 -88.218917] [40.112826 -88.218858] [40.112834 -88.218801] [40.112837 -88.218721] [40.112834 -88.218641] [40.112842 -88.218559] [40.112846 -88.218484] [40.112847 -88.218414] [40.112843 -88.218337] [40.112843 -88.218263] [40.112838 -88.21819] [40.112833 -88.218151] [40.112833 -88.218086] [40.112825 -88.218023] [40.112818 -88.217938] [40.112813 -88.217884] [40.112815 -88.217828] [40.11282 -88.217783] [40.112825 -88.217728] [40.112835 -88.217654] [40.112845 -88.217586] [40.112848 -88.217508] [40.112847 -88.217444] [40.112851 -88.217373] [40.112861 -88.217288] [40.112868 -88.217205] [40.112869 -88.217139] [40.112864 -88.21708] [40.112869 -88.217007] [40.112875 -88.216944] [40.112878 -88.216875] [40.112881 -88.216809] [40.11288 -88.216724] [40.11288 -88.216648] [40.112872 -88.216552] [40.112854 -88.216483] [40.112844 -88.216392] [40.11281 -88.216274] [40.112787 -88.216177] [40.112777 -88.216095] [40.112754 -88.216005] [40.112733 -88.215936] [40.11272 -88.215865] [40.112694 -88.215788] [40.112649 -88.21571] [40.112623 -88.215644] [40.112599 -88.215583] [40.112579 -88.21552] [40.112566 -88.215465] [40.11254 -88.215414] [40.112517 -88.215349] [40.112507 -88.21529] [40.112493 -88.215231] [40.112481 -88.215178] [40.112468 -88.215114] [40.112456 -88.215053] [40.112444 -88.21501] [40.112432 -88.214939] [40.112421 -88.214877] [40.11241 -88.214795] [40.112399 -88.214706] [40.112393 -88.214629] [40.112386 -88.214563] [40.112375 -88.214466] [40.112359 -88.214343] [40.112364 -88.214265] [40.112374 -88.214177] [40.112374 -88.214123] [40.112383 -88.214058] [40.112382 -88.214009] [40.112375 -88.213909] [40.112374 -88.213851] [40.112369 -88.213779] [40.112371 -88.213698] [40.112376 -88.213628] [40.112383 -88.213569] [40.112389 -88.213497] [40.112385 -88.213429] [40.112387 -88.213355] [40.112391 -88.213297] [40.112394 -88.213219] [40.112397 -88.213144] [40.112395 -88.213065] [40.112393 -88.213007] [40.112397 -88.212962] [40.112399 -88.212897] [40.112398 -88.212829] [40.112395 -88.212764] [40.112391 -88.212689] [40.112382 -88.212623] [40.11238 -88.21256] [40.112376 -88.21249] [40.112369 -88.212437] [40.112363 -88.212375] [40.112363 -88.212318] [40.112369 -88.212265] [40.112376 -88.212208] [40.112372 -88.212149] [40.112363 -88.212064] [40.112353 -88.211991] [40.112345 -88.211918] [40.11234 -88.211858] [40.112341 -88.211813] [40.112349 -88.211748] [40.112359 -88.211673] [40.112367 -88.211609] [40.112372 -88.211549] [40.112377 -88.211511] [40.112386 -88.211453] [40.112391 -88.211394] [40.112397 -88.211337] [40.112398 -88.211269] [40.112391 -88.211192] [40.112391 -88.211121] [40.112396 -88.211054] [40.112399 -88.21099] [40.1124 -88.210925] [40.112407 -88.210857] [40.112395 -88.210786] [40.112388 -88.210713] [40.112388 -88.210647] [40.112383 -88.210585] [40.11239 -88.210517] [40.112392 -88.210448] [40.1124 -88.21036] [40.112404 -88.2103] [40.112413 -88.210241] [40.112417 -88.210184] [40.112428 -88.210129] [40.112432 -88.210091] [40.112434 -88.210027] [40.112442 -88.20995] [40.112459 -88.209893] [40.112471 -88.20985] [40.112479 -88.209804] [40.11249 -88.209764] [40.112481 -88.209688] [40.112474 -88.20965] [40.112475 -88.209591] [40.112479 -88.209528] [40.11248 -88.209444] [40.112485 -88.209403] [40.112499 -88.209372] [40.11251 -88.209335] [40.112482 -88.209326] [40.11249 -88.209287] [40.112487 -88.209245] [40.112488 -88.209192] [40.11249 -88.20914] [40.112491 -88.209089] [40.112495 -88.209039] [40.112495 -88.20898] [40.112493 -88.208934] [40.112504 -88.208884] [40.112516 -88.208829] [40.112509 -88.208749] [40.112505 -88.208695] [40.112498 -88.208643] [40.112503 -88.208579] [40.112498 -88.20851] [40.112483 -88.208442] [40.112485 -88.208386] [40.112488 -88.20834] [40.112485 -88.208276] [40.112488 -88.208227] [40.112491 -88.208167] [40.112497 -88.208125] [40.112502 -88.208035] [40.112521 -88.207953] [40.112526 -88.207851] [40.112532 -88.207775] [40.112536 -88.207724] [40.11254 -88.207674] [40.112564 -88.207596] [40.112603 -88.20754] [40.112651 -88.207487] [40.112702 -88.207463] [40.112754 -88.207454] [40.112814 -88.207463] [40.112867 -88.207471] [40.112912 -88.207475] [40.112954 -88.207492] [40.113002 -88.207502] [40.113061 -88.207467] [40.113135 -88.207424] [40.113195 -88.207413] [40.113241 -88.207411] [40.113306 -88.207413] [40.113365 -88.20741] [40.113438 -88.207405] [40.113494 -88.207405] [40.113548 -88.207405] [40.113601 -88.20741] [40.113661 -88.207412] [40.113726 -88.207415] [40.11379 -88.20741] [40.113853 -88.207404] [40.11392 -88.207403] [40.113994 -88.207404] [40.114066 -88.207403] [40.114135 -88.207401] [40.114197 -88.207397] [40.114265 -88.207392] [40.114335 -88.207399] [40.114413 -88.207405] [40.114494 -88.207407] [40.114574 -88.207413] [40.114654 -88.207418] [40.114728 -88.207425] [40.114797 -88.207431] [40.114864 -88.207438] [40.114933 -88.207439] [40.115 -88.207441] [40.115067 -88.207438] [40.115141 -88.207435] [40.115212 -88.207432] [40.115283 -88.207429] [40.11535 -88.207427] [40.115421 -88.207423] [40.115483 -88.207421] [40.115546 -88.207421] [40.115616 -88.207421] [40.115689 -88.207426] [40.115764 -88.20743] [40.115838 -88.207435] [40.115908 -88.207441] [40.115969 -88.207439] [40.116023 -88.207442] [40.116067 -88.207448] [40.116102 -88.207447] [40.11613 -88.207444] [40.116172 -88.207436] [40.11621 -88.207435] [40.116239 -88.207436] [40.116279 -88.207432] [40.116324 -88.207423] [40.116374 -88.20742] [40.116431 -88.207424] [40.116488 -88.207429] [40.116545 -88.207429] [40.116613 -88.207431] [40.116686 -88.207431] [40.116763 -88.207432] [40.116844 -88.207433] [40.116923 -88.207439] [40.116994 -88.20744] [40.117062 -88.207451] [40.117128 -88.207466] [40.117192 -88.207472] [40.117255 -88.207471] [40.117319 -88.207469] [40.117377 -88.207465] [40.117431 -88.20747] [40.117483 -88.207474] [40.117536 -88.207479] [40.117593 -88.20748] [40.117648 -88.207476] [40.117704 -88.207478] [40.117762 -88.207478] [40.117824 -88.20748] [40.117885 -88.207484] [40.117945 -88.207489] [40.118006 -88.207493] [40.118063 -88.207497] [40.118121 -88.207501] [40.118177 -88.207504] [40.11823 -88.207504] [40.118281 -88.207508] [40.118325 -88.207515] [40.118376 -88.20752] [40.118422 -88.207522] [40.118457 -88.207509] [40.118497 -88.207503] [40.118552 -88.207506] [40.118607 -88.207512] [40.118657 -88.207519] [40.118709 -88.207525] [40.118769 -88.207531] [40.118833 -88.207538] [40.118893 -88.207542] [40.118953 -88.207543] [40.119014 -88.20754] [40.119076 -88.207542] [40.119141 -88.207545] [40.119209 -88.207545] [40.119271 -88.207544] [40.119325 -88.207543] [40.119384 -88.207541] [40.119441 -88.20754] [40.119499 -88.207545] [40.119562 -88.207548] [40.11962 -88.20755] [40.119679 -88.207552] [40.119739 -88.207554] [40.119797 -88.207559] [40.119856 -88.20756] [40.119914 -88.207562] [40.119974 -88.207565] [40.120029 -88.207568] [40.120079 -88.207569] [40.120129 -88.207569] [40.120181 -88.207572] [40.120232 -88.207575] [40.120288 -88.207576] [40.120342 -88.207573] [40.120397 -88.20757] [40.120442 -88.207569] [40.120494 -88.207572] [40.120536 -88.207573] [40.120576 -88.207574] [40.120618 -88.207573] [40.120659 -88.207575] [40.120693 -88.207573] [40.120727 -88.20757] [40.120764 -88.207569] [40.120798 -88.207571] [40.120828 -88.207579] [40.120862 -88.207587] [40.120904 -88.207582] [40.120944 -88.207573] [40.120977 -88.207569] [40.12101 -88.20757] [40.121048 -88.207574] [40.121085 -88.207576] [40.121119 -88.207575] [40.121149 -88.207576] [40.121179 -88.207584] [40.121218 -88.207593] [40.121252 -88.207599] [40.121286 -88.207597] [40.12132 -88.207599] [40.121353 -88.207596] [40.121391 -88.207596] [40.121428 -88.207601] [40.12147 -88.207603] [40.121515 -88.207598] [40.121558 -88.207593] [40.121591 -88.207593] [40.121627 -88.207592] [40.12166 -88.207589] [40.121701 -88.207586] [40.121743 -88.207584] [40.121771 -88.207589] [40.121811 -88.207593] [40.121861 -88.207592] [40.121902 -88.207574] [40.121925 -88.207526] [40.121934 -88.20749] [40.121931 -88.207452] [40.121928 -88.207408] [40.121929 -88.207363] [40.121934 -88.207304] [40.121934 -88.207254] [40.121937 -88.207213] [40.121941 -88.207164] [40.121944 -88.207101] [40.121951 -88.207049] [40.12195 -88.206999] [40.121952 -88.206947] [40.121953 -88.206903] [40.121959 -88.206857] [40.121962 -88.206814] [40.121968 -88.206773] [40.121975 -88.206718] [40.121976 -88.20667] [40.121976 -88.206631] [40.121982 -88.206561] [40.121986 -88.206512] [40.121991 -88.206461] [40.121992 -88.206396] [40.121988 -88.206336] [40.121989 -88.206285] [40.121992 -88.206234] [40.121988 -88.206172] [40.121986 -88.206097] [40.121989 -88.20604] [40.121988 -88.205988] [40.121988 -88.205935] [40.121987 -88.205883] [40.121994 -88.205824] [40.121999 -88.205776] [40.121998 -88.205723] [40.122001 -88.205673] [40.122002 -88.205635] [40.122005 -88.205575] [40.122005 -88.205535] [40.122004 -88.205497] [40.122007 -88.205446] [40.12201 -88.205382] [40.122013 -88.205324] [40.122017 -88.205269] [40.122019 -88.205226] [40.122018 -88.205188] [40.122022 -88.20514] [40.122029 -88.205097] [40.12203 -88.205049] [40.122029 -88.204997] [40.122035 -88.204926] [40.122041 -88.204891] [40.122052 -88.204826] [40.122049 -88.20479] [40.122053 -88.204726] [40.122048 -88.204688] [40.122056 -88.204651] [40.122058 -88.204612] [40.12206 -88.204567] [40.122061 -88.204521] [40.122062 -88.204477] [40.122065 -88.204435] [40.122068 -88.204389] [40.122068 -88.204326] [40.122065 -88.204269] [40.122064 -88.20422] [40.122061 -88.204162] [40.122064 -88.204111] [40.122063 -88.204056] [40.122067 -88.204002] [40.122065 -88.203939] [40.12206 -88.203874] [40.122065 -88.203812] [40.122063 -88.203753] [40.122063 -88.203692] [40.122065 -88.203626] [40.122068 -88.203564] [40.12207 -88.20351] [40.122071 -88.203455] [40.122072 -88.203392] [40.122074 -88.203325] [40.122076 -88.203263] [40.122078 -88.203183] [40.122079 -88.203111] [40.122076 -88.203039] [40.122076 -88.202976] [40.122074 -88.202913] [40.122075 -88.202851] [40.122077 -88.202785] [40.122074 -88.202706] [40.122072 -88.202632] [40.122077 -88.202557] [40.122081 -88.202483] [40.122087 -88.202425] [40.122087 -88.202374] [40.122087 -88.202327] [40.122082 -88.202271] [40.122088 -88.202233] [40.12209 -88.202184] [40.122098 -88.20211] [40.122126 -88.20203] [40.122175 -88.201969] [40.122222 -88.201934] [40.122265 -88.201908] [40.122317 -88.201876] [40.122365 -88.201858] [40.122422 -88.201836] [40.122471 -88.201804] [40.122519 -88.201772] [40.122569 -88.201748] [40.122617 -88.201723] [40.122663 -88.201679] [40.122714 -88.201639] [40.122773 -88.201592] [40.122833 -88.201554] [40.122898 -88.201517] [40.122959 -88.201483] [40.123018 -88.201447] [40.123069 -88.201419] [40.123121 -88.201388] [40.123173 -88.201358] [40.12322 -88.201331] [40.123268 -88.201306] [40.123315 -88.201278] [40.123369 -88.20125] [40.123424 -88.201221] [40.123482 -88.201192] [40.123539 -88.201167] [40.123596 -88.201146] [40.123653 -88.201123] [40.123707 -88.201104] [40.123757 -88.201089] [40.123808 -88.201076] [40.123855 -88.201053] [40.123896 -88.201023] [40.123935 -88.200991] [40.123977 -88.200964] [40.124016 -88.200947] [40.124057 -88.20093] [40.124102 -88.200912] [40.124144 -88.200893] [40.124187 -88.20087] [40.124229 -88.200852] [40.12427 -88.200827] [40.124313 -88.2008] [40.124356 -88.20078] [40.124392 -88.200761] [40.124429 -88.200739] [40.124471 -88.200719] [40.124512 -88.200698] [40.124546 -88.200683] [40.124584 -88.200665] [40.124627 -88.200649] [40.124665 -88.200621] [40.124705 -88.200592] [40.124742 -88.200569] [40.124789 -88.200553] [40.124834 -88.200535] [40.124878 -88.200511] [40.124917 -88.200497] [40.124955 -88.200485] [40.124993 -88.200466] [40.125027 -88.20045] [40.125054 -88.20044] [40.125112 -88.200425] [40.125148 -88.200409] [40.125187 -88.200392] [40.125223 -88.200381] [40.125259 -88.200368] [40.125297 -88.200348] [40.125334 -88.200337] [40.125365 -88.200332] [40.125394 -88.200327] [40.125426 -88.20031] [40.125459 -88.200295] [40.125496 -88.200282] [40.125538 -88.200264] [40.12557 -88.200245] [40.12561 -88.200235] [40.125646 -88.200226] [40.125682 -88.200206] [40.125713 -88.200191] [40.125747 -88.200182] [40.125781 -88.200173] [40.125815 -88.200168] [40.125854 -88.200164] [40.125889 -88.200151] [40.125924 -88.200134] [40.125959 -88.200115] [40.12599 -88.200104] [40.126043 -88.200086] [40.126075 -88.200081] [40.126107 -88.200075] [40.126139 -88.200056] [40.126182 -88.200017] [40.126202 -88.199991] [40.12623 -88.199997] [40.12628 -88.199993] [40.126313 -88.199985] [40.126333 -88.199956] [40.126365 -88.199932] [40.126402 -88.199919] [40.126432 -88.199908] [40.126461 -88.199895] [40.126488 -88.199848] [40.126486 -88.199809] [40.126459 -88.199782] [40.126436 -88.199758] [40.126401 -88.199739] [40.126362 -88.199703] [40.12634 -88.199657] [40.126321 -88.19961] [40.126288 -88.199558] [40.126244 -88.199497] [40.126198 -88.199456] [40.126156 -88.1994] [40.12613 -88.199344] [40.12611 -88.199286] [40.1261 -88.199234] [40.126115 -88.199179] [40.126123 -88.199122] [40.126106 -88.199093] [40.126097 -88.199055] [40.126097 -88.199009] [40.126102 -88.198969] [40.126084 -88.198934] [40.126053 -88.198912] [40.126018 -88.198911] [40.125989 -88.198924] [40.125964 -88.198948] [40.12595 -88.198981]]";













    Route route = getRouteFromString(routeData);
    vector<Route> routes;
    routes.push_back(route);
    Center home = findHomeLocation(routes, Radii[0]);
    cout << "\nHome = " << home << "\n";
    cout << "Route[0] = " << routes[0][0] << "\n"; 
    cout << "Route[end] = " << routes[0][route.size()-1] << "\n\n"; 

    vector<Route> prunedRoutes = pruneAndAddBoundaryPoints(routes, home);
    cout << "PrunedRoute[0] = " << prunedRoutes[0][0] << "\n"; 
    cout << "PrunedRoute[end] = " << prunedRoutes[0][route.size()-1] << "\n\n"; 

    vector<uint64_t> routeIds = osrmExtractRoute(routeData);
    
    outfile << html_route(routes[0]) << ",\n";
    outfile << html_route(prunedRoutes[0]);

    outfile << "\n\t\t\t\t];\n\n";

    outfile << html_display_route();
    outfile << html_origin_marker(home.p.lat, home.p.lon);
    outfile << html_epz_circle(home.p.lat, home.p.lon, home.r);
    outfile << html_suffix();
    outfile.close();

    return EXIT_SUCCESS;
}
