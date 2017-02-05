#include "GPX_parse.h"
#include "parse_double.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>

static long long parseDate(char *dateStr)
{
	struct tm date;

	dateStr[4] = 0;
	date.tm_year = atoi(dateStr) - 1900;

	dateStr += 5;
	dateStr[2] = 0;
	date.tm_mon = atoi(dateStr) - 1;

	dateStr += 3;
	dateStr[2] = 0;
	date.tm_mday = atoi(dateStr);

	dateStr += 3;
	dateStr[2] = 0;
	date.tm_hour = atoi(dateStr);

	dateStr += 3;
	dateStr[2] = 0;
	date.tm_min = atoi(dateStr);

	dateStr += 3;
	dateStr[2] = 0;
	date.tm_sec = atoi(dateStr);

	return (long long) mktime(&date);
}

static bool isText_trkseg(char *text, int start)
{
	for(int i = 0; i < 8; i++)
		if(text[(start + i) % 8] != "<trkseg>"[i])
			return false;

	return true;
}

GPX_entity *GPX_read(char *filename, int *output_array_size)
{
	GPX_entity *array;

	array = malloc(sizeof(GPX_entity) * 16);
	*output_array_size = 0;
	int array_capacity = 16;
	int trkseg_count = 0;

	FILE *f;
	f = fopen(filename, "r");

	int i = 0, buffer_pos = 0;
	char  c, entry[1000], buffer[10000];

	while(true)
    {
        char text[] = "12345678";
        while(!isText_trkseg(text, i)) // seeking for <trkseg> markup
        {
            text[i % 8] = getc(f);
            if(text[i % 8] == EOF)
            {
                fclose(f);
                if(trkseg_count % 2 == 0 && trkseg_count > 0)   // successful end of reading
                    return array;
                else    // if there is not <trkseg> markup in file
                {
                    free(array);
                    return NULL;
                }
            }

            i++;
        }

        bool end_of_trkseg = false;
        trkseg_count++;

        while(!end_of_trkseg)
        {
            i = 0;
            while((c = getc(f)) != '<')
                if(c == EOF)
                {
                    *output_array_size = 0; // if GPX file is not valid
                    free(array);
                    fclose(f);
                    return NULL;
                }
                else if(!isspace(c))
                    buffer[buffer_pos++] = c;

            if((c = getc(f)) == '!') // skipping xml comments
            {
                if(c == EOF)
                {
                    *output_array_size = 0; // if GPX file is not valid
                    free(array);
                    fclose(f);
                    return NULL;
                }

                while(i < 3 || (entry[i - 3] != '-' || entry[i - 2] != '-' || entry[i - 1] != '>'))
                {
                    entry[i] = getc(f);

                    if(entry[i] == EOF)
                    {
                        *output_array_size = 0; // if GPX file is not valid
                        free(array);
                        fclose(f);
                        return NULL;
                    }

                    i++;
                }
                entry[i] = 0;
            }
            else
            {
                entry[i++] = '<';
                entry[i++] = c;

                while((entry[i++] = getc(f)) != '>')
                    if(entry[i - 1] == EOF)
                    {
                        *output_array_size = 0; // if GPX file is not valid
                        free(array);
                        fclose(f);
                        return NULL;
                    }

                entry[i] = 0;

                if(strcmp(entry, "</trkseg>") == 0)
                {
                    trkseg_count++;
                    end_of_trkseg = true;
                }
                else if(strstr(entry, "<trkpt"))
                {
                    if(*output_array_size + 1 == array_capacity)	//resizing array
                    {
                        array = realloc(array, array_capacity * 2 * sizeof(GPX_entity));
                        array_capacity *= 2;
                    }

                    // setting default GPX_entity values
                    if(*output_array_size > 0)
                    {
                        array[*output_array_size].time = array[*output_array_size - 1].time;
                        array[*output_array_size].ele = array[*output_array_size - 1].ele;
                    }
                    else
                    {
                        array[*output_array_size].time = 0;
                        array[*output_array_size].ele = 0;
                    }

                    array[*output_array_size].lastInSegment = false;

                    // setting default GPX_entity values-end

                    // reading first attribute

                    i = 0;
                    char num[50], mid[100];
                    int j = 0;

                    while(entry[i] != '"')
                        mid[j++] = entry[i++];
                    mid[j] = 0;
                    j = 0;
                    i++;

                    while(entry[i] != '"')
                        num[j++] = entry[i++];

                    num[j] = 0;
                    if(strstr(mid, "lon"))
                        array[*output_array_size].lon = parseDouble(num);
                    if(strstr(mid, "lat"))
                        array[*output_array_size].lat = parseDouble(num);

                    // reading first attribute-end

                    // reading second attribute
                    i++;
                    j = 0;

                    while(entry[i] != '"')
                        mid[j++] = entry[i++];
                    mid[j] = 0;
                    j = 0;
                    i++;

                    while(entry[i] != '"')
                        num[j++] = entry[i++];

                    num[j] = 0;
                    if(strstr(mid, "lon"))
                        array[*output_array_size].lon = parseDouble(num);
                    if(strstr(mid, "lat"))
                        array[*output_array_size].lat = parseDouble(num);

                    // reading second attribute-end
                }
                else if(strcmp(entry, "</ele>") == 0)
                {
                    buffer[buffer_pos] = 0;
                    array[*output_array_size].ele = parseDouble(buffer);
                }
                else if(strcmp(entry, "</time>") == 0)
                {
                    buffer[buffer_pos] = 0;
                    array[*output_array_size].time = parseDate(buffer);
                }
                else if(strcmp(entry, "</trkpt>") == 0)
                {
                    (*output_array_size)++;
                }
                else
                    buffer_pos = 0;
            }
        }
        if(*output_array_size)
            array[*output_array_size - 1].lastInSegment = true;
    }
}
