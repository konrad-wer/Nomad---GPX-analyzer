#ifndef GPX_PARSE
#define GPX_PARSE

#include <stdbool.h>

typedef struct
{
	double lat;
	double lon;
	double ele;
	long long time; // unix time
	bool lastInSegment;
} GPX_entity;

GPX_entity *GPX_read(char *filename, int *output_size);

#endif // GPX_PARSE
