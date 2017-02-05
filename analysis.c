#include "analysis.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

static double min(double a, double b, double c)
{
    double min = a < b ? a : b;
    return c < min ? c : min;
}

static int filterDivisionResult(int nPoints, bool *divisionPoints, int nDivisions)
{
    bool last = false;
    int seqBegin = 0;

    for(int i = 1; i < nPoints - 1; i++)
    {
        if(divisionPoints[i] == true)
        {
            if(last == false)
                seqBegin = i;

            last = true;
            divisionPoints[i] = false;

            nDivisions--;
        }
        else if(divisionPoints[i] == false)
        {
            if(last)
            {
                divisionPoints[(seqBegin + i - 1) / 2] = true;
                nDivisions++;
            }

            last = false;
        }
    }

    if(last)
    {
        divisionPoints[(seqBegin + nPoints - 1) / 2] = true;
        nDivisions++;
    }

    return nDivisions;
}

int divideByTime(int nPoints, GPX_entity *points, bool *divisionPoints, double minutes)
{
    if(nPoints == 0 || points == NULL || divisionPoints == NULL)
        return 0;

    double nextInterval = minutes * 60;
    int start =  points[0].time;
    int nDivisions = 0;

    for(int i = 0; i < nPoints; i++)
    {
        if(points[i].time - start > nextInterval)
        {
            divisionPoints[i] = true;
            nDivisions++;

            while(points[i].time - start > nextInterval)
                nextInterval += minutes * 60;
        }
    }

    return nDivisions;
}

int divideByDistance(int nPoints, GPX_entity *points, bool *divisionPoints, double segmentLength)
{
    if(nPoints == 0 || points == NULL || divisionPoints == NULL)
        return 0;

    double nextDivisionAt = segmentLength;
    double d = 0, x, y;
    int nDivisions = 0;

    for(int i = 1; i < nPoints; i++)
    {
        if(!points[i - 1].lastInSegment && points[i].time - points[i - 1].time)
        {
            double scale = cos((points[i].lat + points[i - 1].lat) / 2 / 180.0 * 3.141592);
            x = (points[i].lon - points[i - 1].lon) * 40075 * scale / 360;
            y = (points[i].lat - points[i - 1].lat) * 40030 / 360;

            d += sqrt(x * x + y * y);

            if(d >= nextDivisionAt)
            {
                divisionPoints[i] = true;
                nDivisions++;

                while(d >= nextDivisionAt)
                    nextDivisionAt += segmentLength;
            }
        }
    }

    return nDivisions;
}

int divideBySpeedOrPace(int type, int nPoints, GPX_entity *points, bool *divisionPoints, double deltaVal, int sample)
{
    if(nPoints == 0 || points == NULL || divisionPoints == NULL)
        return 0;
    double d1 = 0, d2 = 0, x, y, scale;
    int nDivisions = 0;

    if(sample < nPoints - sample - 1)
    {
        for(int i = 0; i < sample; i++) // counting left side initial value
        {
            scale = cos((points[i].lat + points[i + 1].lat) / 2 / 180.0 * 3.141592);
            x = (points[i + 1].lon - points[i].lon) * 40075 * scale / 360;
            y = (points[i + 1].lat - points[i].lat) * 40030 / 360;

            if(!points[i].lastInSegment)
                d1 += sqrt(x * x + y * y);
        }

        for (int i = sample ; i < 2 * sample; i++) // counting right side initial value
        {
            scale = cos((points[i].lat + points[i + 1].lat) / 2 / 180.0 * 3.141592);
            x = (points[i + 1].lon - points[i].lon) * 40075 * scale / 360;
            y = (points[i + 1].lat - points[i].lat) * 40030 / 360;

            if(!points[i].lastInSegment)
                d2 += sqrt(x * x + y * y);
        }

        int ldi = 0;  //last division index
        double distFromLd; // distance from last division

        for(int i = sample; i < nPoints - sample; i++)
        {
            //calculating dist from last division

            scale = cos((points[i].lat + points[ldi].lat) / 2 / 180.0 * 3.141592);  //updating left side's left end
            x = (points[i].lon - points[ldi].lon) * 40075 * scale / 360;
            y = (points[i].lat - points[ldi].lat) * 40030 / 360;

            distFromLd = sqrt(x * x + y * y);
            //calculating dist from last division-end

            double deltaT1, deltaT2;
            deltaT1 = (points[i].time - points[i - sample].time);
            deltaT2 = (points[i + sample].time - points[i].time);

            if(!points[i - 1].lastInSegment && points[i].time - points[ldi].time && distFromLd > 0 && type == SPEED && deltaT1 && deltaT2 &&
                fabs(d1 / deltaT1 * 3600 - d2 / deltaT2 * 3600) > deltaVal) // searching for difference in speed
            {
                divisionPoints[i] = true;
                nDivisions++;
                ldi = i;
            }
            if(!points[i - 1].lastInSegment && points[i].time - points[ldi].time && distFromLd > 0 && type == PACE && deltaT1 && deltaT2 &&
               fabs(deltaT1 / 60 / d1 - deltaT2 / 60 / d2) > deltaVal) // searching for difference in pace
            {
                divisionPoints[i] = true;
                nDivisions++;
            }

            if(i < nPoints - sample - 1)
            {
                scale = cos((points[i - sample].lat + points[i - sample + 1].lat) / 2 / 180.0 * 3.141592);  //updating left side's left end
                x = (points[i - sample + 1].lon - points[i - sample].lon) * 40075 * scale / 360;
                y = (points[i - sample + 1].lat - points[i - sample].lat) * 40030 / 360;

                if(!points[i - sample].lastInSegment)
                    d1 -= sqrt(x * x + y * y);

                scale = cos((points[i].lat + points[i + 1].lat) / 2 / 180.0 * 3.141592);  //updating left side's right end and right side's left end
                x = (points[i + 1].lon - points[i].lon) * 40075 * scale / 360;
                y = (points[i + 1].lat - points[i].lat) * 40030 / 360;

                if(!points[i].lastInSegment)
                {
                    d1 += sqrt(x * x + y * y);
                    d2 -= sqrt(x * x + y * y);
                }

                scale = cos((points[i + sample + 1].lat + points[i + sample].lat) / 2 / 180.0 * 3.141592); //updating right side's right end
                x = (points[i + sample + 1].lon - points[i + sample].lon) * 40075 * scale / 360;
                y = (points[i + sample + 1].lat - points[i + sample].lat) * 40030 / 360;

                if(!points[i + sample].lastInSegment)
                    d2 += sqrt(x * x + y * y);
            }
        }
    }

    if(sample > 2)
        nDivisions = filterDivisionResult(nPoints, divisionPoints, nDivisions);

    return nDivisions;
}

