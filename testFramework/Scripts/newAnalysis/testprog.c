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
	for (i = 0; i < 90; ++i)
	{
		if (i%2 == 1) //Fake error data
			printf("%s %d\n", "ERROR Number", i);
		if (i%10 == 5) //Fake correctness data
		{
			printf("%s %d\n", "NOT_CORRECT", i);
		}
		if (i%20 < 4) //Fake result data
		{
			printf("%s\n", "RESULT: Success");
		}
	}
	clock_t parse_end = clock();
	double elapse = diffclock(parse_end,parse_begin);
	printf("%s: %lf\n", "TIME_ELAPSE", elapse);
	return 0;
}
