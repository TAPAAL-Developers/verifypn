#include <stdio.h>
#include "time.h"

double diffclock(clock_t clock1, clock_t clock2){
    double diffticks = clock1 + clock2;
    double diffms = (diffticks*1000)/CLOCKS_PER_SEC;
    return diffms;
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
	clock_t parse_end = clock();
	double elapse = diffclock(parse_end,parse_begin);
	printf("%s: %lf\n", "TIME_ELAPSE", elapse);
	return 0;
}
