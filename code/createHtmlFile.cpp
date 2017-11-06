#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>

using namespace std;

string filePrefix() {
    return "<!DOCTYPE html>\n<html>\n\t<head>\n\t\t<meta name=\"viewport\" content=\"initial-scale=1.0, user-scalable=no\">\n\t\t<meta charset=\"utf-8\">\n\t\t<title>Simple Polylines</title>\n\t\t<style>\n\t\t\t#map {\n\t\t\t\theight: 100%;\n\t\t\t}\n\t\t\t\n\t\t\thtml, body {\n\t\t\t\theight: 100%;\n\t\t\t\tmargin: 0;\n\t\t\t\tpadding: 0;\n\t\t\t}\n\t\t</style>\n\t</head>\n\t<body>\n\t\t<div id=\"map\"></div>\n\t\t<script>\n\t\t\tfunction initMap() {\n\t\t\t\tvar map = new google.maps.Map(document.getElementById('map'), {\n\t\t\t\t\tzoom: 12,\n\t\t\t\t\tcenter: {lat: 40.1, lng: -88.2},\n\t\t\t\t\tmapTypeId: 'terrain'\n\t\t\t\t});\n\n\t\t\t\tvar Coords = [";
}

string fileSuffix() {
    return "\t\t\t}\n\t\t</script>\n\n\t\t<script async defer\n\t\tsrc=\"https://maps.googleapis.com/maps/api/js?key=AIzaSyBSCP1vdj1J3Nm4ta-KX47aik0XbZZbQ0U&callback=initMap\">\n\t\t</script>\n\t</body>\n</html>\n";
}

string createLine(double lat1, double lon1, double lat2, double lon2) {
    stringstream str;
    str.precision(10);

    str << "\t\t\t\t\t[\n";
    str << "\t\t\t\t\t\t{lat: " << lat1 << ", lng: " << lon1 << "},\n";
    str << "\t\t\t\t\t\t{lat: " << lat2 << ", lng: " << lon2 << "}\n";
    str << "\t\t\t\t\t]";        
    return str.str();
}

string displayLines(vector<int> lineColorIndexes, vector<string> colorValues) {
    stringstream str;

    str << "\t\t\t\tvar colorValues = [";
    for (int i = 0; i < colorValues.size(); i++) {
        str << "'" << colorValues[i] << "'";
        if (i != colorValues.size()-1) {str << ", ";}
    }
    str << "];\n\n";

    str << "\t\t\t\tvar colorIndexes = [";
    for (int i = 0; i < lineColorIndexes.size(); i++) {
        str << lineColorIndexes[i];
        if (i != lineColorIndexes.size()-1) {str << ", ";}
    }
    str << "];\n\n";

    str << "\t\t\t\tfor (var i = 0; i < colorIndexes.length; i++) {\n";
    str << "\t\t\t\t\tvar Path = new google.maps.Polyline({\n";
    str << "\t\t\t\t\t\tpath: Coords[i],\n";
    str << "\t\t\t\t\t\tgeodesic: true,\n";
    str << "\t\t\t\t\t\tstrokeColor: colorValues[colorIndexes[i]],\n";
    str << "\t\t\t\t\t\tstrokeOpacity: 1.0,\n";
    str << "\t\t\t\t\t\tstrokeWeight: 2,\n";
    str << "\t\t\t\t\t\tmap: map\n";
    str << "\t\t\t\t\t});\n";
    str << "\t\t\t\t}\n";

    return str.str();
}

int main(int argc, const char *argv[]) {
    if (argc < 3) {
        cerr << "\nUsage: " << argv[0] << " <data.coords> <output_file>\n\n";
        return EXIT_FAILURE;
    }
    
    string line;
    double lat1, lon1, lat2, lon2, count, maxCount = -1;
    vector<int> lineColorIndexes;
    string input_filename = argv[1];
    string output_filename = argv[2];
    ifstream file(input_filename);

    vector<string> colorNames = {"blue", "light blue", "pink", "red"};
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

            int index = (int)((count / maxCount) / 0.251);
            lineColorIndexes.push_back(index);

            if (!start) {outfile << ",";}
            start = false;

            outfile << "\n";
            outfile << createLine(lat1, lon1, lat2, lon2);
            //if (i++ == 10) {break;}
        }
        outfile << "\n\t\t\t\t]\n\n";

        outfile << displayLines(lineColorIndexes, colorValues);

        outfile << fileSuffix();

        outfile.close();
    } else {
        cerr << "\nInput File \"" << input_filename << "\" couldn't be opened.\n\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
