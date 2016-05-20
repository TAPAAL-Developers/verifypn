"""Keys expected from the printed output from running a query.

Each data field is expected to be printed on a single line with the key as a prefix.
The output can be printed in any order, as long as it is printed before query evaluation
as noted below.
"""

BEGIN = '['
END = ']'

# Expected before query evaluation
MODEL_NAME =  BEGIN + 'Model Name' + END
NO_TRANSITIONS = BEGIN + 'No. Transitions' + END
NO_PLACES = BEGIN + 'No. Places' + END
MODEL_PARSING_TIME = BEGIN + 'Model Parsing Time' + END
QUERY_PARSING_TIME = BEGIN + 'Query Parsing Time' + END
FORMULA = BEGIN + 'Formula' + END
FORMULA_PRINT = BEGIN + 'Formula Print' + END

# Expected printed after query evaluation
QUERY_RESULT =  BEGIN + 'Query Result' + END
QUERY_EVALUATION_TIME = BEGIN + 'Query Evaluation Time' + END
TOTAL_EVALUATION_TIME = BEGIN + 'Total Evaluation Time' + END
NO_CONFIGURATIONS = BEGIN + 'No. Configurations' + END
NO_MARKINGS = BEGIN + 'No. Markings' + END
PATH_TO_MODEL = BEGIN + 'Path To Model' + END

ALL_KEYS = [
	MODEL_NAME, NO_TRANSITIONS, NO_PLACES, MODEL_PARSING_TIME, QUERY_PARSING_TIME, FORMULA, FORMULA_PRINT,
	QUERY_RESULT, QUERY_EVALUATION_TIME, TOTAL_EVALUATION_TIME, NO_CONFIGURATIONS, NO_MARKINGS, PATH_TO_MODEL
]
