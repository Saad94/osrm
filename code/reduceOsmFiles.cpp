#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <boost/algorithm/string.hpp>
using namespace std;

vector<int> findChar(string line, char c) {
    vector<int> positions; 
    positions.reserve(10);
    for (int i = 0; i < line.length(); i++) {
        if (line[i] == c) {positions.push_back(i);}
    }
    return positions;
}

string substr(string line, int start, int end) {
    return line.substr(start+1, end-start-1);
}

int main(int argc, const char *argv[]) {
    if (argc < 3) {
        cout << "\nUsage: " << argv[0] << " <in.osm> <out.nodes>" << "\n\n";
        return EXIT_FAILURE;
    }

    int counter = 0;
    string line, buffer;
    string osm_file = argv[1];
    string output_file = argv[2];  

    ifstream file(osm_file);

    if (file.is_open()) {
        ofstream output(output_file);
        
        if (output.is_open()) {
            while(getline(file, line)) {
                boost::trim_left(line);
                if (line.substr(0, 5) == "<node") {
                    line = line.substr(6, line.find("version")-7);
                    vector<int> pos = findChar(line, '"');
                    buffer += substr(line, pos[0], pos[1]) + " " + substr(line, pos[2], pos[3]) + " " + substr(line, pos[4], pos[5]) + "\n";
                    
                    if (counter % 1000 == 0) {
                        output << buffer;
                        buffer = "";
                    }
                }
            }

            output << buffer;
            file.close();
            output.close();
        } else {
            file.close();
            cout << "\nError: Couldn't open output file.\n\n";
            return EXIT_FAILURE;
        }
    } else {
        cout << "\nError: Couldn't open osm file.\n\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
