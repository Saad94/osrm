#include <iostream>
#include <math.h>
#include <vector>
#include <set>
#include <dirent.h>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
//#include <omp.h>
#include <unordered_map>
#include <unordered_set>
#include <sys/time.h>

#include "commons.cpp"

using namespace std;

#define EARTH_RADIUS_METERS 6371000
#define PI 3.141592653589793
#define DIST_SIM_THRESHOLD_METERS_HOME 50
#define NONEPZ_RADIUS_INIT_METERS 402.336
#define MILE 0.000621371

double Radii[5] ={201.168, 402.336, 603.504, 804.672, 1005.84};
double convertMetersToMiles(double input){ return MILE * input; }

class Point {
public:
  double lat, lon;
  Point() {lat = 0.0; lon = 0.0;}
  Point(double _lat, double _lon) {lat = _lat; lon = _lon;}
  bool operator==(const Point & rhs) const {return this->lat == rhs.lat && this->lon == rhs.lon;}
  string toString() {return "" + to_string(lat) + "," + to_string(lon);}
  void scale(double factor) {lat *= factor; lon *= factor;}
};

namespace std
{
  template<>
  struct hash<Point>
  {
    size_t
    operator()(const Point & obj) const
    {
      return hash<int>()(obj.lat+obj.lon);
    }
  };
}

class Center {
public:
  Point p;
  int confidence;
  double r;
  double pointsDist;
  unordered_set<Point> edgePoints;
  
  Center() {confidence = 0; r = 0.0;}
  Center(Point _p, int _confidence, double _r) {p = _p; confidence = _confidence; r = _r;}
  Center(Point _p, int _confidence, double _r, double _pointsDist) {p = _p; confidence = _confidence; r = _r; pointsDist = _pointsDist;}
  Center(Point _p, int _confidence, double _r, double _pointsDist, Point _p1, Point _p2) {
      p = _p; confidence = _confidence; r = _r; pointsDist = _pointsDist; edgePoints.insert(_p1); edgePoints.insert(_p2);
  }
  Center(Point _p, int _confidence, double _r, double _pointsDist, unordered_set<Point> _edgePoints) {
      p = _p; confidence = _confidence; r = _r; pointsDist = _pointsDist; edgePoints.insert(_edgePoints.begin(), _edgePoints.end());
  }
  string toString() {return "" + to_string(confidence) + "," + to_string(convertMetersToMiles(r)) + "," + p.toString();}
};

typedef vector<Point> Route;

ostream& operator<<(ostream& os, const Point& p) {os << "(" << p.lat << ", " << p.lon << ")"; return os;}
ostream& operator<<(ostream& os, const Center& c) {os << c.r << " - " << c.confidence << " - " << c.p; return os;}
ostream& operator<<(ostream& os, const vector<string> v) {for (string s : v) os << s << ", "; return os;}
ostream& operator<<(ostream& os, const Route v) {for (Point p : v) os << p << ", "; return os;}
ostream& operator<<(ostream& os, const vector<Route> v) {for (Route r : v) os << r << "\n"; return os;}
double radians(double deg) {return deg * PI / 180.0;}
double degrees(double rad) {return rad * 180.0 / PI;}
double length(double x, double y) {return sqrt(x*x + y*y);}
double length(Point one) {return length(one.lon, one.lat);}
double length(Point one, Point two) {return length(one.lon - two.lon, one.lat - two.lat);}

Center merge(Center one, Center two) {
  unordered_set<Point> tmp;
  tmp.insert(one.edgePoints.begin(),one.edgePoints.end());
  tmp.insert(two.edgePoints.begin(),two.edgePoints.end());
  return Center(Point((one.p.lat+two.p.lat)/2,(one.p.lon+two.p.lon)/2),one.confidence+two.confidence, one.r, 0.0, tmp);
}

inline bool fileExists(string folder, string filename) {
  string path = folder + "/" + filename;
  ifstream file(path);
  if (file.is_open()) {
    file.close();
    return true;
  }
  return false;
}

/*
 * Read list of files from a single directory.
 * Add to a vector.
 */
void readFilesFromDirectory(string dirname, vector<string> & filenames,
          bool limitFiles, unordered_map<string,vector<Route>> & EPZ_Map) {
  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir (dirname.c_str())) != NULL) {
    while ((ent = readdir (dir)) != NULL) {
      if (ent->d_name[0] != '.') {
        if (limitFiles) {
          /*
           * If file is not an EPZ one, add it
           */
          if (EPZ_Map.find(ent->d_name) == EPZ_Map.end()) {
            filenames.push_back(dirname + "/" + ent->d_name);
          }
        } else {
          filenames.push_back(dirname + "/" + ent->d_name);
        }
      }
    }
    closedir (dir);
  } else {
    cout << "Couldn't open directory: " << dirname << endl;
    exit(0);
  }
}

