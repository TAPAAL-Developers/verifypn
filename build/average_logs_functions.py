
from export_keys import FORMULA, TOTAL_EVALUATION_TIME, FORMULA_PRINT
from decimal import Decimal
from os.path import isdir
from os import listdir
from aggregate_logs_functions import aggregate_logs, write_to_csv


def get_runs(directory_of_runs):
    if not isdir(directory_of_runs):
        raise Exception(directory_of_runs + ' is not a directory.') 

    runs = [aggregate_logs(directory_of_runs + '/' + directory) 
            for directory in listdir(directory_of_runs) 
            if isdir(directory_of_runs + '/' + directory)]
    return runs


def get_shared_queries(runs):
    shared_queries = []
    for run in runs:
        for query in run:
            # The query must exist in all other runs
            if all([any([query[FORMULA] == other_runs_query[FORMULA] for other_runs_query in other_run]) for other_run in runs]):
                shared_queries.append(query)
    return shared_queries

def distinct_queries(run):
    distinct = []
    for query in run:
        if not any([query[FORMULA] == other_query[FORMULA] for other_query in distinct]):
            distinct.append(query)
    return distinct

def get_average_run(all_runs, columns_to_be_averaged, distinct_run):
    # Note that this changes the input distinct_run list
    
    formula_avg = 0
    for column in columns_to_be_averaged:
        for formula in [query[FORMULA] for query in distinct_run]:
            formula_sum = Decimal('0')
            for run in all_runs:
                for query in run:
                    if query[FORMULA] == formula:
                        formula_sum += Decimal(query[column])
            formula_avg_old = formula_avg
            formula_avg = formula_sum/Decimal(len(all_runs))
            for query in distinct_run:
                if query[FORMULA] == formula:
                    query[column] = formula_avg
    
    for column in columns_to_be_averaged:
        s = sum([query[column] for query in distinct_run])
        print(column, 'average is', s)
    return distinct_run

def average_runs(directory_of_runs, columns_to_be_averaged):
    runs = get_runs(directory_of_runs)
    shared_queries = get_shared_queries(runs)
    distinct_run = distinct_queries(shared_queries)

    print('\nRun', 0)
    averaged_run = get_average_run(all_runs=[runs[0]], columns_to_be_averaged=columns_to_be_averaged, distinct_run=distinct_run)
    for i in range(1, len(runs)):
        print('\nRun', i)
        averaged_run = get_average_run(all_runs=runs[:i], columns_to_be_averaged=columns_to_be_averaged, distinct_run=distinct_run)
    return averaged_run
