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
#include <chrono>
#include <thread>

using namespace std;
using namespace osrm;

typedef unordered_map<pair<uint64_t, uint64_t>, uint64_t, boost::hash<pair<uint64_t, uint64_t>>> Heatmap;
typedef unordered_set<pair<uint64_t, uint64_t>, boost::hash<pair<uint64_t, uint64_t>>> Heatset; 

// ============================================================================================
//  Convenience Functions
// ============================================================================================

bool terminateProgram = false;

void signalHandler(int signum) {
    terminateProgram = true;
    cout << "\n\nCtrl+C Detected. Program will terminate in a moment after writing data to file.\n\nPlease wait...\n\n";
}

// ============================================================================================
// Printing Functions
// ============================================================================================

/*
 * Comparator function for sorting
 */
bool compare(pair<pair<uint64_t, uint64_t>, uint64_t> one, pair<pair<uint64_t, uint64_t>, uint64_t> two) {
    return one.second >= two.second;
}

/*
 * Sort the Heatmap by count 
 */
vector<pair<pair<uint64_t, uint64_t>, uint64_t>> sortHeatmap(Heatmap & heatmap) {
    vector<pair<pair<uint64_t, uint64_t>, uint64_t>> pairs;
    
    for (auto itr = heatmap.begin(); itr != heatmap.end(); ++itr) {
        pairs.push_back(*itr);
    }

    sort(pairs.begin(), pairs.end(), compare);

    return pairs;
}


/*
 * Print out the userSet 
 */
void printSet(Heatset & userSet) {
    for (auto segment : userSet) {
        cout << setw(12) << segment.first << "  " << setw(12) << segment.second << "\n";
    }
}

/*
 * Pretty Printing, not used since these files aren't meant to be manually read
 * Makes working with output annoying
 * Print out the entire Heatmap in the form:   node1   node2   value
 */
void prettyPrintHeatmap(Heatmap & heatmap, ostream & stream) {
    auto sortedHeatmap = sortHeatmap(heatmap);

    stream << "\n" << setw(12) << "Node1" << "  " << setw(12) << "Node2" << "  " << setw(8) << "Count" << "\n\n";
    for (auto segment : sortedHeatmap) {
        stream << setw(12) << segment.first.first << "  " << setw(12) << segment.first.second << "  " << setw(8) << segment.second << "\n";
    }
}

/*
 * Print out the entire Heatmap in the form:   node1   node2   value
 */
