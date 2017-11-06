#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>

using namespace std;

string colorStyle(string name, string value) {
    stringstream str; 
    str << "\t\t<Style id=\"" << name << "\">\n";
    str << "\t\t\t<LineStyle>\n";
    str << "\t\t\t\t<color>" << value << "</color>\n";
    str << "\t\t\t\t<width>3</width>\n";
    str << "\t\t\t</LineStyle>\n";
    str << "\t\t</Style>\n";
    return str.str();
}

string placemark(double lat1, double lon1, double lat2, double lon2, string color, double count) {
    stringstream str;
    str << "\t\t<Placemark>\n\t\t\t<name>" << count << "</name>\n\t\t\t<styleUrl>#" << color << "</styleUrl>\n";
    str << "\t\t\t<LineString>\n\t\t\t\t<altitudeMode>relative</altitudeMode>\n\t\t\t\t<coordinates>\n";
    str << "\t\t\t\t\t" << lon1 << "," << lat1 << ",0\n";
    str << "\t\t\t\t\t" << lon2 << "," << lat2 << ",0\n";
    str << "\t\t\t\t</coordinates>\n\t\t\t</LineString>\n\t\t</Placemark>\n";
    return str.str();
}

int main(int argc, const char *argv[]) {
    if (argc < 3) {
        cerr << "\nUsage: " << argv[0] << " <data.coords> <output_file>\n\n";
        return EXIT_FAILURE;
    }
    
    string line;
    double lat1, lon1, lat2, lon2, count, maxCount = -1;
    string input_filename = argv[1];
    string output_filename = argv[2];
    ifstream file(input_filename);

    const char *colors[] = {"blue", "yellow", "orange", "red"};
    const char *colorValues[] = {"ffff0000", "ff61f2f2", "ff00ccff", "ff0000ff"};

    if (file.is_open()) {
        ofstream outfile(output_filename);

        if (!outfile.is_open()) {
            cerr << "\nOutput File \"" << output_filename << "\" couldn't be opened.\n\n";
            return EXIT_FAILURE;
        }
        
        outfile.precision(10);

        outfile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        outfile << "<kml xmlns=\"http://earth.google.com/kml/2.1\">\n";
        outfile << "\t<Document>\n";
        outfile << "\t\t<name>" << input_filename << "</name>\n";
        outfile << colorStyle(colors[0], colorValues[0]);
        outfile << colorStyle(colors[1], colorValues[1]);
        outfile << colorStyle(colors[2], colorValues[2]);
        outfile << colorStyle(colors[3], colorValues[3]);
       
        //enum

        int i = 0;
        while (getline(file, line)) {
            stringstream str(line);
            str >> lat1 >> lon1 >> lat2 >> lon2 >> count;
            if (maxCount == -1) {maxCount = count;}

            int index = (int)((count / maxCount) / 0.251);
            string color = colors[index];

            outfile << placemark(lat1, lon1, lat2, lon2, color, count);
            //if (i++ == 1000) {break;}
        }

        outfile << "\t</Document>\n</kml>\n";

        outfile.close();
    } else {
        cerr << "\nInput File \"" << input_filename << "\" couldn't be opened.\n\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
