"""Functionality for going from the easier machine-readable
output format to a more human-readable variant."""

from output_keys import (
	BEGIN, END, NO_TRANSITIONS
)

def export_key(key):
	exported_key = key.replace(BEGIN, '')
	exported_key = exported_key.replace(END, '')
	return exported_key

NO_TRANSITIONS = export_key(NO_TRANSITIONS)
MISCELLANEOUS = 'Miscellaneous'