void printHeatmap(Heatmap & heatmap, ostream & stream) {
    auto sortedHeatmap = sortHeatmap(heatmap);

    //for (auto segment : heatmap) {
    for (auto segment : sortedHeatmap) {
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
//  Helper Functions
// ============================================================================================

/*
 * Read the map's boundaries from the osm file
 */
vector<double> getMapBoundaries(string filename_osm) {
    vector<double> v;
    double d;
    string line, cpy = "";
    ifstream file(filename_osm);

    if (!file.is_open()) {
        cout << "\nOSM File couldn't be opened.\n\n";
        exit(EXIT_FAILURE);
    }

    getline(file, line); getline(file, line); getline(file, line);

    for (char c : line) {
        if (c == ' ' || c == '-' || c == '.' || (c >= '0' && c <= '9')) {
            cpy += c;
        }
    }

    boost::trim_left(cpy);
    stringstream str(cpy);
    
    str >> d; v.push_back(d);
    str >> d; v.push_back(d);
    str >> d; v.push_back(d);
    str >> d; v.push_back(d);
    
    return v;
}

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
 *  Extract pairs of nodes from the route and insert into the userMap
 */
void insertRouteIntoMap(json::Object route, Heatmap & userMap) {
    auto &legs = route.values["legs"].get<json::Array>();
        
    for(auto leg : legs.values) {
        json::Object legObject = leg.get<json::Object>();
        json::Object annotations = legObject.values["annotation"].get<json::Object>();
        json::Array nodes = annotations.values["nodes"].get<json::Array>();
        
        for (int i = 0; i < nodes.values.size()-1; i++) {
            uint64_t node1 = nodes.values[i].get<json::Number>().value;
            uint64_t node2 = nodes.values[i+1].get<json::Number>().value;
            pair<uint64_t, uint64_t> segment = makePair(node1, node2);

            if (userMap.find(segment) == userMap.end()) {
                userMap.insert(pair<pair<uint64_t, uint64_t>, uint64_t>(segment, 1));
            } else {
                userMap[segment] = userMap[segment] + 1;
            }
        }
    }
}

/*
 * Insert pairs from the userMap into the Heatmap
 */
void insertUserIntoMap(Heatmap & userMap, Heatmap & heatmap) {
    for (auto entry : userMap) {
        auto segment = entry.first;
        uint64_t count = entry.second;

        // Pruning step, don't insert segments which only occur once
        //if (count > 1) {
            if (heatmap.find(segment) == heatmap.end()) {
                heatmap.insert(pair<pair<uint64_t, uint64_t>, uint64_t>(segment, 1));
            } else {
                heatmap[segment] = heatmap[segment] + 1;
            }
        //}
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
//bool routeExistsInCurrentRegion(json::Object route) {
    //const auto distance = route.values["distance"].get<json::Number>().value;
    //const auto duration = route.values["duration"].get<json::Number>().value;
    //return distance != 0 && duration != 0;
//}

bool routeExistsInCurrentRegion(vector<double> mapBoundaries, util::Coordinate start, util::Coordinate end) {
    vector<util::Coordinate> coords; coords.push_back(start); coords.push_back(end);
    util::FloatLatitude minLat{mapBoundaries[0]}, maxLat{mapBoundaries[2]};
    util::FloatLongitude minLon{mapBoundaries[1]}, maxLon{mapBoundaries[3]};
    int outofbounds = 0;

    for (util::Coordinate coord : coords) {
        util::FloatLatitude lat = util::toFloating(coord.lat);
        util::FloatLongitude lon = util::toFloating(coord.lon);

        if (!(lat >= minLat && lat <= maxLat && lon >= minLon && lon <= maxLon)) {
            outofbounds++;           
        }
    }
    
    return outofbounds != 2;
}

// ============================================================================================
//  Thread Work
// ============================================================================================

void doWork(EngineConfig config, int start, int end, vector<string> & filenames, Heatmap *heatmap, vector<double> & mapBoundaries) {
    
    // Routing machine with several services (such as Route, Table, Nearest, Trip, Match)
    const OSRM osrm{config};

    // User specific Heatmap Object
    Heatmap userMap;

    int TEST = 0;

    string routeData;

    for (int i = start; i < end && !terminateProgram; i++) {
        string file = filenames[i];

        // Find which of the 2 athlete directories file is in and open it
        ifstream file_athlete = openFile(file);
        //ifstream file_athlete(file);

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

                if (routeExistsInCurrentRegion(mapBoundaries, params.coordinates[0], params.coordinates[params.coordinates.size()-1])) {

                    // Execute routing request, this does the heavy lifting
                    const auto status = osrm.Route(params, result);

                    if (status == Status::Ok) {
                        auto &routes = result.values["routes"].get<json::Array>();
                        
                        // Let's just use the first route
                        auto &route = routes.values.at(0).get<json::Object>();
                       
                        // Check if route exists in the currently loaded map
                        //if (routeExistsInCurrentRegion(route)) 
                        //{
                            // Extract pairs of nodes from the route and insert into a userHeatmap
                            insertRouteIntoMap(route, userMap);
                        //} else {
                            //printRouteNotInCurrentRegion(params, filename_athlete);
                            //TEST++;
                        //}
                    } else if (status == Status::Error) {
                        printFailure(result);
                    }
                } else {
                    TEST++;
                }
            }

            // Insert pairs from the userMap into the Heatmap
            insertUserIntoMap(userMap, *heatmap);

            userMap.clear();
            file_athlete.close();
        }

        if (i%200 == 0) { cout << this_thread::get_id() << " i = " << i << "\n"; }
    }
    cout << this_thread::get_id() << " FAILED = " << TEST << "\n";
}

/*
 * Combine the individual heatmaps of each thread into a single one
 */
void mergeHeatmaps(Heatmap heatmaps[], int num) {
    Heatmap & main = heatmaps[0];

    for (int i = 1; i < num; i++) {
        Heatmap cur = heatmaps[i];
        
        for (auto segment : cur) {
            if (main.find(segment.first) == main.end()) {
                main.insert(pair<pair<uint64_t, uint64_t>, uint64_t>(segment.first, segment.second));
            } else {
                main[segment.first] = main[segment.first] + segment.second;
            }
        }
    }
}

// ============================================================================================
//  Main
// ============================================================================================

int main(int argc, const char *argv[])
{
    if (argc < 5) {
        cerr << "\nUsage: " << argv[0] << " <data.osrm> <map.osm> <US_state_file> <output_file>\n\n";
        return EXIT_FAILURE;
    }

    // Register signal handler to allow prevent data loss upon program termination
    signal(SIGINT, signalHandler);

    string filename_athlete, routeData;
    string filename_osrm = argv[1];
    string filename_osm = argv[2];
    string filename_US_state = argv[3];
    string filename_output = argv[4];
    
    // Configure based on a .osrm base path, and no datasets in shared mem from osrm-datastore
    EngineConfig config;
    config.storage_config = {filename_osrm};
    config.use_shared_memory = false;

    // Thread related stuff
    const int NUM_THREADS = 16;
    thread threads[NUM_THREADS];
    
    // HeatMap Object
    Heatmap heatmaps[NUM_THREADS];

    ifstream file_US_state(filename_US_state);
    ofstream outfile(filename_output);
    vector<string> filenames;
    filenames.reserve(500000);

    vector<double> mapBoundaries = getMapBoundaries(filename_osm);

    cout << "\nOpening State File: " << filename_US_state << "\n\n";
    int TEST = 0;
    if (file_US_state.is_open()) {
        while (getline(file_US_state, filename_athlete) && !terminateProgram) {
            filenames.push_back(filename_athlete);
            if (TEST++ == 1000) {break;}
        }
    } else {
        cout << "State File not opened\n";
        return EXIT_FAILURE;
    }
    //filenames.push_back("tmproute.txt");
    cout << "filenames.size() = " << filenames.size() << "\n\n";

    int num = filenames.size();
    int frac = num / NUM_THREADS + 1;

    for (int i = 0; i < NUM_THREADS; i++) {
        int start = frac * i;
        int end = frac * (i+1);
        end = num < end ? num : end;
        threads[i] = thread(doWork, config, start, end, ref(filenames), &heatmaps[i], ref(mapBoundaries));
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        threads[i].join();
    } 

    mergeHeatmaps(heatmaps, NUM_THREADS);

    printHeatmap(heatmaps[0], outfile);
    file_US_state.close();
    outfile.close();

    return EXIT_SUCCESS; 
}
