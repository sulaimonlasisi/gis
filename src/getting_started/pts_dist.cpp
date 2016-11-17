#include <iostream>
#include "gdal.h"
#include <gdal_priv.h>
#include "ogrsf_frmts.h"
#include <limits>
using namespace::std;
int main()
{

    OGRRegisterAll();

    OGRDataSource *poDS;

    poDS = OGRSFDriverRegistrar::Open( "mile.shp", FALSE );
    if( poDS == NULL )
    {
        printf( "Open failed.\n" );
        exit( 1 );
    }
    OGRLayer  *poLayer;

    poLayer = poDS->GetLayerByName("mile");

    OGRFeature *poFeature;
    OGRGeometry *first_po_geometry;
    if ((poFeature = poLayer->GetNextFeature()) != NULL )
    {        
        first_po_geometry = poFeature->GetGeometryRef();
    }
    std::vector<OGRPoint> pts_vector;
    std::vector<double> dist_vector;
    double min_distance = numeric_limits<double>::max();
    double max_distance = numeric_limits<double>::min();
    //poLayer->ResetReading(); Don't reset distance
    while( (poFeature = poLayer->GetNextFeature()) != NULL )
    {
        OGRGeometry *poGeometry;
        poGeometry = poFeature->GetGeometryRef();
        double curr_distance = first_po_geometry->Distance(poGeometry);
        dist_vector.push_back(curr_distance);
        if ( curr_distance < min_distance ) {
          min_distance = curr_distance;
        }
        if ( curr_distance > max_distance ) {
          max_distance = curr_distance;
        }
        
        if( poGeometry != NULL 
            && wkbFlatten(poGeometry->getGeometryType()) == wkbPoint )
        {
            OGRPoint *poPoint = (OGRPoint *) poGeometry;
            OGRPoint temp = OGRPoint(poPoint->getX(), poPoint->getY());
            pts_vector.push_back(temp);            
        }
        else
        {
            printf( "no point geometry\n" );
        }       
        OGRFeature::DestroyFeature( poFeature );
    }
    printf( "The minimum distance between the first point and all the points in the distance is %f \n", min_distance );  
    printf( "The maximum distance between the first point and all the points in the distance is %f \n", max_distance );
    cout<< "Distance vector values" << endl;
    for (int i = 0; i<dist_vector.size(); i++){
      cout<< dist_vector[i] << endl;
    }
    OGRDataSource::DestroyDataSource( poDS );
}
