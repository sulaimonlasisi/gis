#include <iostream>
#include "gdal.h"
#include "gdal_priv.h"
#include <geos/geom/GeometryFactory.h>
#include <geos/geom/CoordinateSequenceFactory.h>
#include <geos/geom/CoordinateSequence.h>
#include <geos/geom/LinearRing.h>
#include <geos/geom/Polygon.h>
#include <geos/geom/Point.h>
using namespace std;

int main (){
  geos::geom::GeometryFactory factory;
  geos::geom::CoordinateSequence * temp = factory.getCoordinateSequenceFactory()->create( (std::size_t) 0, 0);
  temp->add(geos::geom::Coordinate(0,0));
  temp->add(geos::geom::Coordinate(100,0));
  temp->add(geos::geom::Coordinate(100,100));
  temp->add(geos::geom::Coordinate(0,100));
  temp->add(geos::geom::Coordinate(0,0));
  geos::geom::LinearRing * shell = factory.createLinearRing (temp);
  geos::geom::Polygon * poly = factory.createPolygon (shell, null);
  std::cout << "Poly empty: 
  
  const char *output = "new.tif";
  GDALAllRegister();
  GDALDriver *testDriverTiff;
  GDALDataset *pNewDS;
  testDriverTiff = GetGDALDriverManager()->GetDriverByName("GTiff");
  pNewDS = testDriverTiff->Create(output,300,300,1, GDT_Float32, NULL);
  GDALClose(pNewDS);
 //cout << "Hello World" << endl;
  return 0;
}
