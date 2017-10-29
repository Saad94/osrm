#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <utility>
#include <boost/algorithm/string.hpp>
#include <boost/functional/hash.hpp>
using namespace std;

int main(int argc, const char *argv[]) {
    if (argc < 4) {
        cout << "\nUsage: " << argv[0] << " <reduced_osm_file> <heatmap_file> <output_file>" << "\n\n";
        return EXIT_FAILURE;
    }

    int counter = 0;
    string line, buffer;
    string osm_file = argv[1];
    string heatmap_file = argv[2];
    //string outputDir = argv[3];
    string output_file = argv[3];
    unordered_map<uint64_t, pair<double, double>> nodes;
    //string state;
    //if (heatmap_file.rfind("/") != string::npos) {state = heatmap_file.substr(heatmap_file.rfind("/")+1);}
    //else {state = heatmap_file;}
    //string output_file = outputDir + state.substr(0, state.length()-8) + ".coords";

    ifstream file(osm_file);

    if (file.is_open()) {
        while(getline(file, line)) {
            uint64_t id;
            double lat, lon;
            stringstream str(line);
            str >> id >> lat >> lon;
            pair<double, double> coords(lat, lon);
            nodes.insert(pair<uint64_t, pair<double, double>>(id, coords));
        }
    } else {
        cout << "\nError: Couldn't open osm file.\n\n";
        return EXIT_FAILURE;
    }

    file.close();
    file = ifstream(heatmap_file); 
    ofstream outfile = ofstream(output_file);
    outfile.precision(10);

    if (file.is_open()) {
        while(getline(file, line)) {
            uint64_t n1, n2, count;
            stringstream str(line);
            str >> n1 >> n2 >> count;
            outfile << nodes[n1].first << " " << nodes[n1].second << " " << nodes[n2].first << " " << nodes[n2].second << " " << count << "\n";
        }
    } else {
        cout << "\nError: Couldn't open heatmap file.\n\n";
        return EXIT_FAILURE;
    }
    
    file.close();
    outfile.close();

    return EXIT_SUCCESS;
}
