/*
This program calculates the shortest distance from a reference (crash) lat/lon point among a sequence of line strings (road segments).
Each line string is divided into line segments and the distance is found for each line segment.
The shortest distance from the crash object is visualized on the map as the location of the crash.
Program uses GEOS for GIS calculation
Most recent version of this is locally stored in src/getting_started_get_dist_for_seg_from_db.cpp 
*/
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "gdal.h"
#include <gdal_priv.h>
#include "ogrsf_frmts.h"
#include <ogr_geometry.h>
#include <limits>
#include <geos.h>
#include <geos/geom/LineSegment.h>
#include <geos/geom/CoordinateArraySequence.h>
#include <sys/time.h>
#include <stdio.h>
using namespace::std;


struct RoadSegment
{
  /*
  A road seg contains PR, BPT, FMWK_YR, shortest_distance_from_crash and the CoordinateSequence holding its line segments
  */
  int pr;
  int bpt;
  string fmwk_year;
  double shortest_distance_from_crash;
  CoordinateSequence* line_string;
};


struct Framework {
  /*Framework contains framework year, the number of road segments it holds, the index of road_segment with shortest distance
  , the shortest distance of the framework from the crash and a vector holding the road segments*/
  string fmwk_year;
  int num_road_segments;
  int idx_road_seg_shortest_dist;
  std::vector<RoadSegment> road_seg_vector;
  double shortest_distance_from_crash;
};

struct Crash {
/*Crash object contains crash info, a vector of frameworks (usually 2), the nearest road segment to it, a boolean variable
indicating whether its distance has changed since the crash occured (not yet implemented). */
  int crash_id;
  string crash_year;
  string current_year;
  Coordinate crash_coordinate;
  double distance_from_closest_segment;
  std::vector<Framework> fmwk_vector;
  RoadSegment nearest_road_segment;
  bool same_distance_as_crash_year;  //not used currently but might be useful for future analysis
};


void output_crash_pts_to_file(std::vector<Coordinate>& , std::vector<Coordinate>& , std::vector<Coordinate>& );
void print_coord_seq ( CoordinateSequence *poCoordSeq ); //prints a coordinate sequence
float timedifference_msec(struct timeval t0, struct timeval t1); //timer function
Crash get_single_crash(string input_line, int start, int end); //parses a single crash line and gets a single crash/fmwk from a line
Coordinate get_coord_from_str_stream(string coord_string); //parses coordinates from a string -- used for linestring
void parse_fmwk_from_crash_line(Framework& fmwk_obj, string input_line); //parses framework information from crash information
void get_coord_seq_from_line_string(RoadSegment& rs, string line_segment_string); //parses a linestring and converts it to a coord sequence
std::vector<Crash> get_crash_data(); //gets crash points from a file, calls helper functions and saves them in a vector
void shortest_dist_from_crash_to_fmwk (Framework& fmwk_obj,  Coordinate crash_coord); //Minimum distance calculator
void get_nearest_road_segment(Crash& crash_obj); //calls helper functions to calculate segment with minimum distance

int main()
{

    cout.precision(8);
    struct timeval start;
    struct timeval finish;
    std::vector<Coordinate> crash_pts_ne_vector;
    std::vector<Coordinate> crash_pts_sw_vector;
    std::vector<CoordinateSequence*> *coord_seq_vector = new std::vector<CoordinateSequence*>();
    std::vector<Coordinate> *coord_vector = new std::vector<Coordinate>();
    std::vector<Coordinate> crash_pts_vector;
    std::vector<Crash> crash_vector;

    float time_elapsed;
    gettimeofday(&start, 0);
      crash_vector = get_crash_data();
      cout << "The size of the crash vector is " << crash_vector.size() << endl;


    
}


