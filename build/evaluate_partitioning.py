import sys
from query_information_annotation import annotate, ANNOTATIONS
from average_logs_functions import average_runs
from os import listdir
from os.path import isdir
from export_keys import TOTAL_EVALUATION_TIME, MISCELLANEOUS, FORMULA, NO_PLACES, NO_TRANSITIONS, NO_PLACES
from average_logs_functions import get_shared_queries, distinct_queries
from aggregate_logs_functions import write_to_csv
from decimal import *


if len(sys.argv) < 2:
    raise Exception('Missing argument.')

# Take a directory of directories of runs for each partioning
# The name of the directories for each partioning will be used to label best partitioning in the resulting csv
directory_of_experiments = sys.argv[1] 

if not isdir(directory_of_experiments):
    raise Exception(directory_of_experiments + ' is not a directory.')

experiments = [{'partitioning_strategy': directory, 'avg_run': average_runs(directory_of_experiments + '/' + directory, [TOTAL_EVALUATION_TIME])} for directory in listdir(directory_of_experiments)]

for experiment in experiments:
    annotate(experiment['avg_run'])
    for query in experiment['avg_run']:
        del query[MISCELLANEOUS]

# Find the queries in common for all experiments
shared_queries = get_shared_queries([experiment['avg_run'] for experiment in experiments])
shared_queries = distinct_queries(shared_queries)

summary = [query for query in shared_queries]
for query in summary:
	query[TOTAL_EVALUATION_TIME] = Decimal('0')

for query in shared_queries:
    print(query[FORMULA], query['Query Nested Depth'])

for experiment in experiments:
	for query in experiment['avg_run']:
		for sh_query in summary:
			if query[FORMULA] == sh_query[FORMULA]:
				if query[TOTAL_EVALUATION_TIME] > sh_query[TOTAL_EVALUATION_TIME]:
					sh_query[TOTAL_EVALUATION_TIME] = query[TOTAL_EVALUATION_TIME]
					sh_query['Best Partioning'] = experiment['partitioning_strategy']

    print(experiment['partitioning_strategy'])

# Annotate the average for each partioning experiment
# Compare Total Search Time and pick the experiment with the lowest for the query
# Label the query with the experiment
# Prune all uninteresting columns
# Interesting columns are FORMULA (for recognizing them) and NO_MARKINGS, NO_TRANSITIONS, and all the query annotations for classification
interesting_columns = ANNOTATIONS + [FORMULA, NO_PLACES, NO_TRANSITIONS]
for query in summary:
	for c in interesting_columns:
		del query.pop(c)
# Write to a common csv file
write_to_csv # function not called