/*
 * Read list of files from a single directory.
 * Add to an unordered_set.
 */
void readFilesFromDirectory(string dirname, unordered_set<string> & filenames,
          bool limitFiles, unordered_map<string,vector<Route>> & EPZ_Map) {
  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir (dirname.c_str())) != NULL) {
    while ((ent = readdir (dir)) != NULL) {
      if (ent->d_name[0] != '.') {
        if (limitFiles) {
          /*
           * If file is not an EPZ one, add it
           */
          if (EPZ_Map.find(ent->d_name) == EPZ_Map.end()) {
            filenames.insert(dirname + "/" + ent->d_name);
          }
        } else {
          filenames.insert(dirname + "/" + ent->d_name);
        }
      }
    }
    closedir (dir);
  } else {
    cout << "Couldn't open directory: " << dirname << endl;
    exit(0);
  }
}

// Check that if the route already exist in the vector.
// This is faster than vector class exists/compare function.
// We don't need to check whole route.
bool checkRouteAlreadyExist(vector<Route> & routes, vector<Point> route){
  for (int i = 0; i < routes.size(); i++) {
    Route curr_route = routes[i];
    if (curr_route[0].lat == route[0].lat && curr_route[0].lon == route[0].lon && curr_route[curr_route.size() - 1].lat == route[route.size() - 1].lat){
      return true;
    }
   }
  return false;
}

/*
 * Read route information from a string
 */
Route getRouteFromString(string line) {
  Route route;
  stringstream str = prepareLineForParsing(line);
  double lon, lat;
  while (str >> lat) {
    str >> lon;
    route.push_back(Point(lat, lon));
  }

  return route;
}

/*
 * Read route information from a specified file
 */
void getRoutesFromFile(string path, vector<Route> & routes) {
  ifstream file(path);
  string line;
  if (file.is_open()) {
    while (getline(file, line)) {
      Route route = getRouteFromString(line);

      if (checkRouteAlreadyExist(routes, route)){
	    // cout << "culprit "<<path << endl;
      } else{
	    routes.push_back(route);
      }
      route.clear();
    }
    file.close();
  } else {
    cout << "Couldn't open file: " << path << "\n";
  }
}

/*
 * Read route information from a specified file
 * Prune those routes which are known not to have an EPZ
 */
void getRoutesFromFileEPZ(string folder, string filename, vector<Route> & routes, unordered_map<string,vector<Route>> & EPZ_Map) {
  string path = folder + "/" + filename;
  getRoutesFromFile(path, routes);
  for (int i = 0; i < routes.size(); i++) {
    Route route = routes[i];
    vector<Route> EPZ_routes = EPZ_Map[filename];
    bool keep = false;
    for (Route EPZ_route : EPZ_routes) {
      if (route[0] == EPZ_route[0] && route[route.size()-1] == EPZ_route[EPZ_route.size()-1]) {
        keep = true;
        break;
      }
    }
    if (keep == false) {
      routes.erase(routes.begin()+i);
      i--;
    }
  }
}

/*
 * Read information about which user's which routes are known to have EPZs.
 * Add information to a map.
 */
void readEPZMapFile(string EPZ_Map_filename, vector<string> &EPZ_filenames, unordered_map<string,vector<Route>> &EPZ_Map){
    ifstream file(EPZ_Map_filename);
    string line;
    if (file.is_open()) {
      while (getline(file, line)) {
        line.erase(remove(line.begin(), line.end(), '['), line.end());
        line.erase(remove(line.begin(), line.end(), ']'), line.end());
        replace(line.begin(), line.end(), ',', ' ');
        stringstream str(line);
        string filename;
        double lon, lat;
        Route route;
        str >> filename >> lat >> lon;
        route.push_back(Point(lat, lon));
        str >> lat >> lon;
        route.push_back(Point(lat, lon));
        filename += ".txt";
        if (EPZ_Map.find(filename) == EPZ_Map.end()) {
          EPZ_filenames.push_back(filename);
          vector<Route> routes;
          routes.push_back(route);
          EPZ_Map[filename] = routes;
        } else {
          EPZ_Map[filename].push_back(route);
        }
      }
      file.close();
    } else {
      cout << "ERROR: COULDN'T OPEN EPZ_MAP_FILE NAMED " << string(EPZ_Map_filename) << "\n";
      exit(0);
    }
}