int divideByAngle(int nPoints, GPX_entity *points, bool *divisionPoints, double deltaAngle, int sample)
{
    if(nPoints == 0 || points == NULL || divisionPoints == NULL)
        return 0;
    double a1 = 0, a2 = 0, x, y, r, scale, lon1 = 0, lat1 = 0, lon2 = 0, lat2 = 0;
    int nDivisions = 0;

    if(sample < nPoints - sample - 1)
    {
        for(int i = 0; i < sample; i++) // counting left side initial value
        {
            lon1 += points[i].lon;
            lat1 += points[i].lat;
        }

        for (int i = sample + 1; i <= 2 * sample; i++) // counting right side initial value
        {
            lon2 += points[i].lon;
            lat2 += points[i].lat;
        }

        int ldi = 0;  //last division index
        double distFromLd; // distance from last division

        for(int i = sample; i < nPoints - sample; i++)
        {
            //calculating dist from last division

            scale = cos((points[i].lat + points[ldi].lat) / 2 / 180.0 * 3.141592);  //updating left side's left end
            x = (points[i].lon - points[ldi].lon) * 40075 * scale / 360;
            y = (points[i].lat - points[ldi].lat) * 40030 / 360;

            distFromLd = sqrt(x * x + y * y);
            //calculating dist from last division-end


            scale = cos((points[i].lat + lat1 / sample) / 2 / 180.0 * 3.141592);
            x = (points[i].lon - lon1 / sample) * 40075 * scale / 360;
            y = (points[i].lat - lat1 / sample) * 40030 / 360;
            r = sqrt(x * x + y * y);

            if(y >= 0)
                a1 = acos(x / r);
            else
                a1 = 2 * 3.141592 - acos(x / r);


            scale = cos((points[i].lat + lat2 / sample) / 2 / 180.0 * 3.141592);
            x = (lon2 / sample - points[i].lon) * 40075 * scale / 360;
            y = (lat2 / sample - points[i].lat) * 40030 / 360;
            r = sqrt(x * x + y * y);

            if(y >= 0)
                a2 = acos(x / r);
            else
                a2 = 2 * 3.141592 - acos(x / r);



            if(points[i].time - points[ldi].time && distFromLd > 0 &&
                   min(fabs(a1 - a2), fabs(a1 + (2 * 3.141592 - a2)),
                   fabs(-(2 * 3.141592 - a1) - a2)) > deltaAngle) // searching for difference in angle
            {
                divisionPoints[i] = true;
                nDivisions++;
                ldi = i;
            }

            if(i < nPoints - sample - 1) //updating when it is not a last point
            {
                lat1 -= points[i - sample].lat;  //updating left side's left end
                lon1 -= points[i - sample].lon;

                lat1 += points[i].lat;  //updating left side's right end and right side's left end
                lon1 += points[i].lon;

                lat2 -= points[i + 1].lat;  //updating left side's right end and right side's left end
                lon2 -= points[i + 1].lon;

                lat2 += points[i + sample + 1].lat;  //updating right side's right end
                lon2 += points[i + sample + 1].lon;
            }
        }
    }

    if(sample > 2)
        nDivisions = filterDivisionResult(nPoints, divisionPoints, nDivisions);

    return nDivisions;
}

