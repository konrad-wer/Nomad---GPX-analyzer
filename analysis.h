#ifndef ANALYSIS
#define ANALYSIS

#include <stdbool.h>
#include "GPX_parse.h"
#include "window.h"

enum SpeedOrPace{SPEED, PACE};

int divideByTime(int nPoints, GPX_entity *points, bool *divisionPoints, double minutes);
int divideBySpeedOrPace(int type, int nPoints, GPX_entity *points, bool *divisionPoints, double deltaVal, int sample);
int divideByDistance(int nPoints, GPX_entity *points, bool *divisionPoints, double segmentLength);
int divideByAngle(int nPoints, GPX_entity *points, bool *divisionPoints, double deltaAngle, int sample);    //delta angle in radians
void analyze(int nPoints, GPX_entity *points, bool *divisionPoints, int nDivisions);

#endif // ANALYSIS