double dist(Point one, Point two) {
  double dlon = two.lon - one.lon;
  double dlat = two.lat - one.lat;
  double a = pow(sin(radians(dlat/2)), 2) + cos(radians(one.lat)) * cos(radians(two.lat)) * pow(sin(radians(dlon/2)), 2);
  double c = 2 * atan2(sqrt(a), sqrt(1-a));
  double d = EARTH_RADIUS_METERS * c;
  return d;
}

Point convertLatLonToXY(Point one, double avgLat) {
  /*
   * X = lat, Y = lon
   */
  Point p;
  p.lat = radians(EARTH_RADIUS_METERS * one.lat);
  p.lon = radians(EARTH_RADIUS_METERS * one.lon) * cos(radians(avgLat));
  return p;
}

Point convertXYToLatLon(Point one, double avgLat) {
  /*
   * X = lat, Y = lon
   */
  Point p;
  p.lat = degrees(one.lat / EARTH_RADIUS_METERS);
  p.lon = degrees(one.lon / (EARTH_RADIUS_METERS * cos(radians(avgLat))));
  return p;
}

/*
 * PROCESSING FUNCTIONS
 */

// function to sort centers by their confidence
void centersort(vector<Center> & centers) {
  for (int i = 0; i < centers.size(); i++) {
    int j = i;
    while (j > 0 && centers[j].confidence > centers[j-1].confidence) {
      Center tmp = centers[j];
      centers[j] = centers[j-1];
      centers[j-1] = tmp;
      j--;
    }
  }
}

// Combine/circles merge that are less than threshold values.
// Different modes have different threshold values.
void prune(vector<Center> & centers, double threshold) {
  for (int i = 0; i < centers.size(); i++) {
    for (int j = i+1; j < centers.size(); j++) {
      if (centers[i].r == centers[j].r) {
        if (dist(centers[i].p, centers[j].p) <= threshold) {
          centers[i] = merge(centers[i], centers[j]);
          centers.erase(centers.begin()+j);
          j--;
        }
      }
    }
  }
}

/*
 * Loop over all routes and
 * find the routes which have common "home" point
 * r will be home's EPZ radius
 */
Center findHomeLocation(vector<Route> & routes, double r) {
  vector<Center> centers;
  vector<Route> prunedRoutes;

  for (Route route : routes) {
    centers.push_back(Center(route[0], 1, r));
    centers.push_back(Center(route[route.size()-1], 1, r));
  }

  prune(centers, DIST_SIM_THRESHOLD_METERS_HOME);
  
  centersort(centers);
  Center home = centers[0];
  for (int i = 0; i < routes.size(); i++) {
    Route route = routes[i];
    if (dist(home.p, route[0]) <= DIST_SIM_THRESHOLD_METERS_HOME || dist(home.p, route[route.size()-1]) <= DIST_SIM_THRESHOLD_METERS_HOME) {
      prunedRoutes.push_back(route);
    }
  }
  routes = prunedRoutes;
  return home;
}

/*
 * Prune home epz radius (home.r) from all the routes
 * And add point on the boundary of EPZ
 */
