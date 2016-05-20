"""Parsing of output from test runs.

Outputted lines may or may not start with an output key.
Lines not starting with an output key will be collected in
the miscellaneous field. Such lines are often errors."""
from output_keys import ALL_KEYS
from export_keys import export_key, MISCELLANEOUS

miscellaneous = []

def parse_line(line):
	"""Convert each line to a key, value pair for later dict conversion."""
	if any([line.startswith(key) and key != '[Formula Print]' for key in ALL_KEYS]):
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

    return dict(parsed_lines)
