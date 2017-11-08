# !/usr/bin/python
import sys
import ast

coords = []

def html_prefix():
    return "<!DOCTYPE html>\n<html>\n\t<head>\n\t\t<meta name=\"viewport\" content=\"initial-scale=1.0, user-scalable=no\">\n\t\t<meta charset=\"utf-8\">\n\t\t<title>Simple Polylines</title>\n\t\t<style>\n\t\t\t#map {\n\t\t\t\theight: 100%;\n\t\t\t}\n\n\t\t\thtml, body {\n\t\t\t\theight: 100%;\n\t\t\t\tmargin: 0;\n\t\t\t\tpadding: 0;\n\t\t\t}\n\t\t</style>\n\t</head>\n\t<body>\n\t\t<div id=\"map\"></div>\n\t\t<script>\n\t\t\tfunction initMap() {\n\t\t\t\tvar map = new google.maps.Map(document.getElementById('map'), {\n\t\t\t\t\tzoom: 11,\n\t\t\t\t\tcenter: {lat: _lat_placeholder_, lng: _lng_placeholder_},\n\t\t\t\t\tmapTypeId: 'terrain'\n\t\t\t\t});\n\n"

def html_suffix():
    return "\t\t\t}\n\t\t</script>\n\n\t\t<script async defer\n\t\t\tsrc=\"https://maps.googleapis.com/maps/api/js?key=AIzaSyBSCP1vdj1J3Nm4ta-KX47aik0XbZZbQ0U&callback=initMap\">\n\t\t</script>\n\t</body>\n</html>\n"

def html_route(coordinates, i):
    s = "\t\t\tvar r_" + str(i) + " = ["

    for coord in coordinates:
        s += "{lat: " + str(coord[0]) + ", lng: " + str(coord[1]) + "}, "

    s = s[:-2] + "];\n\n"

    return s

def html_display_route(max_i):
    s = ""

    for i in range(max_i):
        s += "\t\t\t\tvar Path_" + str(i) + " = new google.maps.Polyline({\n"
        s += "\t\t\t\t\tpath: r_" + str(i) + ",\n"
        s += "\t\t\t\t\tgeodesic: true,\n"
        s += "\t\t\t\t\tstrokeColor: '#0000CC',\n"
        s += "\t\t\t\t\tstrokeOpacity: 1.0,\n"
        s += "\t\t\t\t\tstrokeWeight: 2,\n"
        s += "\t\t\t\t\tmap: map\n"
        s += "\t\t\t\t});\n\n"

    return s


def generate_file(input_file):
    with open(input_file,'r') as f:
        i = 0
        s = html_prefix()

        for line in f:
            try:
                line = line.replace(' ', ',')
                coordinates = list(ast.literal_eval(line))
            except:
                print "error: couldn't get coordinates from line: ", line
                continue

            if (i == 0):
                s = s.replace('_lat_placeholder_', str(coordinates[0][0]))
                s = s.replace('_lng_placeholder_', str(coordinates[0][1]))
            s += html_route(coordinates, i)
            i += 1

        s += html_display_route(i)
        s += html_suffix()

        return s

if __name__ == '__main__':

    if (len(sys.argv) < 3) :
        print "\nUsage: python mapStravaRoute.py <in.txt> <out.html>\n"
        sys.exit()

    in_file = sys.argv[1]
    out_file = sys.argv[2]
   
    print "in_file:  ", in_file
    print "out_file: ", out_file

    s = generate_file(in_file)

    with open(out_file,'w') as f:
        f.write(s)