vector<Route> pruneAndAddBoundaryPoints(vector<Route> routes, Center home) {
  for (int i = 0; i < routes.size() ; i++) {
    Route & route = routes[i];
    bool addToStart = false;
    bool addToEnd = false;
    Point boundaryPointStart;
    Point boundaryPointEnd;

    /*
     * Remove all points inside the EPZ
     * Check if we need to prune from start point else remove from end point
     */
    
    // Removing Start point
    if (dist(home.p, route[0]) <= home.r) {
      vector<Point>::iterator rIter;
      for (rIter=route.begin(); rIter<route.end(); ) {
        Point p = *rIter;
    	bool rErased = false;
        if (dist(home.p, p) <= home.r) {
	      rIter = route.erase(rIter);
	      rErased = true;
	    } else {
	      break;
	    }
	    if (!rErased)
	      ++rIter;
      }
      addToStart = true;
      boundaryPointStart = route[0];
    }

    // Removing End point
    if (dist(home.p, route[route.size()-1]) <= home.r){
      for (int j = route.size()-1; j >= 0; j--) {
        Point p = route[j];
        if (dist(home.p, p) <= home.r) {
          route.erase(route.begin()+j);
        } else {
	      break;
	    }
      }
      addToEnd = true;
      boundaryPointEnd = route[route.size()-1];
    }

    // Removing middle point
    vector<Point>::iterator rIter;
    for (rIter=route.begin(); rIter<route.end(); ) {
      Point p = *rIter;
      bool rErased = false;
      if (dist(home.p, p) <= home.r) {
	    rIter = route.erase(rIter);
	    rErased = true;
      }
      if (!rErased)
	    ++rIter;
    }
    
    /*
     * Check for empty routes
     */
    if (route.size() == 0) {
      routes.erase(routes.begin()+i);
      i--;
      continue;
    }

    /*
     * Add a point on the boundary of the EPZ
     */
    if (addToStart) {
      double avgLat    = home.p.lat;
      Point XYPoint    = convertLatLonToXY(boundaryPointStart, avgLat);
      Point XYHome     = convertLatLonToXY(home.p, avgLat);
      Point XYDir      = Point(XYPoint.lat - XYHome.lat, XYPoint.lon - XYHome.lon);
      double angle     = atan2(XYDir.lon, XYDir.lat);
      double X         = home.r * cos(angle);
      double Y         = home.r * sin(angle);
      XYPoint.lon      = XYHome.lon + Y;
      XYPoint.lat      = XYHome.lat + X;
      Point newPoint   = convertXYToLatLon(XYPoint, avgLat);
      route.insert(route.begin(), newPoint);
    }

    if (addToEnd){
      double avgLat    = home.p.lat;
      Point XYPoint    = convertLatLonToXY(boundaryPointEnd, avgLat);
      Point XYHome     = convertLatLonToXY(home.p, avgLat);
      Point XYDir      = Point(XYPoint.lat - XYHome.lat, XYPoint.lon - XYHome.lon);
      double angle     = atan2(XYDir.lon, XYDir.lat);
      double X         = home.r * cos(angle);
      double Y         = home.r * sin(angle);
      XYPoint.lon      = XYHome.lon + Y;
      XYPoint.lat      = XYHome.lat + X;
      Point newPoint   = convertXYToLatLon(XYPoint, avgLat);
      route.push_back(newPoint);
    }
  }

  return routes;
}

void doStuff(vector<Route> & routes, double r) {
  Center home = findHomeLocation(routes, r);
  pruneAndAddBoundaryPoints(routes, home);
}


