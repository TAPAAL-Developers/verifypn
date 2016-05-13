# Take a run and and for each printed_formula annotate the printed_formula information

from export_keys import FORMULA_PRINT

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
        'Query No. EG': no_eg,
        'Query No. EF': no_ef,
        'Query No. EX': no_ex,
        'Query No. EU': no_eu,

        'Query No. AG': no_ag,
        'Query No. AF': no_af,
        'Query No. AX': no_ax,
        'Query No. AU': no_au,

        'Query No. OR': no_disjunc,
        'Query No. AND': no_conjunc,
        'Query No. Negation': no_negation,
        'Query Nested Depth': get_nested_depth(printed_formula)
    }
    return result

def annotate(run):
    for query in run:
        query_info = get_query_inf(query[FORMULA_PRINT])
        query.update(query_info)

