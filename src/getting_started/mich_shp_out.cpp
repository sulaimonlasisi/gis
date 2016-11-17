#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdlib.h> 
#include "gdal.h"
#include <gdal_priv.h>
#include "ogrsf_frmts.h"

int main()
{
    std::ifstream fin;
    const char *input_file = "mile_2.csv";
    fin.open(input_file);
    if( fin.fail() )
    {
        printf( "%s couldn't open cvs.\n", input_file );
        exit( 1 );
    }
    const char *pszDriverName = "ESRI Shapefile";
    OGRSFDriver *poDriver;

    OGRRegisterAll();

    poDriver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(
                pszDriverName );
    if( poDriver == NULL )
    {
        printf( "%s driver not available.\n", pszDriverName );
        exit( 1 );
    }

    OGRDataSource *poDS;

    poDS = poDriver->CreateDataSource( "point_out.shp", NULL );
    if( poDS == NULL )
    {
        printf( "Creation of output file failed.\n" );
        exit( 1 );
    }

    OGRLayer *poLayer;

    poLayer = poDS->CreateLayer( "point_out", NULL, wkbPoint, NULL );
    if( poLayer == NULL )
    {
        printf( "Layer creation failed.\n" );
        exit( 1 );
    }

    OGRFieldDefn oField( "Name", OFTString );

    oField.SetWidth(32);

    if( poLayer->CreateField( &oField ) != OGRERR_NONE )
    {
        printf( "Creating Name field failed.\n" );
        exit( 1 );
    }

    double x, y;
    char comma; 
    char szName [50];

    while( fin >> x >> comma >> y >> comma >> szName)
    {
                
        OGRFeature *poFeature;
        poFeature = OGRFeature::CreateFeature( poLayer->GetLayerDefn() );
        poFeature->SetField( "Name", szName );

        OGRPoint pt;
        
        pt.setX( x );
        pt.setY( y );
 
        poFeature->SetGeometry( &pt ); 

        if( poLayer->CreateFeature( poFeature ) != OGRERR_NONE )
        {
           printf( "Failed to create feature in shapefile.\n" );
           exit( 1 );
        }

         OGRFeature::DestroyFeature( poFeature );
    }

    OGRDataSource::DestroyDataSource( poDS );

    fin.close();
    return 0;
}