void analyze(int nPoints, GPX_entity *points, bool *divisionPoints, int nDivisions)
{
    if(nPoints == 0 || points == NULL || divisionPoints == NULL)
        return;

    double distance = 0, maxAltitude, minAltitude, x, y, maxSpeed = 0, minSpeed = 100000, v;
    maxAltitude = minAltitude = points[0].ele;
    char analysisLine[256];
    int time, hour, min, sec, start = points[0].time, segmentId = 1;

    // whole track analysis
    time = points[nPoints - 1].time - points[0].time;
    hour = time / 3600;
    min = (time % 3600) / 60;
    sec = time % 60;

    for(int i = 1; i < nPoints; i++)
    {
        double scale = cos((points[i].lat + points[i - 1].lat) / 2 / 180.0 * 3.141592);
        x = (points[i].lon - points[i - 1].lon) * 40075 * scale / 360;
        y = (points[i].lat - points[i - 1].lat) * 40030 / 360;

        if(!points[i - 1].lastInSegment)
            distance += sqrt(x * x + y * y);

        if(!points[i - 1].lastInSegment && points[i].time - points[i - 1].time)
        {
            v = sqrt(x * x + y * y) / (points[i].time - points[i - 1].time) * 3600;
            maxSpeed = v > maxSpeed ? v : maxSpeed;
            minSpeed = v < minSpeed ? v : minSpeed;
        }

        maxAltitude = points[i].ele > maxAltitude ? points[i].ele : maxAltitude;
        minAltitude = points[i].ele < minAltitude ? points[i].ele : minAltitude;
    }

    clearTextBox();

    appendTextBox("======= Whole track =======\n\n");

    sprintf(analysisLine, "Distance: %.3f km\n", distance);
    appendTextBox(analysisLine);

    sprintf(analysisLine, "Time: %.2d:%2.2d:%2.2d\n", hour, min, sec);
    appendTextBox(analysisLine);

    sprintf(analysisLine, "Max speed: %.3f km/h\n", maxSpeed);
    appendTextBox(analysisLine);

    sprintf(analysisLine, "Min speed: %.3f km/h\n", minSpeed);
    appendTextBox(analysisLine);

    sprintf(analysisLine, "Average speed: %.3f km/h\n", distance / (time / 3600.0));
    appendTextBox(analysisLine);

    sprintf(analysisLine, "Average pace: %.2f min/km\n", (time / 60.0) / distance);
    appendTextBox(analysisLine);

    sprintf(analysisLine, "Altitude difference: %.2f m\n", maxAltitude - minAltitude);
    appendTextBox(analysisLine);

    // whole track analysis-end

    // segments analysis

    distance = 0;
    maxAltitude = minAltitude = points[0].ele;
    maxSpeed = 0;
    minSpeed = 100000;

    for(int i = 1; i < nPoints && nDivisions; i++)
    {
        double scale = cos((points[i].lat + points[i - 1].lat) / 2 / 180.0 * 3.141592);
        x = (points[i].lon - points[i - 1].lon) * 40075 * scale / 360;
        y = (points[i].lat - points[i - 1].lat) * 40030 / 360;

        if(!points[i - 1].lastInSegment)
            distance += sqrt(x * x + y * y);

        if(!points[i - 1].lastInSegment && points[i].time - points[i - 1].time)
        {
            v = sqrt(x * x + y * y) / (points[i].time - points[i - 1].time) * 3600;
            maxSpeed = v > maxSpeed ? v : maxSpeed;
            minSpeed = v < minSpeed ? v : minSpeed;
        }

        maxAltitude = points[i].ele > maxAltitude ? points[i].ele : maxAltitude;
        minAltitude = points[i].ele < minAltitude ? points[i].ele : minAltitude;

        if(divisionPoints[i])
        {
            time = points[i].time - start;
            hour = time / 3600;
            min = (time % 3600) / 60;
            sec = time % 60;

            sprintf(analysisLine, "\n======= Segment %d =======\n\n", segmentId);
            appendTextBox(analysisLine);

            sprintf(analysisLine, "Distance: %.3f km\n", distance);
            appendTextBox(analysisLine);

            sprintf(analysisLine, "Time: %.2d:%2.2d:%2.2d\n", hour, min, sec);
            appendTextBox(analysisLine);

            sprintf(analysisLine, "Max speed: %.3f km/h\n", maxSpeed);
            appendTextBox(analysisLine);

            sprintf(analysisLine, "Min speed: %.3f km/h\n", minSpeed);
            appendTextBox(analysisLine);

            sprintf(analysisLine, "Average speed: %.3f km/h\n", (points[i].time - start ? distance / (time / 3600.0) : 0)); //checking if time difference is not 0
            appendTextBox(analysisLine);

            sprintf(analysisLine, "Average pace: %.2f min/km\n", (distance ? (time / 60.0) / distance : 0));
            appendTextBox(analysisLine);

            sprintf(analysisLine, "Altitude difference: %.2f m\n", maxAltitude - minAltitude);
            appendTextBox(analysisLine);

            distance = 0;
            maxAltitude = minAltitude = points[i].ele;
            start = points[i].time;
            maxSpeed = 0;
            minSpeed = 100000;
            segmentId++;
        }
    }

    // segments analysis-end

    appendTextBox("\n=====================");
}
