#include <fstream>
#include <string>
#include <sstream>
#include <unordered_set>
#include <boost/algorithm/string.hpp>

#include "osrm/match_parameters.hpp"
#include "osrm/nearest_parameters.hpp"
#include "osrm/route_parameters.hpp"
#include "osrm/table_parameters.hpp"
#include "osrm/trip_parameters.hpp"
#include "osrm/coordinate.hpp"
#include "osrm/engine_config.hpp"
#include "osrm/json_container.hpp"
#include "osrm/osrm.hpp"
#include "osrm/status.hpp"

using namespace std;
using namespace osrm;

/*
 * Return an ifstream object based on whether the input file lies in either one of two directories.
 */
ifstream openFile(string filename) {
    string dir1 = "/srv/data/strava-dataset/athlete-dataset-first/";
    string dir2 = "/srv/data/strava-dataset/athlete-dataset-second/";
    ifstream file(dir1 + filename + ".txt");

    if (!file.is_open()) {
        file = ifstream(dir2 + filename + ".txt");
    }
   
    return file;
}

/*
 * Prepare the input line for parsing
 */
stringstream prepareLineForParsing(string line) {
    boost::erase_all(line, "[");
    boost::erase_all(line, "]");
    boost::replace_all(line, ",", " ");
    return stringstream(line);
}

/*
 * Parse the data of a single route and input coordinates into the RouteParameters project
 */
void extractRouteDataIntoParams(string routeData, RouteParameters & params) {
    double lat, lon;
    int i = 0;
    stringstream str = prepareLineForParsing(routeData);
    
    while (str >> lat >> lon) {
        // Reduce route granularity by half in order to speed up performance
        if (i++ % 2 == 0) {
            params.coordinates.push_back({util::FloatLongitude{lon}, util::FloatLatitude{lat}});
        }   
    }
}

/*
 * Create a pair out of 2 longs, always ordering them in ascending order
 * This helps avoid duplicates such as <123, 456> and <456, 123>
 */
pair<uint64_t, uint64_t> makePair(uint64_t one, uint64_t two) {
    if (one < two) {return pair<uint64_t, uint64_t>(one, two);}
    else {return pair<uint64_t, uint64_t>(two, one);}
}

/*
 * Create a RouteParameters object with Node Annotations
 */
RouteParameters createRouteParameters() {
    RouteParameters params;
    params.annotations = true;
    params.annotations_type = RouteParameters::AnnotationsType::Nodes;
    return params;
}

/*
 * Print out the error code and message in case the routing request failed
 */
void printFailure(json::Object result) {
    const auto code = result.values["code"].get<json::String>().value;
    const auto message = result.values["message"].get<json::String>().value;

    std::cout << "Code: " << code << "\n";
    std::cout << "Message: " << code << "\n";
}
