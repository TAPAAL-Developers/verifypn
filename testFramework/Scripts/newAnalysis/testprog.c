#include <stdio.h>
#include <string.h>
#include "time.h"

struct result
{
	char *toolname;
	char *modelname;
	char *examination;
	int time_flag;
	char *result;
	char *status;
	char *estimated_result;
};

const char* getfield(char* line, int num)
{
    const char* tok;
    for (tok = strtok(line, ",");
            tok && *tok;
            tok = strtok(NULL, ",\n"))
    {
        if (!--num)
            return tok;
    }
    return NULL;
}

int main(int argc, char const *argv[])
{
	clock_t parse_begin = clock();
	int i;
	int count = 0;
	for ( i = 0; i < 50; ++i)
	{
		int j;
		for( j = 0; j < 4; j++)
		{
			if(j == 0){
				printf("%s-%d %s\n", "FORMULA TEST", ++count, "TRUE");
			}
			else if (j == 1)
				printf("%s-%d %s\n", "FORMULA TEST", ++count, "FALSE");
			else if (j == 2){
				printf("%s-%d %s\n", "FORMULA TEST", ++count, "CANNOT_COMPUTE");
				printf("%s-%d %s\n", "FORMULA TEST", ++count, "CANNOT_COMPUTE");
			}
			else if(j == 3)
				printf("%s-%d %s\n", "FORMULA TEST", ++count, "DO_NOT_COMPETE");
		}
	}
	fclose(ifp);
	return 0;
}
