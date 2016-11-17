#include <iostream>
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
void print_coord_seq ( CoordinateSequence *poCoordSeq );
double project_crash_point_to_line_segment ( CoordinateSequence *poCoordSeq,  Coordinate* crash_coord, double& shortest_projection);
void convert_pts_to_coords (OGRPoint* crashPoint, Coordinate*& refCoord);
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
    OGRLayer  *poLayer;
    OGRFeature *poFeature;
    std::vector<CoordinateSequence*> *coord_seq_vector = new std::vector<CoordinateSequence*>();
    std::vector<Coordinate> *coord_vector = new std::vector<Coordinate>();
    float time_elapsed;
    poDS = OGRSFDriverRegistrar::Open( "mile.shp", FALSE );
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
        int coord_seq_ctr = 0;
        //OGRLineString *poLineString = new OGRLineString();
        //CoordinateSequence *poCoordSeq = new CoordinateSequence();
        while (coord_seq_ctr < 5){
          OGRGeometry *poGeometry;          
          if (coord_seq_ctr > 0){
            if ( (poFeature = poLayer->GetNextFeature()) != NULL){
                poGeometry = poFeature->GetGeometryRef();
            }
          } else {
              poGeometry = poFeature->GetGeometryRef();
            }
          if( poGeometry != NULL && wkbFlatten(poGeometry->getGeometryType()) == wkbPoint )
            {
                OGRPoint *poPoint = (OGRPoint *) poGeometry; //There might be other ways to read an OGR Geometry object into GEOS
                Coordinate* new_coord = new Coordinate();
                convert_pts_to_coords(poPoint, new_coord);
                coord_vector->push_back(*new_coord);
            }
          coord_seq_ctr++;
        }
        CoordinateSequence *new_coord_seq = new CoordinateArraySequence();
        new_coord_seq->add(coord_vector, true);
        coord_seq_vector->push_back(new_coord_seq);
        coord_vector->clear();
        OGRFeature::DestroyFeature( poFeature );
    }

    //Just hard coding a crash reference point to test things for now:
    //MULTIPOINT (-84.992250489972861 41.78786977180274) - This location is on N I-69 from Mile Markers dataset #28
    OGRPoint *crashPoint = new OGRPoint (-84.992250489972861, 41.78786977180274);
    //cout << "Crash point coords are coordX: " << crashPoint->getX() << " coordY: " << crashPoint->getY() << endl;
    Coordinate* crash_coord = new Coordinate();
    convert_pts_to_coords(crashPoint, crash_coord);
    //cout << "Coords of crash coord are coordX: " << crash_coord->x << " coordY: " << crash_coord->y << endl;
    double shortest_projection = numeric_limits<double>::max();
    //for each CoordinateSequence in the vector, implement a projection using the distance function of its line Segment
    // and compare it to the current minimum value
    for (int i = 0; i<coord_seq_vector->size(); i++){
      cout<< "For the " << i << "th coordinate sequence,";
      project_crash_point_to_line_segment ( coord_seq_vector->at(i),  crash_coord, shortest_projection);
    }
    gettimeofday(&finish, 0);
    time_elapsed = timedifference_msec(start, finish);
    cout << "The shortest distance from the crash is " << shortest_projection << " and it took " << time_elapsed << " ms." << endl;
    /*for (int i = 0; i<coord_seq_vector->size(); i++){
      cout<< "For the " << i << "th coordinate sequence,";
      print_coord_seq ( coord_seq_vector->at(i) );
    }*/
   OGRDataSource::DestroyDataSource( poDS );
}

float timedifference_msec(struct timeval t0, struct timeval t1)
{
    return (t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f;
}

void print_coord_seq ( CoordinateSequence *poCoordSeq ) {
    //int numPoints = poLineString->getNumPoints();
    int num_coords = poCoordSeq->getSize();
    cout << "Number of coords in this coord seq is " << num_coords << endl;
    for (int i=0; i<num_coords; i++)
        {
            //OGRPoint* poPoint = new OGRPoint();
            //poLineString->getPoint(i, poPoint);
            cout << "Coordseq's point at vertex " << (i) << ", coordX: " << poCoordSeq->getAt(i).x << " coordY: " << poCoordSeq->getAt(i).y << endl;
        }
}

double project_crash_point_to_line_segment ( CoordinateSequence *poCoordSeq,  Coordinate* crash_coord, double& shortest_projection){
    double current_projection = numeric_limits<double>::max();;
    int points [2];
    for (int k = 0; k<poCoordSeq->getSize()-1; k++){
        LineSegment* ls = new LineSegment (poCoordSeq->getAt(k), poCoordSeq->getAt(k+1));
        current_projection = ls->distance(*crash_coord);
        if (current_projection < shortest_projection){
          shortest_projection = current_projection;
          points [0] = k;
          points[1] = k+1;
        }
    }

    cout << " size is " << poCoordSeq->getSize() << ", shortest projection is " << shortest_projection << " by line seg btw points " << points [0]
         << " and " << points [1] << endl;   
}

void convert_pts_to_coords (OGRPoint* crashPoint, Coordinate*& refCoord){
  refCoord->x = crashPoint->getX();
  refCoord->y = crashPoint->getY();
}