//Timer function
float timedifference_msec(struct timeval t0, struct timeval t1)
{
    return (t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f;
}


//Prints a coordinate sequence
void print_coord_seq ( CoordinateSequence *poCoordSeq ) {
  /*
    Prints a coordinate sequence which is the sequence of points that make up a line (or a road segment)
  */
    int num_coords = poCoordSeq->getSize();
    cout << "Number of coords in this coord seq is " << num_coords << endl;
    for (int i=0; i<num_coords; i++)
      {
        cout << "Coordseq's point at vertex " << (i) << ", coordX: " << poCoordSeq->getAt(i).x << " coordY: " << poCoordSeq->getAt(i).y << endl;
      }
}

void get_nearest_road_segment(Crash& crash_obj){
  /*
    For each crash, calculate the nearest road segment by iterating through all the frameworks and finding the framework
    with the nearest road segment. This framework is identified and its nearest road segment is accessed by its index
  */
  int shortest_fmwk_idx = -1*(numeric_limits<double>::max());
  crash_obj.distance_from_closest_segment = numeric_limits<double>::max();
  cout << "Crash id " << crash_obj.crash_id << endl;
  for (int j = 0; j < crash_obj.fmwk_vector.size(); j++){   
    shortest_dist_from_crash_to_fmwk(crash_obj.fmwk_vector.at(j), crash_obj.crash_coordinate);
    if (crash_obj.fmwk_vector.at(j).shortest_distance_from_crash < crash_obj.distance_from_closest_segment){
      shortest_fmwk_idx = j;
    }
  }
  int index_of_nearest_road_seg = crash_obj.fmwk_vector.at(shortest_fmwk_idx).idx_road_seg_shortest_dist;
  crash_obj.nearest_road_segment = crash_obj.fmwk_vector.at(shortest_fmwk_idx).road_seg_vector.at(index_of_nearest_road_seg);
  cout << "Nearest road_seg for crash " << crash_obj.crash_id << " from crash year " << crash_obj.crash_year <<
  " has PR, BPT, framework year and distance " << crash_obj.nearest_road_segment.pr << ", " <<
  crash_obj.nearest_road_segment.bpt << ", " <<  crash_obj.nearest_road_segment.fmwk_year << ", "
  << crash_obj.nearest_road_segment.shortest_distance_from_crash << endl;
}

void shortest_dist_from_crash_to_fmwk ( Framework& fmwk_obj,  Coordinate crash_coord){
  /*
    //Minimum distance calculator
    Takes  framework (which contains one or more road segments) and iterates through to find the shortest distance within each
    segment. The framework compares the shortest distance across segments and returns the overall shortest distance from the crash
  */
    fmwk_obj.idx_road_seg_shortest_dist = -1*(numeric_limits<double>::max());
    fmwk_obj.shortest_distance_from_crash = numeric_limits<double>::max();
    double current_projection = numeric_limits<double>::max();
    for (int i = 0; i < fmwk_obj.num_road_segments; i++){
      fmwk_obj.road_seg_vector.at(i).shortest_distance_from_crash = numeric_limits<double>::max();
      for (int k = 0; k<fmwk_obj.road_seg_vector.at(i).line_string->getSize()-1; k++){
        LineSegment* ls = new LineSegment (fmwk_obj.road_seg_vector.at(i).line_string->getAt(k), fmwk_obj.road_seg_vector.at(i).line_string->getAt(k+1));
        current_projection = ls->distance(crash_coord);
        //Lines below are for debugging
        cout << fmwk_obj.road_seg_vector.at(i).pr << '\t' << fmwk_obj.road_seg_vector.at(i).bpt << '\t' << fmwk_obj.road_seg_vector.at(i).fmwk_year 
        << '\t' << "Shortest distance from line segment: " << current_projection  << endl;
        //Compares current projection from line segment to the current road segment being examined
        if (current_projection < fmwk_obj.road_seg_vector.at(i).shortest_distance_from_crash){
          fmwk_obj.road_seg_vector.at(i).shortest_distance_from_crash = current_projection;
        }
        //Compares current projection from line segment to the current framework
        if (current_projection < fmwk_obj.shortest_distance_from_crash){
          fmwk_obj.shortest_distance_from_crash = current_projection;
          fmwk_obj.idx_road_seg_shortest_dist = i;          
          //More analysis could be done here by getting the exact GPS coordinates of the lineSegment that is the shortest
          //and plotting that on the map so that the visual appearance matches the calculated value
        }
      }
    }
}



//function similar to get_all_crash_points but is tailored to the data format retrieved from the database for a crash
std::vector<Crash> get_crash_data(){
  /*
  Gets crash points from a file, calls helper functions to process each line and save all the crashes in
  a vector which is returned upon completion.
  */
  std::vector<Crash> crash_vector; 
  string current_year = "2015"; 
  int start = 0;
  int end = 0;
  const char* crash_file_path="/home/slasisi/data/2004_2015/test_algo_file.txt";
  std::ifstream crash_input_stream;
  std::string input_line;
  crash_input_stream.open (crash_file_path);
  if( crash_input_stream.fail() )
  {
      std::cout << "Could not open " << crash_file_path << endl;
      exit( 1 );
  }
  while (std::getline(crash_input_stream, input_line))
  {
    Crash crash_obj = Crash();
    crash_obj =  get_single_crash(input_line, start, end);    
    if (crash_obj.crash_year != current_year){
      std::getline(crash_input_stream, input_line);
      Framework fmwk_obj = Framework();
      parse_fmwk_from_crash_line(fmwk_obj, input_line);
      crash_obj.fmwk_vector.push_back(fmwk_obj);
    }
    crash_vector.push_back(crash_obj);
    get_nearest_road_segment(crash_obj);
  }
  crash_input_stream.close();
  return crash_vector;
}



Crash get_single_crash(string input_line, int start, int end){
  /*
    parses a single crash line and gets a single framework
    Used alone to get crash information if the crash used is in current year
  */
  Crash crash_obj = Crash();
  string current_year = "2015";
  crash_obj.current_year = current_year;
  Framework fmwk_obj = Framework();
  char delimeter = '|';
  int delimeter_counter = 0;
  int col_span;
  char * pEnd;
  int decimal_base = 10;
  //Get crash_id, crash_year, fmwk_year, num_road_segments in framework and crash coordinates
  while (delimeter_counter < 5){        
    end = input_line.find(delimeter, start);
    if (delimeter_counter != 0){
      col_span = end - start;
    }
    if (delimeter_counter == 0){
      //gets crash_id, starts at beginning of line until delimeter    
      col_span = end - start;  
      crash_obj.crash_id = strtol(((input_line.substr (start,col_span)).c_str()),&pEnd, decimal_base);
    }
    else if (delimeter_counter == 1){
      //gets crash year with preceding c
      crash_obj.crash_year = input_line.substr (start+1,col_span-1); 
    }
    else if (delimeter_counter == 2){
      parse_fmwk_from_crash_line(fmwk_obj, input_line);
    }
    else if (delimeter_counter == 4){
      //gets coordinate of crash
      double crash_coord_x, crash_coord_y;
      crash_coord_x = strtod(((input_line.substr (start,col_span)).c_str()),0); 
      start = end+1;
      end = input_line.find(delimeter, start);
      col_span= end - start;
      crash_coord_y = strtod(((input_line.substr (start,col_span)).c_str()),0);
      crash_obj.crash_coordinate.x = crash_coord_x;
      crash_obj.crash_coordinate.y = crash_coord_y;
    }
    start = end+1;
    delimeter_counter++;
  }
  crash_obj.fmwk_vector.push_back(fmwk_obj);
  return crash_obj;
}

void parse_fmwk_from_crash_line(Framework& fmwk_obj, string input_line){
  /*
    parses framework information from crash information
    Can be used from the get_single_crash_function or as stand-alone when getting the second
    framework object of a crash that did not occur in the current year.
  */
  string::size_type start = 0, end = 0;
  char delimeter = '|';
  int dlmt_counter = 0;
  int col_span;
  char * pEnd;
  int decimal_base = 10;
  while (dlmt_counter < 2){
    end = input_line.find(delimeter, start);
    start = end+1;
    dlmt_counter++;
  }  
  while (dlmt_counter < 4){        
    end = input_line.find(delimeter, start);
    col_span = end - (start);    
    if (dlmt_counter == 2){
      //gets framework year with preceding f
      fmwk_obj.fmwk_year = input_line.substr (start+1,col_span-1); 
    }
    else if (dlmt_counter == 3){
      //gets number of road segments in framework
      fmwk_obj.num_road_segments = strtol(((input_line.substr (start,col_span)).c_str()),&pEnd, decimal_base);
    }
    start = end+1;
    dlmt_counter++;
  }
  //Advance the end pointer twice
  dlmt_counter = 0;
  while (dlmt_counter < 2){
    end = input_line.find(delimeter, start);
    start = end+1;
    dlmt_counter++;
  }

  //Get Road Segment object
  for (int seg_counter = 0; seg_counter< fmwk_obj.num_road_segments; seg_counter++){
    RoadSegment rs = RoadSegment();
    rs.fmwk_year = fmwk_obj.fmwk_year;
    //Get Road segment PR
    int rs_counter = 0;
    while (rs_counter < 3){
      end = input_line.find(delimeter, start);
      col_span = end - start;
      if (rs_counter == 0){
        rs.pr = strtol(((input_line.substr (start,col_span)).c_str()),&pEnd, decimal_base);
      }
      else if (rs_counter == 1){
        rs.bpt = strtol(((input_line.substr (start,col_span)).c_str()),&pEnd, decimal_base);
      }
      else {
        //Parsed out "|LINESTRING" and the characters at the end of the line segments.
        string curr_line_string = input_line.substr (start+11,(col_span-12));
        get_coord_seq_from_line_string(rs, curr_line_string);
      }
      start = end+1;
      rs_counter++;
    }
    fmwk_obj.road_seg_vector.push_back(rs);
    start = end+1;
  }

}


void output_crash_pts_to_file(std::vector<Coordinate>& crash_pts_vector, std::vector<Coordinate>& crash_pts_ne_vector, std::vector<Coordinate>& crash_pts_sw_vector){
  
  const char* crash_pts_ne_sw_path="crash_ne_sw.txt";
  std::ofstream crash_ne_sw_output_stream;
  crash_ne_sw_output_stream.precision(12);
  crash_ne_sw_output_stream.open (crash_pts_ne_sw_path);
  for (int i = 0; i < crash_pts_vector.size(); i++){
    crash_ne_sw_output_stream << "Crash points coords: " <<crash_pts_vector.at(i).x << " " << crash_pts_vector.at(i).y 
        << " NE Crash points coords: " <<crash_pts_ne_vector.at(i).x << " " << crash_pts_ne_vector.at(i).y 
        << " SW Crash points coords: " <<crash_pts_sw_vector.at(i).x << " " << crash_pts_sw_vector.at(i).y << endl;
  }
  crash_ne_sw_output_stream.close();
}

void get_coord_seq_from_line_string(RoadSegment& rs, string line_segment_string){
  /*
    Takes a string representing a linestring and parses it so it becomes a usable 
    Coordinate sequence for a road segment object.
  */
  char comma_delimeter = ',';
  string::size_type start = 0, end = 0;
  rs.line_string = new CoordinateArraySequence();
  std::vector<Coordinate> *coord_vector = new std::vector<Coordinate>();
  end = line_segment_string.find(comma_delimeter, start);
  int coord_span;
  while (end != string::npos){
    coord_span = end - start;
    string coord_string = line_segment_string.substr(start, coord_span);
    Coordinate new_coord = get_coord_from_str_stream(coord_string);
    coord_vector->push_back(new_coord);
    start = end+1;
    end = line_segment_string.find(comma_delimeter, start);
  }
  string coord_string = line_segment_string.substr(start);
  Coordinate new_coord = get_coord_from_str_stream(coord_string);
  coord_vector->push_back(new_coord);
  rs.line_string->add(coord_vector, true);
}

Coordinate get_coord_from_str_stream(string coord_string){
  /*
  parses coordinates from a string -- used for linestring parsing
  */
  std::stringstream ss;
  ss << coord_string;
  double x_coord, y_coord;
  Coordinate new_coord = Coordinate();
  while (ss.good()){
    string x_string, y_string;
    ss >> x_string >> y_string;
    x_coord = strtod (x_string.c_str(),0);
    y_coord = strtod (y_string.c_str(),0);
  }    
  new_coord.x = x_coord;
  new_coord.y = y_coord;
  return new_coord;
}
  
