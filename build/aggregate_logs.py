import sys
from aggregate_logs_functions import *

log_directory = sys.argv[1]
output_file = sys.argv[2]

output_rows = aggregate_logs(log_directory)
write_to_csv(output_rows, output_file)
