"""Functionality for going from the easier machine-readable
output format to a more human-readable variant."""

from output_keys import (
	BEGIN, END, NO_TRANSITIONS, FORMULA, TOTAL_EVALUATION_TIME, FORMULA_PRINT
)

def export_key(key):
	exported_key = key.replace(BEGIN, '')
	exported_key = exported_key.replace(END, '')
	return exported_key

NO_TRANSITIONS = export_key(NO_TRANSITIONS)
MISCELLANEOUS = 'Miscellaneous'
FORMULA = export_key(FORMULA)
TOTAL_EVALUATION_TIME = export_key(TOTAL_EVALUATION_TIME)
FORMULA_PRINT = export_key(FORMULA_PRINT)