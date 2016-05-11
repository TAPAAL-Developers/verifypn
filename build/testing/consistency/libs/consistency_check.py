import csv
import sys
from result_keys import FORMULA, QUERY_RESULT

actual = sys.argv[1]
expected = sys.argv[2]

def pretty_print(inconsistencies):
	header_format = 'Formula: {{{0}}}'.format(FORMULA)
	result_format = 'Actual: {ACTUAL} - Expected: {EXPECTED} \n'

	for inconsistency in inconsistencies:
		try:
			print(header_format.format(**inconsistency))
			print(result_format.format(**inconsistency))
		except:
			import pdb; pdb.set_trace()

def consistency_check_from_lists(test_data, consistency_data):
	inconsistencies = []
	answered_test_rows = [test for test in test_data if test[QUERY_RESULT] != '']
	no_answers = len(answered_test_rows)
	no_possible_answers = len(test_data)
	answers_unverified = 0
	for test in answered_test_rows:
		if any([test[FORMULA].strip() == consistency_entry[FORMULA].strip() for consistency_entry in consistency_data]):
			# print('Consistency entry available for:', test[FORMULA])
			for consistency_entry in consistency_data:
				if (test[FORMULA].strip() == consistency_entry[FORMULA].strip() and consistency_entry[QUERY_RESULT] != '' and
					consistency_entry[QUERY_RESULT] != test[QUERY_RESULT]
					):
					inconsistency = {
						FORMULA: test[FORMULA], 
						'ACTUAL': test[QUERY_RESULT], 
						'EXPECTED': consistency_entry[QUERY_RESULT] 
					}
					inconsistencies.append(inconsistency)
		else:
			answers_unverified += 1
			# print('No consistency entry available for:', test[FORMULA])

	return inconsistencies, no_answers, no_possible_answers, answers_unverified

def consistency_check_from_files(test_file, consistency_file):
	with open(test_file, 'r') as f:
	    reader = csv.DictReader(f, delimiter=',')
	    test_rows = list(reader)
	with open(consistency_file, 'r') as f:
	    reader = csv.DictReader(f, delimiter=',')
	    consistency_rows = list(reader)

	# for e in consistency_rows:
	#	print(e[FORMULA].strip())
	return consistency_check_from_lists(test_data=test_rows, consistency_data=consistency_rows)


# Global execution of consistency test
inconsistencies, no_answers, no_possible_answers, answers_unverified = consistency_check_from_files(test_file=actual, consistency_file=expected)

print('Number of Answers:', '{0}/{1}'.format(no_answers, no_possible_answers))
print('Number of Verified Answers:', '{0}/{1}'.format(no_answers - answers_unverified, no_possible_answers))
if not inconsistencies:
	print('No inconsistencies found')
	sys.exit(0)
else:
	print('The following inconsistencies were found:')
	# Maybe some output in some file
	pretty_print(inconsistencies)
	sys.exit(1)