//int main(int argc, char** argv) {
  //if (argc < 3 || argc > 7) {
    //cout << "Usage: ./EPZ.out INPUTDIR [INPUTDIR2] OUTPUTDIR [[EPZ | NOEPZ | MODIFYRADIUS | SHIFTRADIUS | FUZZWITHINTER | FUZZWITHEPZ] [EPZ_MAP_FILE]]\n";
    //return 0;
  //}
  //string mode, inputdir1, inputdir2, outputdir, outputdir_for_map, EPZ_Map_filename;
  //mode = inputdir1 = inputdir2 = outputdir = outputdir_for_map = EPZ_Map_filename = "";
  //if (argc == 3) {
    //inputdir1 = argv[1];
    //outputdir = argv[2];
  //} else if (argc == 4) {
    //inputdir1 = argv[1];
    //inputdir2 = argv[2];
    //outputdir = argv[3];
  //} else if (argc == 5) {
    //inputdir1 = argv[1];
    //outputdir = argv[2];
    //mode = argv[3];
    //EPZ_Map_filename = argv[4];
  //} else if (argc == 6) {
    //inputdir1 = argv[1];
    //inputdir2 = argv[2];
    //outputdir = argv[3];
    //mode = argv[4];
    //EPZ_Map_filename = argv[5];
  //} else if (argc >= 7) {
    //inputdir1 = argv[1];
    //inputdir2 = argv[2];
    //outputdir = argv[3];
    //outputdir_for_map = argv[4];
    //mode = argv[5];
    //EPZ_Map_filename = argv[6];
  //}

  //if (mode != "EPZ" && mode != "NOEPZ" && mode != "MODIFYRADIUS" &&
      //mode != "SHIFTRADIUS" && mode != "FUZZWITHINTER" && mode != "FUZZWITHEPZ"){
    //mode = "DEFAULT";}
  //cout << fixed << setprecision(6);
  //unordered_set<string> filenames;
  //vector<string> filenames_vector,EPZ_filenames,FilesAlreadyProcessed;
  //Center NoEPZHome;
  //unordered_map<string,vector<Route>> EPZ_Map;
  //filenames.reserve(2000000);
  //EPZ_Map.reserve(1000000);
  //FilesAlreadyProcessed.reserve(500000);
  
  //if (mode != "DEFAULT") {
    //readEPZMapFile(EPZ_Map_filename, EPZ_filenames, EPZ_Map);
    //if (mode == "EPZ") {
      //cout << "EPZ MODE: READING EPZ FILES IN INPUTDIR\n";
      //filenames.insert(EPZ_filenames.begin(), EPZ_filenames.end());
    //} else {
      //cout << mode << " MODE: READING NON-EPZ FILES IN INPUTDIR\n\n";
      //readFilesFromDirectory(inputdir1, filenames, true, EPZ_Map);
      //int tmp = filenames.size();
      //cout << "NumFiles " << inputdir1 << " = " << tmp << "\n";

      //if (inputdir2 != "") {
        //readFilesFromDirectory(inputdir2, filenames, true, EPZ_Map);
        //cout << "NumFiles " << inputdir2 << " = " << filenames.size()-tmp << "\n";
      //}
    //}
    //EPZ_filenames.clear();
  //} else {
    //cout << "DEFAULT MODE: READING ALL FILES IN INPUTDIR\n\n";
    //readFilesFromDirectory(inputdir1, filenames, false, EPZ_Map);
    //int tmp = filenames.size();
    //cout << "NumFiles " << inputdir1 << " = " << tmp << "\n";
    //if (inputdir2 != "") {
      //readFilesFromDirectory(inputdir2, filenames, false, EPZ_Map);
      //cout << "NumFiles " << inputdir2 << " = " << filenames.size()-tmp << "\n";
    //}
  //}

  //int numFiles = filenames.size();
  //cout << "Total num of files = " << numFiles << "\n";
  //readFilesFromDirectory(outputdir, FilesAlreadyProcessed, false, EPZ_Map);
  //cout << "Files already processed = " << FilesAlreadyProcessed.size() << "\n";
  /*
   * Check if user already done
   */
  //for (string filename : FilesAlreadyProcessed) {
    //filename = filename.substr(filename.rfind("/")+1);
    //if (mode == "NOEPZ") {
      //filenames.erase(inputdir1 + "/" + filename + ".txt");
      //filenames.erase(inputdir2 + "/" + filename + ".txt");
    //} else if (mode == "MODIFYRADIUS" || mode == "SHIFTRADIUS" || mode == "FUZZWITHINTER"){
      //filename = filename.substr(0,filename.rfind("_"));
      //filenames.erase(inputdir1 + "/" + filename + ".txt");
      //filenames.erase(inputdir2 + "/" + filename + ".txt");
    //} else {
      //filenames.erase(filename + ".txt");
    //}
  //}
  //numFiles = filenames.size();
  //cout << "Actual num of files = " << numFiles << "\n\n";
  //filenames_vector.reserve(numFiles);
  //for (auto it = filenames.begin(); it != filenames.end(); ++it) {filenames_vector.push_back(*(it));}
  //filenames.clear();
  

  //// MAIN LOOP //////////////////////////////////////////////////////////////
//#pragma omp parallel for num_threads(64) schedule(dynamic, 10) //shared(readfilestime, findcenterstime, writefilestime)
  //for (int i = 0; i < numFiles; i++) {
    //// struct timeval atts, attf;
    //// gettimeofday(&atts,NULL);

    //string filename = filenames_vector[i];
    //// cout << "\n New User-------------- " << filename<< endl;
    //int id = omp_get_thread_num();
    //vector<Route> routes;
    //if (mode == "EPZ") {
      //if (fileExists(inputdir1, filename)) {
        //getRoutesFromFileEPZ(inputdir1, filename, routes, EPZ_Map);
      //} else if (fileExists(inputdir2, filename)) {
        //getRoutesFromFileEPZ(inputdir2, filename, routes, EPZ_Map);
      //} else {
        //cout << "Couldn't open file: " << filename << "\n";
      //}
    //} else {
      //getRoutesFromFile(filename, routes);
    //}
    //if (mode == "MODIFYRADIUS" || mode == "SHIFTRADIUS" || mode == "FUZZWITHINTER" || mode == "FUZZWITHEPZ") {
      //for (int i = 0; i < 5; i++) {
      //// for (double r = 0.125; r < 0.7; r += 0.125) {
	//if (mode == "FUZZWITHEPZ"){
	  //for (int c = 0; c <= 20; c++){
		//vector<Route> tmpRoutes = routes;
		//doStuff(tmpRoutes, mode, outputdir,outputdir_for_map, filename, Radii[i],id, c);
	  //}
	//}else{
	  //vector<Route> tmpRoutes = routes;
	  //doStuff(tmpRoutes, mode, outputdir,outputdir_for_map, filename, Radii[i],id,0);
	//}
      //}
    //} else {
      //doStuff(routes, mode, outputdir, outputdir_for_map, filename, NONEPZ_RADIUS_INIT_METERS,id,0);
    //}
  //}
  //return 0;
//}
