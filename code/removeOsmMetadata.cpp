#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <boost/algorithm/string.hpp>
using namespace std;

int main(int argc, const char *argv[]) {
    if (argc < 3) {
        cout << "\nUsage: " << argv[0] << " <osm_file> <output_file>" << "\n\n";
        return EXIT_FAILURE;
    }

    int counter = 0;
    string line, tagline, buffer;
    string osm_file = argv[1];
    string output_file = argv[2];

    ifstream file(osm_file);

    if (file.is_open()) {
        ofstream output(output_file);
        int i = 0;
        while(getline(file, line)) {
            string cpy = string(line);
            boost::trim_left(cpy);

            //if (boost::starts_with(cpy, "<node")) {
                //line = line.substr(0, line.find("version")-1);
                //line += "/>\n";
            //} else if (boost::starts_with(cpy, "</node")) {
                //// If closing node tag, ignore
                //line = "";
            //} else if (boost::starts_with(cpy, "<tag")) {
                //// If tag tag, ignore
                //line = "";
            //} else {
                //line += "\n";
            //}

            if (cpy.substr(0, 5) == "<node") {
                line = line.substr(0, line.find("version")-1);
                
                if (cpy.substr(cpy.length()-2) == "/>") {
                    // No Tags
                    line += "/>\n";
                } else {
                    // Tags
                    line += "/>\n";
                    while(getline(file, tagline)) {
                        cpy = string(tagline);
                        boost::trim_left(cpy);
                        //line += tagline + "\n";
                        if (cpy == "</node>") {break;}
                    }
                }
            } else if (boost::starts_with(cpy, "<relation") || boost::starts_with(cpy, "<way")){
                // Ways and Relations
                //line += "\n";
                line = line.substr(0, line.find("version")-1);
                
                if (cpy.substr(cpy.length()-2) == "/>") {
                    // No Tags
                    line += "/>\n";
                } else {
                    // Tags
                    line += ">\n";
                    string tmp = string(cpy);
                    while(getline(file, tagline)) {
                        cpy = string(tagline);
                        boost::trim_left(cpy);
                        if (boost::starts_with(tmp, "<relation") && boost::starts_with(cpy, "<tag")) {continue;}
                        line += tagline + "\n";
                        if (cpy == "</way>" || cpy == "</relation>") {break;}
                    }
                }
            }
            
            buffer += line;

            if (counter % 1000 == 0) {
                output << buffer;
                buffer = "";
            }
        }

        output << buffer;
        file.close();
        output.close();
    } else {
        cout << "\nError: Couldn't open osm file.\n\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
