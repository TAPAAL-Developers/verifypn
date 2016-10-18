"""The system interface."""
from getopt import getopt
from messages import (HELP_MESSAGE,
                      ARGUMENTS_NOT_UNDERSTOOD,
                      MODELS_NOT_FOUND
                      )
from helpers import *  # NOQA
import helpers
from datetime import datetime
from time import sleep


def confirm_settings(options, experiment, repository):
    print('Experiment:', experiment)
    for key, value in options.items():
        print(key, value)
    print('Repository:', repository)
    confirmation = input('Are you sure you wish to run? (Y/N): ')
    if not confirmation.lower() == 'y':
        raise Exception('Experiment aborted - User cancellation.')

def get_repository():
    isDefault = input('Would you like to use the default repository? (Y/N): ')
    repository = None
    if not isDefault.lower() == 'y':
        repository = input('Please specify: ')
    return repository

def main(args):
    """The main function of the program."""
    options, remainder = (
        getopt(args, '-h',
               ['help',
                'repository=', 'modeldb=', 'expname=',
                'engine=', 'models=', 'timeout=', 'querytypes=',
                'nodes=', 'workers=', 'strategy=', 'memlimit='
                ]
               )
    )

    options = dict(options)

    # Default values
    my_directory = os.path.dirname(os.path.abspath(__file__))
    engine = 'distczero'
    repository = '/'.join(my_directory.split('/')[0:-2])
    modeldb = '/user/smni12/launchpad/modelDatabase/modelconfFiles/'
    conf_file = 'modelDBconf'
    timeout = '60'
    querytypes = ['CTLCardinality']
    nodes = '1'
    workers = '4'
    strategy = 'DFS'
    memlimit = '1024000000'

    if remainder:
        print(ARGUMENTS_NOT_UNDERSTOOD)

    # Help
    if '-h' in options or '--help' in options:
        print(HELP_MESSAGE)
    else:
        if '--repository' in options:
            repository = options['--repository']
        else:
            options['--repository'] = repository

        if '--engine' in options:
            engine = options['--engine']
        else:
            options['--engine'] = engine

        if '--models' in options:
            conf_file = options['--models'].split('/')[-1]
        else:
            options['--models'] = conf_file
        try:
            models = get_models(options['--models'], modeldb)
        except Exception as ex:
            print(ex)
            print(MODELS_NOT_FOUND)

        if '--timeout' in options:
            timeout = options['--timeout']
        else:
            options['--timeout'] = timeout

        if '--querytypes' in options:
            querytypes = options['--querytypes'].split(',')

        if '--nodes' in options:
            nodes = options['--nodes']
        else:
            options['--nodes'] = nodes

        if '--workers' in options:
            workers = options['--workers']
        else:
            options['--workers'] = workers

        if '--strategy' in options:
            strategy = options['--strategy']
        else:
            options['--strategy'] = strategy

        if '--memlimit' in options:
            # Memory limit specified in kb
            memlimit = options['--memlimit']
            if int(memlimit) < 10240 or int(memlimit) > 1024000000:
                raise Exception('Memory limit out of bounds. Must exceed 10MB, but can be no higher than 800GB.')
        else:
            options['--memlimit'] = memlimit

        if '--expname' in options:
            experiment = options['--expname']
        else:
            experiment = input('Supply a name for the experiment: ')

        if '--repository' in options:
            repository = options['--repository']

        helpers.repository = repository
        queries_to_run_per_model = 16
        confirmed = confirm_settings(options, experiment, repository)

        # p, outstr, errstr = compile(engine)
        compile_job_id = 1 # get_job_id(outstr)
        slurm_count = 0
        timestamp = str(datetime.now()).split(':')[:2]
        timestamp = ':'.join(timestamp).replace(' ','T')

        for model in models:
            for querytype in querytypes:
                for i in range(1, queries_to_run_per_model+1):
                    call_slurm(engine=engine, repository=repository,
                               model=model, querytype=querytype,
                               timeout=timeout, nodes=nodes,
                               workers=workers, strategy=strategy,
                               query_number=str(i), memlimit=memlimit, timestamp=timestamp,
                               compile_job_id=compile_job_id, experiment=experiment,
                               conf_file=conf_file)
                    slurm_count += 1
                    sleep(1)

