#ifndef GEOOBJECT_H
#define GEOOBJECT_H

#include <QString>

struct GeoObject {
    QString name;
    double latitude;
    double longitude;

    double equipment;
    double accessibility;
    double cost;

    double suitability = 0.0;
};

#endif // GEOOBJECT_H
