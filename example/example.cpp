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

#include <exception>
#include <iostream>
#include <string>
#include <utility>
#include <fstream>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <boost/functional/hash.hpp>
#include <cstdlib>
#include <utility>

using namespace std;
using namespace osrm;

// ============================================================================================
//  Debugging Information In Case Library Fails
// ============================================================================================

bool routeExistsInCurrentRegion(json::Object route) {
    const auto distance = route.values["distance"].get<json::Number>().value;
    const auto duration = route.values["duration"].get<json::Number>().value;

    // Warn users if extract does not contain the default coordinates from above
    if (distance == 0 || duration == 0)
    {
        std::cout << "Note: distance or duration is zero. ";
        std::cout << "You are probably doing a query outside of the OSM extract.\n\n";
        return false;
    }
    return true;
}

void printFailure(json::Object result) {
    const auto code = result.values["code"].get<json::String>().value;
    const auto message = result.values["message"].get<json::String>().value;

    std::cout << "Code: " << code << "\n";
    std::cout << "Message: " << code << "\n";
}

// ============================================================================================
//  Helper Functions
// ============================================================================================

/*
 * Create a pair out of 2 longs, always ordering them in ascending order
 * This helps avoid duplicates such as <123, 456> and <456, 123>
 */
pair<long, long> makePair(long one, long two) {
    if (one < two) {return pair<long, long>(one, two);}
    else {return pair<long, long>(two, one);}
}

/*
 * Print out the entire heatmap
 *
 * <node1, node2> = value
 */
void printHeatmap(unordered_map<pair<long, long>, long, boost::hash<pair<long, long>>> heatmap) {
    for (auto segment : heatmap) {
        cout << "<" << segment.first.first << " " << segment.first.second << "> = " << segment.second << "\n";
    }
}

// ============================================================================================
//  Main
// ============================================================================================

int main(int argc, const char *argv[])
{
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " data.osrm routefile\n";
        return EXIT_FAILURE;
    }

    // Configure based on a .osrm base path, and no datasets in shared mem from osrm-datastore
    EngineConfig config;
    config.storage_config = {argv[1]};
    config.use_shared_memory = false;

    // Routing machine with several services (such as Route, Table, Nearest, Trip, Match)
    const OSRM osrm{config};

    // Setting the RouteParameters to include Node annotations
    RouteParameters params;
    params.annotations = true;
    params.annotations_type = RouteParameters::AnnotationsType::Nodes;

    string line;
    string filename = argv[2];
    ifstream file(filename);

    if (file.is_open()) {
        while (getline(file, line)) {
            boost::erase_all(line, "[");
            boost::erase_all(line, "]");
            boost::replace_all(line, ",", " ");
            cout.precision(10);

            stringstream str(line);
            double lat, lon;

            while (str >> lat >> lon) {
                params.coordinates.push_back({util::FloatLongitude{lon}, util::FloatLatitude{lat}});
            }
        }

        file.close();
    } else {
        cout << "File not opened\n";
    }

    // Response is in JSON format
    json::Object result;

    // HeatMap Object
    unordered_map<pair<long, long>, long, boost::hash<pair<long, long>>> heatmap;

    // Execute routing request, this does the heavy lifting
    const auto status = osrm.Route(params, result);

    if (status == Status::Ok) {
        auto &routes = result.values["routes"].get<json::Array>();

        // Let's just use the first route
        auto &route = routes.values.at(0).get<json::Object>();

        // Check if route exists in the currently loaded map
        if (!routeExistsInCurrentRegion(route)) { return EXIT_FAILURE; }

        auto &legs = route.values["legs"].get<json::Array>();
        
        for(auto leg : legs.values) {
            json::Object legObject = leg.get<json::Object>();
            json::Object annotations = legObject.values["annotation"].get<json::Object>();
            json::Array nodes = annotations.values["nodes"].get<json::Array>();
            
            for (int i = 0; i < nodes.values.size()-1; i++) {
                long node1 = nodes.values[i].get<json::Number>().value;
                long node2 = nodes.values[i+1].get<json::Number>().value;
                pair<long, long> p = makePair(node1, node2);
                if (heatmap.find(p) == heatmap.end()) {
                    heatmap.insert(pair<pair<long, long>, long>(p, 1));
                } else {
                    heatmap[p] = heatmap[p] + 1;
                }
            }
        }

        printHeatmap(heatmap);

        return EXIT_SUCCESS;

    } else if (status == Status::Error) {
        printFailure(result);
        return EXIT_FAILURE;
    }
}
