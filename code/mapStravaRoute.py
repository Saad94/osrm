# !/usr/bin/python
import sys
import ast
import os
from selenium import webdriver

coords = []

def html_prefix(zoom):
    return "<!DOCTYPE html>\n<html>\n\t<head>\n\t\t<meta name=\"viewport\" content=\"initial-scale=1.0, user-scalable=no\">\n\t\t<meta charset=\"utf-8\">\n\t\t<title>Simple Polylines</title>\n\t\t<style>\n\t\t\t#map {\n\t\t\t\theight: 100%;\n\t\t\t}\n\n\t\t\thtml, body {\n\t\t\t\theight: 100%;\n\t\t\t\tmargin: 0;\n\t\t\t\tpadding: 0;\n\t\t\t}\n\t\t</style>\n\t</head>\n\t<body>\n\t\t<div id=\"map\"></div>\n\t\t<script>\n\t\t\tfunction getRandomColor() {\n\t\t\t\tvar letters = '0123456789ABCDEF';\n\t\t\t\tvar color = '#';\n\t\t\t\tfor (var i = 0; i < 6; i++) {\n\t\t\t\t\tcolor += letters[Math.floor(Math.random() * 16)];\n\t\t\t\t}\n\t\t\t\treturn color;\n\t\t\t}\n\n\t\t\tvar myStyle = [\n\t\t\t\t{\n\t\t\t\t\tfeatureType: \"all\",\n\t\t\t\t\telementType: \"labels\",\n\t\t\t\t\tstylers: [\n\t\t\t\t\t\t{ visibility: \"off\" }\n\t\t\t\t\t]\n\t\t\t\t}\n\t\t\t];\n\n\t\t\tfunction initMap() {\n\t\t\t\tvar map = new google.maps.Map(document.getElementById('map'), {\n\t\t\t\t\tmapTypeControlOptions: {\n\t\t\t\t\t\tmapTypeIds: ['mystyle', google.maps.MapTypeId.ROADMAP, google.maps.MapTypeId.TERRAIN]\n\t\t\t\t\t},\n\t\t\t\t\tzoom: 14,\n\t\t\t\t\tcenter: {lat: 48.892576, lng: 2.227793},\n\t\t\t\t\tmapTypeId: 'mystyle'\n\t\t\t\t});\n\n\t\t\t\tmap.mapTypes.set('mystyle', new google.maps.StyledMapType(myStyle, { name: 'My Style' }));\n\n"

def html_suffix():
    return "\t\t\t}\n\t\t</script>\n\n\t\t<script async defer\n\t\t\tsrc=\"https://maps.googleapis.com/maps/api/js?key=AIzaSyBSCP1vdj1J3Nm4ta-KX47aik0XbZZbQ0U&callback=initMap\">\n\t\t</script>\n\t</body>\n</html>\n"

def html_route(coordinates, i):
    s = "\t\t\t\tvar r_" + str(i) + " = ["

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
        s += "\t\t\t\t\tstrokeColor: getRandomColor(),\n"
        s += "\t\t\t\t\tstrokeOpacity: 1.0,\n"
        s += "\t\t\t\t\tstrokeWeight: 2,\n"
        s += "\t\t\t\t\tmap: map\n"
        s += "\t\t\t\t});\n\n"

    return s


def generate_file(input_file, zoom):
    with open(input_file,'r') as f:
        i = 0
        s = html_prefix(zoom)

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

    if (len(sys.argv) < 5) :
        print "\nUsage: python mapStravaRoute.py <in.txt> <out.html> <out.png> <zoom_level>\n"
        sys.exit()

    cur_dir = os.getcwd()
    in_file = os.path.join(cur_dir, sys.argv[1])
    html_file = os.path.join(cur_dir, sys.argv[2])
    png_file = os.path.join(cur_dir, sys.argv[3])
    zoom = sys.argv[4]

    print "in_file:   ", in_file
    print "html_file: ", html_file
    print "png_file:  ", png_file
    print "zoom:      ", zoom

    s = generate_file(in_file, zoom)

    with open(html_file, 'w') as f:
        f.write(s)

    html_file = 'file://' + html_file
    driver = webdriver.Chrome()
    driver.get(html_file)     
    driver.get_screenshot_as_file(png_file)
    driver.quit()
