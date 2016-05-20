# Take a run and and for each printed_formula annotate the printed_formula information

from export_keys import FORMULA_PRINT

QUERY_NO_EG = 'Query No. EG'
QUERY_NO_EF = 'Query No. EF'
QUERY_NO_EX = 'Query No. EX'
QUERY_NO_EU = 'Query No. EU'

QUERY_NO_AG = 'Query No. AG'
QUERY_NO_AF = 'Query No. AF'
QUERY_NO_AX = 'Query No. AX'
QUERY_NO_AU = 'Query No. AU'

QUERY_NO_OR = 'Query No. OR'
QUERY_NO_AND = 'Query No. AND'
QUERY_NO_NEGATION = 'Query No. Negation'
QUERY_NESTED_DEPTH = 'Query Nested Depth'

ANNOTATIONS = [
    QUERY_NO_EG, QUERY_NO_EF, QUERY_NO_EX, QUERY_NO_EU,
    QUERY_NO_AG, QUERY_NO_AF, QUERY_NO_AX, QUERY_NO_AU,
    QUERY_NO_OR, QUERY_NO_AND, QUERY_NO_NEGATION, QUERY_NESTED_DEPTH
]

def get_nested_depth(printed_formula):
    # The maximum depth of the query
    nested_depth = 0
    current_nested_depth = 0
    for ch in printed_formula:
        if ch == '(':
            current_nested_depth += 1
        elif ch == ')':
            current_nested_depth -= 1
        if current_nested_depth > nested_depth:
            nested_depth = current_nested_depth
    return nested_depth

def get_query_inf(printed_formula):
    no_eg = printed_formula.count('EG')
    no_ef = printed_formula.count('EF')
    no_ex = printed_formula.count('EX')
    no_eu = printed_formula.count('E') - no_eg - no_ef - no_ex 

    no_ag = printed_formula.count('AG')
    no_af = printed_formula.count('AF')
    no_ax = printed_formula.count('AX')
    no_au = printed_formula.count('A') - no_ag - no_af - no_ax 

    no_disjunc = printed_formula.count('OR')
    no_conjunc = printed_formula.count('AND')

    no_negation = printed_formula.count('!')
    
    result ={
        QUERY_NO_EG: no_eg,
        QUERY_NO_EF: no_ef,
        QUERY_NO_EX: no_ex,
        QUERY_NO_EU: no_eu,

        QUERY_NO_AG: no_ag,
        QUERY_NO_AF: no_af,
        QUERY_NO_AX: no_ax,
        QUERY_NO_AU: no_au,

        QUERY_NO_OR: no_disjunc,
        QUERY_NO_AND: no_conjunc,
        QUERY_NO_NEGATION: no_negation,
        QUERY_NESTED_DEPTH: get_nested_depth(printed_formula)
    }
    return result

def annotate(run):
    for query in run:
        query_info = get_query_inf(query[FORMULA_PRINT])
        query.update(query_info)

