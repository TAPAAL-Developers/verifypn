def get_key_value(log_file_line):
    log_file_line = log_file_line.replace(':', '')
    log_file_line = log_file_line.replace('\n', '')
    line_split = log_file_line.split(' ')

    key = None
    value = None
    result = []

    if 'analysis' in log_file_line.lower():

        if 'modelname' in log_file_line.lower():
            key = 'Model Name'
            value = line_split[-1]
        elif 'transistions' in log_file_line.lower():
            key = 'No. Transitions'
            value = line_split[-1]
        elif 'places' in log_file_line.lower():
            key = 'No. Places'
            value = line_split[-1]
        # elif 'modefile' in line_split[1].lower():  # Excluded upon request from Soren
        #     key = 'Model File'
        #     value = line_split[-1]
        else:
            pass
            # raise Exception(("Unrecognized analysis field: " + str(line_split)))
    elif 'bad_alloc' in log_file_line.lower():
        key = 'Failure Cause'
        value = 'Memory'
    elif 'error' in log_file_line.lower():  
        key = 'Error'
        value = ' '.join(line_split[1:])

    elif ('time' in log_file_line.lower() and
          'timeout' not in log_file_line.lower() and
          'worker' not in log_file_line.lower()):
        if 'parse model' in log_file_line.lower():
            key = 'Model Parsing Time'
            value = line_split[-2]
        elif 'parse of ctl query' in log_file_line.lower():
            key = 'Query Parsing Time'
            value = line_split[-2]

        elif 'search' in line_split[1].lower():
            key = 'Query Search Time'
            value = line_split[-2]

        elif 'total' in line_split[1].lower():
            key = 'Total Evaluation Time'
            value = line_split[-2]
        else:
            raise Exception(("Unrecognized time field: " + str(line_split)))
    elif 'formula' in log_file_line.lower():
        key = 'Formula'
        value = line_split[1]
        result.append((key, value))
        key = 'Query Result'
        value = line_split[2]
    elif 'algorithm' in log_file_line.lower():
        key = 'Algorithm'
        value = line_split[-1]

    elif 'query type' in log_file_line.lower():
        key = 'Query Type'
        value = line_split[-1]
    elif 'search' in log_file_line.lower():
        key = 'Search Type'
        value = line_split[-1]

    elif 'data' in log_file_line.lower():
        key = 'No. Configurations'
        value = line_split[2]
        result.append((key, value))
        key = 'No. Markings'
        value = line_split[4]
    else:
        pass  # Discard other input
    if (key, value) != (None, None):
        result.append((key, value))
    return result

def get_query_inf(query):
    no_eg = query.count('EG')
    no_ef = query.count('EF')
    no_ex = query.count('EX')
    no_eu = query.count('E') - no_eg - no_ef - no_ex 

    no_ag = query.count('AG')
    no_af = query.count('AF')
    no_ax = query.count('AX')
    no_au = query.count('A') - no_ag - no_af - no_ax 

    no_disjunc = query.count('OR')
    no_conjunc = query.count('AND')

    no_negation = query.count('!')

    nested_depth = 0
    c_nested_depth = 0
    for ch in query:
        if ch == '(':
            c_nested_depth += 1
        elif ch == ')':
            c_nested_depth -= 1
        if c_nested_depth > nested_depth:
            nested_depth = c_nested_depth
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
        'Query Nested Depth': nested_depth,

    }

    return result


def parse(log_file):
    result = {}
    with open(log_file, 'r') as lf:
        lines = lf.readlines()
    for line in lines:
        for (key, value) in get_key_value(line):
            result[key] = value

    read_query = False
    query = ''
    for line in lines:
        if read_query:
            query += line
        if 'Query start' in line:
            read_query = True
        if 'Query end' in line:
            read_query = False
    query = query.replace('\n', ' ')
    q_inf = get_query_inf(query)
    result.update(q_inf)
    return result
