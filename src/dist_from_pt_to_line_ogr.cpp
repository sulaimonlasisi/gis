/*
This program calculates the shortest distance from a reference lat/lon point among a sequence of line strings.
Each line string is divided into line segments and the distance is found for each line segment.
Program uses GDAL/OGR to read in files and uses GEOS/OGR to do the computation
This might be inefficient for huge datasets
*/
#include <iostream>
#include "gdal.h"
#include <gdal_priv.h>
#include "ogrsf_frmts.h"
#include <ogr_geometry.h>
#include <limits>
#include <geos.h>
#include <geos/geom/Coordinate.h>
#include <geos/geom/CoordinateArraySequence.h>
#include <geos/geom/CoordinateSequence.h>
#include <geos/geom/LineSegment.h>
#include <sys/time.h>
#include <stdio.h>
using namespace::std;
void print_line_string ( OGRLineString *poLineString );
double project_crash_point_to_line_string ( OGRLineString *poLineString,  OGRPoint *crashPoint, double& shortest_projection);
float timedifference_msec(struct timeval t0, struct timeval t1);
int main()
{

    /*struct RoadSegment
    {
        OGRLineString* seg_line_string;
        string phys_reference;
        string road_name; 
        RoadSegment(){
          seg_line_string; = new OGRLineString();
          phys_reference = "";
          road_name = "";
        }       
    };*/
    struct timeval start;
    struct timeval finish;
    OGRRegisterAll();
    OGRDataSource *poDS;
    const char *path="/home/slasisi/data/getting_started/mile.shp";
    poDS = OGRSFDriverRegistrar::Open( path, FALSE );
    OGRLayer  *poLayer;
    OGRFeature *poFeature;
    std::vector<OGRLineString*> line_str_vector;
    float time_elapsed;
    if( poDS == NULL )
    {
        printf( "Open failed.\n" );
        exit( 1 );
    }
    poLayer = poDS->GetLayerByName("mile");
    poLayer->ResetReading();
    gettimeofday(&start, 0);
    while( (poFeature = poLayer->GetNextFeature()) != NULL )
    {
        //Get a feature, get geometry from the feature and add to lineString until there are
        //5 points in each line string. These line strings will then be used to test for a projection near a crash
        int line_str_ctr = 0;
        OGRLineString *poLineString = new OGRLineString();
        while (line_str_ctr < 5){
          OGRGeometry *poGeometry;          
          if (line_str_ctr > 0){
            if ( (poFeature = poLayer->GetNextFeature()) != NULL){
                poGeometry = poFeature->GetGeometryRef();

            }
          } else poGeometry = poFeature->GetGeometryRef();
          if( poGeometry != NULL && wkbFlatten(poGeometry->getGeometryType()) == wkbPoint )
            {
                OGRPoint *poPoint = (OGRPoint *) poGeometry;
                //OGRPoint temp = OGRPoint(poPoint->getX(), poPoint->getY());
                poLineString->addPoint(poPoint);            
            }
          line_str_ctr++;
        }
        line_str_vector.push_back(poLineString);
        OGRFeature::DestroyFeature( poFeature );
    }
    //Just hard coding a crash reference point to test things for now:
    //MULTIPOINT (-84.992250489972861 41.78786977180274) - This location is on N I-69 from Mile Markers dataset #28
    OGRPoint *crashPoint = new OGRPoint (-84.992250489972861, 41.78786977180274);
    double shortest_projection = numeric_limits<double>::max();
    //for each Linestring in the vector, implement a projection and compare it to the current minimum value
    for (int i = 0; i<line_str_vector.size(); i++){
        cout<< "For the " << i << "th line string,";
      project_crash_point_to_line_string ( line_str_vector[i],  crashPoint, shortest_projection);
    }
    gettimeofday(&finish, 0);
    time_elapsed = timedifference_msec(start, finish);
    cout << "The shortest distance from the crash is " << shortest_projection << " and it took " << time_elapsed << " ms." << endl;
    /*for (int i = 0; i<line_str_vector.size(); i++){
      //project_crash_point_to_line_string ( line_str_vector[i],  crashPoint, shortest_projection);
      print_line_string(line_str_vector[i]);
    }*/

   OGRDataSource::DestroyDataSource( poDS );
}

float timedifference_msec(struct timeval t0, struct timeval t1)
{
    return (t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f;
}


void print_line_string ( OGRLineString *poLineString ) {
    int numPoints = poLineString->getNumPoints();
    cout << "Number of points in this line string is " << numPoints << endl;
    for (int i=0; i<numPoints; i++)
        {
            OGRPoint* poPoint = new OGRPoint();
            poLineString->getPoint(i, poPoint);
            cout << "Linestring's point at vertex " << (i) << ", ptX: " << poPoint->getX() << " ptY: " << poPoint->getY() << endl;
        }
}

double project_crash_point_to_line_string ( OGRLineString *poLineString,  OGRPoint *crashPoint, double& shortest_projection) {
    double current_projection;
    int points [2];
    for (int k = 0; k<poLineString->getNumPoints()-1; k++){
        OGRPoint* poPoint = new OGRPoint(); 
        poLineString->getPoint(k, poPoint);
        Coordinate* c1 = new Coordinate(poPoint->getX(), poPoint->getY());
        poLineString->getPoint(k+1, poPoint);
        Coordinate* c2 = new Coordinate(poPoint->getX(), poPoint->getY());
        LineSegment* ls = new LineSegment (c1->x, c1->y, c2->x, c2->y);
        Coordinate* refCoord = new Coordinate(crashPoint->getX(), crashPoint->getY());
        current_projection = ls->distance(*refCoord);
        if (current_projection < shortest_projection){
          shortest_projection = current_projection;
          points [0] = k;
          points[1] = k+1;
        }
    }
    cout << "the current shortest projection is " << shortest_projection << " formed by the line segment btw points " << points [0]
         << " and " << points [1] << endl;
   
}