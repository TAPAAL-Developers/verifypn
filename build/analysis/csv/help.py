help_command_long = 'python3 csvutils.py --help'
help_command_short = 'python3 csvutils.py -h'
basic_syntax = ('python3 csvutils.py --select=<columns> ' +
                '--from=<csv file> [--delimiter=<some delimiter>] ' +
                '[--sortedby=<columns>] [--to=<some output file>]' +
                '[--todelimiter=<some delimiter>]'
                )


def print_help():
    print('csvutils can parse a CSV file, remove some columns,' +
          ' sort the rows and output them to a new file.')
    print('<------ Usage ------>')
    print_see_help()
    print_basic_syntax()


def print_see_help():
    print('To see this help menu, write:')
    print(help_command_long)
    print(help_command_short)
    print('')


def print_columns():
    print('The columns specified for the --select and --sortedby arguments ' +
          'must be a comma separated list of column ' +
          'names in the source CSV file.')
    print('The rows will be sorted according to the order of the columns ' +
          'specified in the --sortedby argument.')
    print('If all columns are desired a asterisk (*) can be used ' +
          'instead of column names for --select.')
    print('')


def print_delimiters():
    print('By default the delimiter of the source CSV file is set to a comma' +
          ' but may be changed using the --delimiter argument.')
    print('By default the delimiter of the target CSV file is the same ' +
          'as the source but may be changed using the --todelimiter argument.')
    print('')


def print_to():
    print('If the --to argument is not set to a file, ' +
          'the result of the command is printed in the terminal.')
    print('')


def print_example():
    print('<------ Example ------>')
    print("python3 csvutils.py --select=* from=example.csv --delimiter=';' " +
          "--sortedby=column1, column2 --to=example_target.csv " +
          "--todelimiter=';'")

def print_basic_syntax():
    print('The following syntax is used for converting' +
          'a CSV file to another format:')
    print(basic_syntax)
    print_columns()
    print_delimiters()
    print_to()
