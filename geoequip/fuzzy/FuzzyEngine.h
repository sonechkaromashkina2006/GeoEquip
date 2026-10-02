#ifndef FUZZYENGINE_H
#define FUZZYENGINE_H

#include "../model/GeoObject.h"

double calculateMembership(
    const GeoObject& obj,
    double desiredEquipment,
    double desiredAccessibility,
    double desiredCost
);

#endif // FUZZYENGINE_H
