# osrm

## create-heatmap

_Usage: ./create-heatmap <data.osrm> <map.osm> <US\_state\_file> <out.heatmap>_

Goes through every Activity mentioned in the US\_state\_file and uses OSRM
to map every single route to a series of OSM node segments.  
Outputs a heatmap of node segments based on the number of unique users who
ran those segments.

## find-node-coordinates

_Usage: ./find-node-coordinates <in.nodes> <in.heatmap> <out.coords>_

Reads in a heatmap (which contains nodeIDs) and outputs a file where those
nodeIDs are replaced by actual GPS coordinates.

## create-html-file

_Usage: ./create-html-file <in.coords> <out.html>_

Reads in a coords file and outputs an HTML file which contains a map representing
the heatmap.

## reduce-osm-files

_Usage: ./reduce-osm-files <in.osm> <out.nodes>_

Goes through an OSM file and outputs nodeIDs and their GPS coordinates for
use in other files.

## remove-extraneous-nodes

_Usage: ./remove-extraneous-nodes <in.osm> <out.osm>_

Takes in an osm file, considers all the Ways in it and removes those nodes
which only occur once in the Ways.

## routing-algorithm

_Usage: ./routing-algorithm <in.heatmap> <in.nodes> <out.html>_

## find\_us\_states.py

Goes through a list of all the activities and finds which ones are located in the US.
Splits them up by State and writes the activity IDs to individual files.

## mapStravaRoute.py

_Usage: python mapStravaRoute.py <in.txt> <out.html> <out.png> <zoom\_level>_

Takes in one of the Activity files, plots the routes onto a map in an HTML file and
takes a screenshot of the map.
