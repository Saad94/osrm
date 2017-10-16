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
#include <iomanip>
#include <unordered_set>
#include <csignal>

using namespace std;
using namespace osrm;

// ============================================================================================
//  Convenience Functions
// ============================================================================================

bool terminateProgram = false;

void signalHandler(int signum) {
    terminateProgram = true;
    cout << "\n\nCtrl+C Detected. Program will terminate in a moment after writing data to file.\n\nPlease wait...\n\n";
}

// ============================================================================================
//  Helper Functions
// ============================================================================================

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
    stringstream str = prepareLineForParsing(routeData);

    while (str >> lat >> lon) {
        params.coordinates.push_back({util::FloatLongitude{lon}, util::FloatLatitude{lat}});
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
 *  Extract pairs of nodes from the route and insert into the userSet
 */
void insertRouteIntoSet(json::Object route, unordered_set<pair<uint64_t, uint64_t>, boost::hash<pair<uint64_t, uint64_t>>> & userSet) {
    auto &legs = route.values["legs"].get<json::Array>();
        
    for(auto leg : legs.values) {
        json::Object legObject = leg.get<json::Object>();
        json::Object annotations = legObject.values["annotation"].get<json::Object>();
        json::Array nodes = annotations.values["nodes"].get<json::Array>();
        
        for (int i = 0; i < nodes.values.size()-1; i++) {
            uint64_t node1 = nodes.values[i].get<json::Number>().value;
            uint64_t node2 = nodes.values[i+1].get<json::Number>().value;
            pair<uint64_t, uint64_t> segment = makePair(node1, node2);
            userSet.insert(segment);
        }
    }
}

/*
 * Insert pairs from the userSet into the Heatmap
 */
void insertSetIntoMap(unordered_set<pair<uint64_t, uint64_t>, boost::hash<pair<uint64_t, uint64_t>>> userSet, 
        unordered_map<pair<uint64_t, uint64_t>, uint64_t, boost::hash<pair<uint64_t, uint64_t>>> & heatmap) {
    for (auto segment : userSet) {
        if (heatmap.find(segment) == heatmap.end()) {
            heatmap.insert(pair<pair<uint64_t, uint64_t>, uint64_t>(segment, 1));
        } else {
            heatmap[segment] = heatmap[segment] + 1;
        }
    }
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
 * Check whether the route exists inside the currently loaded map region
 */
bool routeExistsInCurrentRegion(json::Object route) {
    const auto distance = route.values["distance"].get<json::Number>().value;
    const auto duration = route.values["duration"].get<json::Number>().value;
    return distance != 0 && duration != 0;
}

// ============================================================================================
// Printing Functions
// ============================================================================================

/*
 * Print out the userSet 
 */
void printSet(unordered_set<pair<uint64_t, uint64_t>, boost::hash<pair<uint64_t, uint64_t>>> userSet) {
    for (auto segment : userSet) {
        cout << setw(12) << segment.first << "  " << setw(12) << segment.second << "\n";
    }
}

/*
 * Pretty Printing, not used since these files aren't meant to be manually read
 * Makes working with output annoying
 * Print out the entire Heatmap in the form:   node1   node2   value
 */
void prettyPrintHeatmap(unordered_map<pair<uint64_t, uint64_t>, uint64_t, boost::hash<pair<uint64_t, uint64_t>>> heatmap, ostream & stream) {
    stream << "\n" << setw(12) << "Node1" << "  " << setw(12) << "Node2" << "  " << setw(8) << "Count" << "\n\n";
    for (auto segment : heatmap) {
        stream << setw(12) << segment.first.first << "  " << setw(12) << segment.first.second << "  " << setw(8) << segment.second << "\n";
    }
}

/*
 * Print out the entire Heatmap in the form:   node1   node2   value
 */
void printHeatmap(unordered_map<pair<uint64_t, uint64_t>, uint64_t, boost::hash<pair<uint64_t, uint64_t>>> heatmap, ostream & stream) {
    for (auto segment : heatmap) {
        stream << segment.first.first << " " << segment.first.second << " " << segment.second << "\n";
    }
}

/*
 * Print out whenever a route doesn't exist within the currently loaded map
 */
void printRouteNotInCurrentRegion(RouteParameters & params, string filename_athlete) {
    cout << "Error: Route not found. " << filename_athlete << " " << params.coordinates[0] << " " << params.coordinates[params.coordinates.size()-1] << "\n";
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

// ============================================================================================
//  Main
// ============================================================================================

int main(int argc, const char *argv[])
{
    if (argc < 4) {
        std::cerr << "\nUsage: " << argv[0] << " <data.osrm> <US_state_file> <outputDir>\n\n";
        return EXIT_FAILURE;
    }

    // Register signal handler to allow prevent data loss upon program termination
    signal(SIGINT, signalHandler);

    // Configure based on a .osrm base path, and no datasets in shared mem from osrm-datastore
    EngineConfig config;
    config.storage_config = {argv[1]};
    config.use_shared_memory = false;

    // Routing machine with several services (such as Route, Table, Nearest, Trip, Match)
    const OSRM osrm{config};
    
    // HeatMap Object
    unordered_map<pair<uint64_t, uint64_t>, uint64_t, boost::hash<pair<uint64_t, uint64_t>>> heatmap;

    // Set Object (so that each user's nodes are only counted once)
    unordered_set<pair<uint64_t, uint64_t>, boost::hash<pair<uint64_t, uint64_t>>> userSet;

    string filename_athlete, routeData;
    string filename_US_state = argv[2];
    string outputDir = argv[3];
    string state;
    if (filename_US_state.rfind("/") != string::npos) {state = filename_US_state.substr(filename_US_state.rfind("/")+1);}
    else {state = filename_US_state;}
    string filename_output = outputDir + state.substr(0, state.length()-4) + ".heatmap";
    
    ifstream file_US_state(filename_US_state);
    ofstream outfile(filename_output); outfile << "hi"; outfile.close();

    cout << "\nOpening State File: " << filename_US_state << "\n\n";
    int i = 0; 

    if (file_US_state.is_open()) {
        while (getline(file_US_state, filename_athlete) && !terminateProgram) 
        {
            // Find which of the 2 athlete directories file is in and open it
            ifstream file_athlete = openFile(filename_athlete);

            if (file_athlete.is_open()) { 
                while (getline(file_athlete, routeData)) 
                {
                    // json object to store the result of the routing request
                    json::Object result;

                    // Create a RouteParameters object with Node Annotations
                    RouteParameters params = createRouteParameters();

                    // Parse the data of a single route and input coordinates into the RouteParameters project
                    extractRouteDataIntoParams(routeData, params);

                    // Skip current route if it doesn't contain enough coordinates
                    if (params.coordinates.size() < 2) { continue; }

                    // Execute routing request, this does the heavy lifting
                    const auto status = osrm.Route(params, result);

                    if (status == Status::Ok) {
                        auto &routes = result.values["routes"].get<json::Array>();
                        
                        // Let's just use the first route
                        auto &route = routes.values.at(0).get<json::Object>();
                       
                        // Check if route exists in the currently loaded map
                        if (routeExistsInCurrentRegion(route)) 
                        {
                            // Extract pairs of nodes from the route and insert into the userSet
                            insertRouteIntoSet(route, userSet);                        
                        } else {
                            printRouteNotInCurrentRegion(params, filename_athlete);
                        }
                    } else if (status == Status::Error) {
                        printFailure(result);
                    }
                }

                // Insert pairs from the userSet into the Heatmap
                insertSetIntoMap(userSet, heatmap);

                userSet.clear();
                file_athlete.close();
            }

           if (++i%10 == 0) { cout << "i = " << i << "\n"; }
        }

        printHeatmap(heatmap, outfile);
        file_US_state.close();
        outfile.close();
    } else {
        cout << "File not opened\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS; 
}
