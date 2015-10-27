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
	int endoffile = 0;
	char ch;
	FILE *ifp, *ofp;
	char *resultfile = "MCCMarcieRes.csv";

	ifp = fopen(resultfile, "r");
	
	char result[1024];
	while (fgets(result, 1024, resultfile)) {
		char* tmp = strdup(result);
		printf("Modelnavn: %s\n", getfield(tmp, 2));
		free(tmp);
	}
	fclose(ifp);
	return 0;
}
