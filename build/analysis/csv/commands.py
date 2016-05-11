import csv
from itertools import groupby
from operator import itemgetter
import os
from os.path import abspath
from log_file_parsing import parse
from intervals import interval_no_places, interval_no_transitions


"""
Relevant documentation:
https://docs.python.org/3/library/csv.html
"""

def equality(left, right):
    return left == right

def inequality(left, right):
    return not equality(left, right)

def select(from_, columns):
    """
    Takes a list of rows and returns it with only the specified columns.
    """
    result = []
    for row in from_:
        new_dict = {}
        for key, value in row.items():
            if key in columns:
                new_dict[key] = value
        result.append(new_dict)
    return result


def sort(rows, by):
    result = []
    if by == []:
        return rows
    else:
        s = sorted(rows, key=itemgetter(by[0]))
        for k, g in groupby(s, key=itemgetter(by[0])):
            result += sort(list(g), by[1:])

    return result


def _normalize(rows):
    # Not all rows will have the same columns.
    # This function ensures that they do.
    columns = set()
    for row in rows:
        columns = columns.union(set(row.keys()))
    for row in rows:
        for column in columns:
            if column not in row:
                row[column] = ''


def from_(path, delimiter=','):
    # path = os.path.abspath(path)
    if path.endswith('.csv'):  # Single CSV file
        with open(path, 'r') as csv_file:
            reader = csv.DictReader(csv_file, delimiter=delimiter)
            rows = list(reader)
    elif path.endswith('.log'):  # Single log file
        rows = [parse(path)]
    elif os.path.isdir(path):  # Directory of log files
        file_names = os.listdir(path)
        log_file_paths = [path + '/' + fn
                          for fn in file_names
                          if fn.endswith('.log')]
        rows = [parse(lfp) for lfp in log_file_paths]
    else:
        raise Exception(
            'Unrecognized source type "{0}" specified. '
            'Only .csv files, .log files, and directories of '
            'log files are supported.'.format(path))
    _normalize(rows)
    return rows


def to(rows, file_path, delimiter=','):
    fieldnames = set()
    for row in rows:
        fieldnames = fieldnames.union(set(row.keys()))
    with open(file_path, 'w') as file:
        writer = csv.DictWriter(file,
                                fieldnames=fieldnames,
                                delimiter=delimiter)
        writer.writeheader()
        for row in rows:
            writer.writerow(row)


def discretize_no_places(rows):
    for row in rows:
        if 'No. Places' in row:
            row['No. Places'] = interval_no_places(int(row['No. Places']))


def discretize_no_transitions(rows):
    for row in rows:
        if 'No. Transitions' in row:
                row['No. Transitions'] = interval_no_transitions(int(row['No. Transitions']))


def assign_failure_cause(rows):
    for row in rows:
        if 'Failure Cause' in row and 'Query Result' in row:
            if row['Query Result'] == 'T':
                row['Failure Cause'] = 'NotMemory'
            elif row['Query Result'] == 'F':
                row['Failure Cause'] = 'NotMemory'
            elif row['Failure Cause'] != 'Memory':
                row['Failure Cause'] = 'NotMemory'
