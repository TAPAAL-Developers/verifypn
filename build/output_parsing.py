"""Parsing of output from test runs.

Outputted lines may or may not start with an output key.
Lines not starting with an output key will be collected in
the miscellaneous field. Such lines are often errors."""
from output_keys import ALL_KEYS, NO_CONFIGURATIONS, PROCESSED_EDGES, PROCESSED_NEGATION_EDGES
from export_keys import export_key, MISCELLANEOUS
from decimal import Decimal
import re

miscellaneous = []

def _calculate_total_no_configurations(lines):
    total_no_configurations = Decimal('0')
    for line in lines:
        if line.startswith(NO_CONFIGURATIONS):
            no_configurations = line.split(NO_CONFIGURATIONS)[1].strip()
            total_no_configurations += Decimal(no_configurations)
    return total_no_configurations

def _calculate_maximum_distribution_potential(lines):
    worker_print = re.compile(r'^\[Worker \d* printing stats\]')
    current_worker = -1
    edges_processed_per_worker = []
    for line in lines:
        if worker_print.match(line):
            edges_processed_per_worker.append(Decimal('0'))
            current_worker += 1
        if line.startswith(PROCESSED_EDGES):
            value = line.split(key)[1].strip()
            edges_processed_per_worker[current_worker] += Decimal()
        if line.startswith(PROCESSED_NEGATION_EDGES):
            pass

def parse_line(line):
	"""Convert each line to a key, value pair for later dict conversion."""
	if any([line.startswith(key) for key in ALL_KEYS]):
		for key in ALL_KEYS:
			if line.startswith(key):
				value = line.split(key)[1].strip()
				return (export_key(key), value)
	else:
		#miscellaneous.append(line)
		return (MISCELLANEOUS, miscellaneous)

def parse(log_file):
    with open(log_file, 'r') as lf:
        lines = lf.readlines()

    parsed_lines = [parse_line(line) for line in lines]
    parsed_lines.append((MISCELLANEOUS, ''.join(miscellaneous)))
    parsed_lines.append((export_key(NO_CONFIGURATIONS), _calculate_total_no_configurations(lines)))
    parsed_lines.append()

    return dict(parsed_lines)
