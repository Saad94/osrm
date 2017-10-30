#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <boost/algorithm/string.hpp>
using namespace std;

class Way {
  public:
    vector<string> nodes;
    bool highway;

    Way() {highway = false;}   
};

class Segment {
  public:
      string prev, cur, next, endpoint, highway;

      Segment() {}
      Segment(string p, string c, string n, string e, string h) {prev = p; cur = c; next = n; endpoint = e; highway = h;}
};

ostream& operator<<(ostream & stream, const vector<string> & v) {
    for (string s : v) {
        stream << s << " ";
    }
    return stream;
}

ostream& operator<<(ostream & stream, const vector<vector<string>> & v2) {
    for (vector<string> v : v2) {
        stream << v << "\n";
    }
    return stream;
}

string parseCurLine(string line) {
    boost::trim_left(line); 
    
    if (boost::starts_with(line, "<way")) {
        return "start";
    } else if (boost::starts_with(line, "</way")) {
        return "end";
    } else if (boost::starts_with(line, "<nd")) {
        return line.substr(9, line.length()-9-3);
    } else {
        if (line.find("k=\"highway\"") != string::npos) {
            return "highway";
        } else {
            return "other";
        }
    }
}

void extractWays(ifstream & file, vector<Way> & ways, vector<Way> & highways) {
    string line;

    while(getline(file, line)) {
        string tmp = parseCurLine(line);

        if (tmp == "start") {
            Way way;

            while(getline(file, line)) {
                tmp = parseCurLine(line);
                if (tmp == "end") {
                    ways.push_back(way);
                    if (way.highway) {highways.push_back(way);}
                    break;
                } else if (tmp == "other") {
                    continue;
                } else if (tmp == "highway") {
                    way.highway = true;
                } else {
                    way.nodes.push_back(tmp);
                }
            }
        }
    }
}

void extractSegments(vector<Way> & ways, vector<Segment> & segments) {
    for (Way way : ways) {
        string prev, cur, next, endpoint;

        for (int i = 0; i < way.nodes.size(); i++) {
            prev = i == 0 ? "" : way.nodes[i-1];
            cur = way.nodes[i];
            next = i == way.nodes.size()-1 ? "" : way.nodes[i+1];
            endpoint = (i == 0 || i == way.nodes.size()-1) ? "true" : "false";
            segments.push_back(Segment(prev, cur, next, endpoint, "false"));
        }
    }
}

int main(int argc, const char *argv[]) {
    if (argc < 3) {
        cout << "\nUsage: " << argv[0] << " <osm_file> <output_file>" << "\n\n";
        return EXIT_FAILURE;
    }

    int counter = 0;
    string line, buffer;
    string osm_file = argv[1];
    string output_file = argv[2];

    ifstream file(osm_file);
    ofstream output(output_file);
    unordered_map<string, vector<vector<string>>> nodeNeighbours;
    vector<Way> ways;
    vector<Way> highways;
    vector<Segment> segments;
    vector<Segment> highwaysegments;

    /*
     * Extract Ways
     */

    if (file.is_open()) {
        extractWays(file, ways, highways);    
        cout << "total ways = " << ways.size() << "\nhighways = " << highways.size() << "\n\n";
        file.close();
    } else {
        cout << "\nError: Couldn't open osm file.\n\n";
        return EXIT_FAILURE;
    }

    /*
     * Extract Segments
     */

    extractSegments(ways, segments);
    extractSegments(highways, highwaysegments);

    for (Segment segment : segments) {
        string prev = segment.prev;
        string cur = segment.cur;
        string next = segment.next;
        string endpoint = segment.endpoint;
        string highway = segment.highway;
        vector<string> v; v.push_back(prev); v.push_back(next); v.push_back(endpoint); v.push_back(highway);
        vector<vector<string>> blank;

        if (nodeNeighbours.find(cur) == nodeNeighbours.end()) {
            nodeNeighbours.insert(pair<string, vector<vector<string>>>(cur, blank));
        }
        nodeNeighbours[cur].push_back(v);
    }

    int numOnes = 0, numEndpoints = 0;
    unordered_set<string> uniqueNodes;

    for (Segment segment : highwaysegments) {
        uniqueNodes.insert(segment.cur);
        if (nodeNeighbours[segment.cur].size() == 1) {
            nodeNeighbours[segment.cur][0][3] = "true";
            numOnes++;
            if (segment.endpoint == "true") {numEndpoints++;}
        }
    }
    cout << "uniqueNodes = " << uniqueNodes.size() << "\n";
    cout << "highwaysegments = " << highwaysegments.size() << "\n";
    cout << "numOnes = " << numOnes << "\n";
    cout << "numEndpoints = " << numEndpoints << "\n";
    file = ifstream(osm_file);

    if (file.is_open()) {
        while (getline(file, line)) {
            string cpy = string(line);
            cpy = parseCurLine(cpy);

            if (cpy[0] >= '0' && cpy[0] <= '9') {
                /* 
                 * <nd> tag inside a way
                 * if (occurs_once && is_not_an_endpoint && is_a_highway_node) {skip}
                 */
                if (nodeNeighbours[cpy].size() == 1 && nodeNeighbours[cpy][0][2] == "false" && nodeNeighbours[cpy][0][3] == "true") {
                    // Skip that node
                } else {
                    buffer += line + "\n";
                }
            } else {
                buffer += line + "\n";
            }

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
