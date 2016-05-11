import unittest
from output_keys import NO_TRANSITIONS
from export_keys import NO_TRANSITIONS as EX_NO_TRANSITIONS, MISCELLANEOUS
from output_parsing import parse

class OutputParsingTests(unittest.TestCase):

    def test_parse_output_1_line_with_output_key(self):
        data = ' Some data'
        lines = [NO_TRANSITIONS + data]
        expected = {EX_NO_TRANSITIONS: data.strip(), MISCELLANEOUS: ''}
        actual = parse(lines)
        self.assertEqual(actual, expected)

    def test_parse_output_multiple_lines_without_output_key(self):
        data = ' Some data\n'
        lines = [data, data]
        expected = {MISCELLANEOUS: data + data}
        actual = parse(lines)
        self.assertEqual(actual, expected)
        
# Must be at bottom
if __name__ == '__main__':
    unittest.main()