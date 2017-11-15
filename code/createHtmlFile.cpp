#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>

using namespace std;

string filePrefix() {
    return "<!DOCTYPE html>\n<html>\n<head>\n<meta name=\"viewport\" content=\"initial-scale=1.0, user-scalable=no\">\n<meta charset=\"utf-8\">\n<title>Simple Polylines</title>\n<style>\n#map {\nheight: 100%;\n}\n\nhtml, body {\nheight: 100%;\nmargin: 0;\npadding: 0;\n}\n</style>\n</head>\n<body>\n<div id=\"map\"></div>\n<script>\nfunction initMap() {\nvar map = new google.maps.Map(document.getElementById('map'), {\nzoom: 12,\ncenter: {lat: 40.1, lng: -88.2},\nmapTypeId: 'terrain'\n});\n\n";
}

string fileSuffix() {
    return "}\n</script>\n\n<script async defer\nsrc=\"https://maps.googleapis.com/maps/api/js?key=AIzaSyBSCP1vdj1J3Nm4ta-KX47aik0XbZZbQ0U&callback=initMap\">\n</script>\n</body>\n</html>\n";
}

string createArray(string name) {
    return "var " + name + " = [";
}

string createLine(double lat1, double lon1, double lat2, double lon2) {
    stringstream str;
    str.precision(10);

    str << "[\n";
    str << "{lat: " << lat1 << ", lng: " << lon1 << "},\n";
    str << "{lat: " << lat2 << ", lng: " << lon2 << "}\n";
    str << "]";        
    return str.str();
}

string displayLines(string arrayName, string colorValue) {
    stringstream str;

    str << "for (var i = 0; i < " << arrayName << ".length; i++) {\n";
    str << "var Path = new google.maps.Polyline({\n";
    str << "path: " << arrayName << "[i],\n";
    str << "geodesic: true,\n";
    str << "strokeColor: '" << colorValue << "',\n";
    str << "strokeOpacity: 1.0,\n";
    str << "strokeWeight: 2,\n";
    str << "map: map\n";
    str << "});\n";
    str << "}\n\n";

    return str.str();
}

int main(int argc, const char *argv[]) {
    if (argc < 3) {
        cerr << "\nUsage: " << argv[0] << " <in.coords> <out.html>\n\n";
        return EXIT_FAILURE;
    }
    
    string line;
    double lat1, lon1, lat2, lon2, count, maxCount = -1;
    string input_filename = argv[1];
    string output_filename = argv[2];
    ifstream file(input_filename);
    int oldColor = -1;
    vector<int> usedColors;

    vector<string> colorNames = {"blue", "light_blue", "pink", "red"};
    vector<string> colorValues = {"#4d4dc1", "#0099ff", "#f984f2", "#ff0000"};

    if (file.is_open()) {
        ofstream outfile(output_filename);

        if (!outfile.is_open()) {
            cerr << "\nOutput File \"" << output_filename << "\" couldn't be opened.\n\n";
            return EXIT_FAILURE;
        }
        
        outfile.precision(10);

        bool start = true;
        outfile << filePrefix();       

        //int i = 0;
        while (getline(file, line)) {
            stringstream str(line);
            str >> lat1 >> lon1 >> lat2 >> lon2 >> count;
            if (maxCount == -1) {maxCount = count;}

            int color = (int)((count / maxCount) / 0.251);
            if (oldColor != color) {
                if (oldColor != -1) {outfile << "\n];\n\n";}
                oldColor = color;
                usedColors.push_back(color);
                outfile << createArray(colorNames[color]);
                start = true;
            }

            if (!start) {outfile << ",";}
            start = false;

            outfile << "\n";
            outfile << createLine(lat1, lon1, lat2, lon2);
            //if (i++ == 100) {break;}
        }
        outfile << "\n];\n\n";

        for (int i : usedColors) {
            outfile << displayLines(colorNames[i], colorValues[i]);
        }

        outfile << fileSuffix();

        outfile.close();
    } else {
        cerr << "\nInput File \"" << input_filename << "\" couldn't be opened.\n\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
