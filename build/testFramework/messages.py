"""Contains the messages given by the program."""

HELP_MESSAGE = (
    'The following commands are available: \n'
    '  --help, --h or -h: Prints this help message. \n \n'
    '  --engine= or --e=: Sets the engine. \n'
    '    Supported: czero, sharedczero, and distczero. \n \n'

    '  --models= or --m=: Specifies a file containing a line \n'
    '      separated list of model names to test on. \n \n'

    '  --timeout= or --t=: Specifies the timeout time in WHAT?. \n \n'

    '  --querytype= or --q: Specifies the query type. \n'
    '    Supported: CTLCardinality, CTLFireability, CTLFireabilitySimple, \n'
    '      ReachabilityBounds, ReachabilityCardinality, ReachabilityFireability, \n'
    '      ReachabilityDeadlock'

    'Examples: '
    '  python3 czerotest.py --engine=czero --models=testmodelconf' 
    '    --timeout=60 --querytype=CTLCardinality'
    '  python3 czerotest.py --engine=sharedczero --models=testmodelconf'
    '   --timeout=60 --querytype=CTLCardinality --workers=4 --strategy=DFS'
    '  python3 czerotest.py --engine=onthefly --models=testmodelconf'
    '   --timeout=60 --querytype=ReachabilityCardinality --workers=4 --strategy=DFS'
)

ARGUMENTS_NOT_UNDERSTOOD = (
    'One or more arguments were not understood.'
)

MODELS_NOT_FOUND = (
    'One or more models could not be found in ModelDB'
)
