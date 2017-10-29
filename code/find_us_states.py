# !/usr/bin/python
import sys
import reverse_geocoder as rg
from collections import defaultdict
import ast

infile = "/srv/data/strava-dataset/total_athId_loc.txt"
outdir = "/srv/data/strava-dataset/states/"
us_map = defaultdict(list)
ids = []
coords = []

def read_generate(input_file):
    count_total = 0
    count_us = 0

    with open(input_file,'r') as f:
        for line in f:
            count_total += 1
            tokens = line.split(',')
            athleteID = tokens[0]

            try:
                coordinates = ast.literal_eval(tokens[1].strip()[1:-1].replace(" ",","))
            except:
                print "error: couldn't get coordinates"
                continue

            try:
                lat = coordinates[0][0]
                lng = coordinates[0][1]
            except TypeError:
                lat = coordinates[0]
                lng = coordinates[1]
            
            coords.append((lat,lng))
            ids.append(athleteID)
            
    results = rg.search(coords)
    
    for i in range(len(results)):
        result = results[i]
        athleteID = ids[i]
        
        if result["cc"] == "US":
            count_us += 1
            us_map[result["admin1"]].append(athleteID)
    
    print "Number of global Athletes = ", count_total, "\nNumber of US Athletes = ", count_us, "\n"
    for state in us_map:
	with open(outdir+state+'.txt','w') as f:
            for ID in us_map[state]:
                f.write(str(ID) + '\n')

read_generate(infile)

#for k in range(1,51):
#    input_file=indir+str(k)
#    read_generate(input_file,k)

#target = open(outfil, 'w')
#for key, value in country_map.items():
#    target.write(str(key) + "," + str(value) + "\n")
