from commands import * # NOQA
from interface import run # NOQA
import nose  # NOQA
import os


class TestCommands():
    """ Tests for the commands module. """
    def test_select_only_specified_columns_returned(self):
        some_dict = {'c1': "some_value", 'c2': "some_value"}
        expected = [{'c1': "some_value"}]
        assert (select(from_=[some_dict], columns=['c1']) == expected)

    def test_sort_rows_sorted_by_columns_in_the_given_order(self):
        rows = [
            {'c1': 2, 'c2': 1, 'c3': 1},
            {'c1': 1, 'c2': 2, 'c3': 1},
            {'c1': 1, 'c2': 1, 'c3': 1}]

        sorted_rows = [
            {'c1': 1, 'c2': 1, 'c3': 1},
            {'c1': 1, 'c2': 2, 'c3': 1},
            {'c1': 2, 'c2': 1, 'c3': 1}]

        sorted_rows2 = [
            {'c1': 1, 'c2': 1, 'c3': 1},
            {'c1': 2, 'c2': 1, 'c3': 1},
            {'c1': 1, 'c2': 2, 'c3': 1}]

        assert sort(rows, ['c2', 'c1']) == sorted_rows2
        assert sort(rows, ['c1', 'c2']) == sorted_rows


class TestInterface():
    def setUp(self):
        self.all_rows = [
            {'c1': '1', 'c3': '3', 'c2': '2'},
            {'c1': '3', 'c3': '1', 'c2': '2'},
            {'c1': '1', 'c3': '2', 'c2': '3'},
            {'c1': '2', 'c3': '1', 'c2': '3'}
        ]

        self.all_sorted_c1_rows = [
            {'c1': '1', 'c3': '3', 'c2': '2'},
            {'c1': '1', 'c3': '2', 'c2': '3'},
            {'c1': '2', 'c3': '1', 'c2': '3'},
            {'c1': '3', 'c3': '1', 'c2': '2'}
        ]

        self.rows_c1 = [
            {'c1': '1'},
            {'c1': '3'},
            {'c1': '1'},
            {'c1': '2'}
        ]      

    def test_select_all_from_file(self):
        args = ['--select=*', '--from=tests/sample-comma.csv']

        assert run(args) == self.all_rows

    def test_select_one_column_from_file(self):
        args = ['--select=c1', '--from=tests/sample-comma.csv']

        assert run(args) == self.rows_c1

    def test_select_more_than_one_column_from_file(self):
        args = ['--select=c1,c2,c3', '--from=tests/sample-comma.csv']

        assert run(args) == self.all_rows

    def test_select_from_file_with_different_delimiter(self):
        args = ['--select=*', '--from=tests/sample-semicolon.csv',
            "--delimiter=;"]

        assert run(args) == self.all_rows

    def test_select_from_file_sorted_by_one_column(self):
        args = ['--select=*', '--from=tests/sample-semicolon.csv',
            '--delimiter=;', '--sortedby=c1']

        assert run(args) == self.all_sorted_c1_rows


    def test_select_from_file_sorted_by_more_than_one_column(self):
        args = ['--select=*', '--from=tests/sample-semicolon.csv',
            '--delimiter=;', '--sortedby=c1,c2,c3']

        assert run(args) == self.all_sorted_c1_rows

    def test_select_from_file_to_file(self):
        to_file = 'to_file.csv'
        args = ['--select=*', '--from=tests/sample-comma.csv',
            '--to=tests/' + to_file]

        args_exp = ['--select=*', '--from=tests/' + to_file]

        run(args)
        assert run(args_exp) == self.all_rows

