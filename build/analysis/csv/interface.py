import os
from os.path import abspath
from getopt import getopt

from commands import (
    from_, select, sort, to, equality, inequality,
    discretize_no_places, discretize_no_transitions,
    assign_failure_cause
)
from log_file_parsing import parse

from help import print_help

def print_run(run):
    print('---------------------------------------------------')
    for value in run.items():
        print(value)


def print_stats(rows):
    print('<-- Result Set Size -->')
    print('Rows: ' + str(len(rows)))
    print('Columns: ' + str(len(rows[0])))


def print_report(rows):
    no_rows_total = len(rows)
    print('<-- Report -->')
    print('Total number of queries: ' + str(no_rows_total))

    no_completed_rows = len([row for row in rows if row['Query Result'] != ''])
    print('Number of completed queries: {0} ({1:.2f}%)'.format(
        no_completed_rows, 100 * (no_completed_rows) / no_rows_total))

    no_failed_rows = no_rows_total - no_completed_rows
    print('Number of failed queries: {0} ({1:.2f}%)'.format(
        no_failed_rows, 100 * (no_failed_rows / no_rows_total)))

    no_failed_rows_mem = len([row for row in rows if row['Failure Cause'] == 'Memory'])
    print('Number of failed queries due to memory: {0} ({1:.2f}%)'.format(
        no_failed_rows_mem, 100 * (no_failed_rows_mem) / no_rows_total))


def run(args):
    options, remainder = getopt(args, '-h',
                                ['help',
                                 'where=',
                                 'select=',
                                 'from=',
                                 'fromdelimiter=',
                                 'sortedby=',
                                 'to=',
                                 'todelimiter=',
                                 'limit=',
                                 'discretize'])
    options = dict(options)
    print(options)
    source_delimiter = ','
    target_delimiter = ','
    rows = None
    if '--help' in options or '-h' in options and len(options) == 1:
        print_help()

    # Delimiters
    if '--fromdelimiter' in options:
        source_delimiter = options['--fromdelimiter']

    if '--todelimiter' in options:
        target_delimiter = options['--todelimiter']

    if '--from' in options:
        from_path = options['--from']
        rows = from_(path=from_path, delimiter=source_delimiter)
        columns = rows[0].keys()

    if '--where' in options:
        if '==' in options['--where']:
            operands = options['--where'].split('==')
            operator = equality
        elif '!=' in options['--where']:
            operands = options['--where'].split('!=')
            operator = inequality

        left_operand = operands[0]
        right_operand = operands[1]

        if (left_operand in columns and
                right_operand in columns):
            rows = [row
                    for row in rows
                    if operator(row[left_operand], row[right_operand])]
        elif left_operand in columns:
            rows = [row
                    for row in rows
                    if operator(row[left_operand], right_operand)]
        elif right_operand in columns:
            rows = [row
                    for row in rows
                    if operator(left_operand, row[right_operand])]

    if '--sortedby' in options:
        columns_to_sort_by = options['--sortedby'].split(',')
        rows = sort(rows, columns_to_sort_by)

    if '--limit' in options:
        limit = int(options['--limit'])
        rows = rows[:limit]

    assign_failure_cause(rows)
    if '--discretize' in options:
        discretize_no_places(rows)
        discretize_no_transitions(rows)

    if '--select' in options:
        columns_to_select = options['--select'].replace("'", "").split(',')
        print(columns_to_select)
        rows = select(rows, columns_to_select)

    if '--unique' in options:
        raise NotImplementedError('--unique not implemented')
        unique_rows = []
        for row in rows:
            if not any([urow['Error'] == row['Error'] for urow in new_rows]):
                new_rows.append(row)
        rows = unique_rows

    if '--to' in options:
        to_path = options['--to']
        to(rows, to_path, target_delimiter)

    #for row in rows:
    #    print(row)
    print_stats(rows)
    try:
        print_report(rows)
    except:
        print(
            'Unable to print report. '
            'Source may not contain log information.')

    print('--discretize' in options)
    return rows
