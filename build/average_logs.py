from average_logs_functions import average_runs
from aggregate_logs_functions import write_to_csv
from export_keys import TOTAL_EVALUATION_TIME, MISCELLANEOUS, MAXIMUM_DISTRIBUTION_POTENTIAL, DISTRIBUTION_VARIANCE, FORMULA_PRINT
import sys

if len(sys.argv) < 3:
    raise Exception('Not enough arguments given.')

directory_of_runs = sys.argv[1] # A directory with a subdirectory containing log files from a run
output_file = sys.argv[2]

averaged_run = average_runs(directory_of_runs=directory_of_runs, columns_to_be_averaged=[TOTAL_EVALUATION_TIME, MAXIMUM_DISTRIBUTION_POTENTIAL, DISTRIBUTION_VARIANCE])
for query in averaged_run:
    del query[MISCELLANEOUS]
    del query[FORMULA_PRINT]
write_to_csv(averaged_run, output_file)