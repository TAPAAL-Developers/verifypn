"""Parsing of output from test runs.

Outputted lines may or may not start with an output key.
Lines not starting with an output key will be collected in
the miscellaneous field. Such lines are often errors."""
from output_keys import ALL_KEYS, NO_CONFIGURATIONS, PROCESSED_EDGES, PROCESSED_NEGATION_EDGES
from export_keys import export_key, MISCELLANEOUS, MAXIMUM_DISTRIBUTION_POTENTIAL, DISTRIBUTION_VARIANCE, NO_WORKERS_COMPUTING
from decimal import Decimal
import re
import math

miscellaneous = []

def _calculate_total_no_configurations(lines):
    total_no_configurations = Decimal('0')
    for line in lines:
        if line.startswith(NO_CONFIGURATIONS):
            no_configurations = line.split(NO_CONFIGURATIONS)[1].strip()
            total_no_configurations += Decimal(no_configurations)
    return total_no_configurations


def _edges_processed_per_worker(lines):
    worker_print = re.compile(r'^\[Worker \d* printing stats\]')
    current_worker = -1
    edges_processed_per_worker = []
    for line in lines:
        if worker_print.match(line):
            edges_processed_per_worker.append(Decimal('0'))
            current_worker += 1
        for processed_key in [PROCESSED_EDGES, PROCESSED_NEGATION_EDGES]:
            if line.startswith(processed_key):
                value = line.split(processed_key)[1].strip()
                edges_processed_per_worker[current_worker] += Decimal(value)

    return edges_processed_per_worker

def _calculate_maximum_distribution_potential(lines):
    edges_processed_per_worker = _edges_processed_per_worker(lines)
    if edges_processed_per_worker:
        total_edges_processed = sum(edges_processed_per_worker)
        normalized_edges_processed_per_worker = [(edges_processed_by_worker/total_edges_processed) * 100 
                                                 for edges_processed_by_worker in edges_processed_per_worker]
        maximum_percent_of_edges_processed_by_worker = max(normalized_edges_processed_per_worker)
        return Decimal('100') - maximum_percent_of_edges_processed_by_worker
    else:
        return Decimal('0')

def _no_of_workers_computing(lines):
    edges_processed_per_worker = _edges_processed_per_worker(lines)
    return Decimal(str(len([
        edges_processed_by_worker
        for edges_processed_by_worker in edges_processed_per_worker
        if edges_processed_by_worker != Decimal('0')
        ])))

def _calculate_distribution_variance(lines):
    edges_processed_per_worker = _edges_processed_per_worker(lines)

    if edges_processed_per_worker:
        total_edges_processed = Decimal(str(sum(edges_processed_per_worker)))
        no_workers = Decimal(str(len(edges_processed_per_worker)))

        average_edges_processed = total_edges_processed/no_workers
        
        deviation = sum([(((edges_processed_by_worker - average_edges_processed)/total_edges_processed) * 100)**2
                         for edges_processed_by_worker in edges_processed_per_worker])
        variance = math.sqrt(deviation)

        return variance
    else:
        return Decimal('0')


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
    parsed_lines.append((MAXIMUM_DISTRIBUTION_POTENTIAL, _calculate_maximum_distribution_potential(lines)))
    parsed_lines.append((DISTRIBUTION_VARIANCE, _calculate_distribution_variance(lines)))
    parsed_lines.append((NO_WORKERS_COMPUTING, _no_of_workers_computing(lines)))

    return dict(parsed_lines)
