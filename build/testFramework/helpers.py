"""Assisting functions for use in interface.py."""
import os

import subprocess
from subprocess import Popen, PIPE
from datetime import datetime
import time
script_dir = os.path.dirname(os.path.realpath(__file__))

repository = None

def get_job_id(outstr):
    outstr = outstr.decode('utf-8')
    job_id = outstr.replace('Submitted batch job ', '')
    job_id = job_id.replace('\n', '')
    return job_id


def get_models(models_file):
    """Get all absolute paths to model files."""
    file = os.path.abspath(models_file)
    with open(file, 'r') as f:
        model_names = f.readlines()

    model_file_format = script_dir + '/completeModelDB/{0}/model.pnml'
    models = []

    for name in model_names:
        model_file = model_file_format.format(name)
        models.append((name.replace('\n', ''), model_file.replace('\n', '')))
    return models


def compile(engine, base='~/launchpad/'):
    """Compile the engine using preset makefile."""
    if not repository:
        makefiles = {
            'czero': base + 'verifypn/',
            'sharedczero': base + 'parallel-lockfree/',
            'distczero': base + 'verifypn-dist-ctl/',
            'classic': base + 'verifypn/',
            'onthefly': base + 'verifypnLTSmin/'
        }
        makefiledir = makefiles[engine]
    else:
        makefiledir = repository

    sbatch_command = 'sbatch compile.sh {0}'.format(makefiledir)
    p = Popen(
        sbatch_command.split(' '),
        stdout=PIPE, stderr=PIPE)
    outstr, errstr = p.communicate()
    return (p, outstr, errstr)


def get_slurm_conf(engine, output_file_params, slurm_call_params, experiment):
    slurm_conf_dir = script_dir + '/slurm_conf/'

    if engine == 'onthefly':
        conf_template_path = (
            slurm_conf_dir + 'slurm_conf_template_onthefly.sh'
        )
    elif engine == 'classic':
        conf_template_path = (
            slurm_conf_dir + 'slurm_conf_template_classic.sh'
        )
    elif engine == 'distczero':
        conf_template_path = (
            slurm_conf_dir + 'slurm_conf_template_dist.sh'
	)
    else:
        conf_template_path = (
            slurm_conf_dir + 'slurm_conf_template_czero.sh'
        )

    with open(conf_template_path, 'r') as f:
        template = f.read()

    template_params = {}
    template_params.update(slurm_call_params)
    template_params.update(output_file_params)

    conf_content = template.format(**template_params)

    conf_content_file = (
        slurm_conf_dir +
        '-'.join(output_file_params.values()) +
        '.sh'
    )

    with open(conf_content_file, 'w') as f:
        f.write(conf_content)

    return conf_content_file


def get_alg_flag(engine):
    alg_flags = {
        'czero': '-ctl czero',
        'sharedczero': '-ctl par',
        'distczero': '-ctl dist',
        'classic': '',
        'onthefly': '-o mc -c'
    }
    return alg_flags[engine]


def get_engine_path(engine, base='~/launchpad/'):
    if not repository:
        engine_paths = {
            'czero': base + 'verifypn/{0}',
            'sharedczero': base + 'parallel-lockfree/{0}',
            'distczero': base + 'verifypn-dist-ctl/{0}',
            'classic': base + 'verifypn/{0}',
            'onthefly': base + 'verifypnLTSmin/{0}'
        }
        engine_path = engine_paths[engine]
    else:
        engine_path = repository + '{0}'

    return engine_path.format('verifypn-linux64')


def get_query_path(model_file, querytype):
    query_file = '{0}.xml'.format(querytype)
    return model_file.replace('model.pnml', query_file)


def call_slurm(engine, model, querytype, timeout,
               nodes, workers, strategy, query_number,
               compile_job_id, memlimit, timestamp, 
               experiment, conf_file):
    """
    Call Slurm to queue up a batch of runs on the models.

    Takes an engine, a model file, a query type, a number of nodes, 
    a number of workers, a strategy and a query_number.
    """
    # Call Slurm Conf file
    model_name, model_file = model
    output_file_params = {
        'TIMESTAMP': timestamp,
        'ENGINENAME': engine,
        'MODELNAME': model_name,
        'QUERYCAT': querytype,
        'QUERY_NUM': query_number,
        'STRATEGY': strategy
    }

    slurm_call_params = {
        'NODES': nodes,
        'TIMEOUT': timeout,
        'OPT_MPI': '',
        'ENGINEPATH': get_engine_path(engine),
        'MODELPATH': model_file,
        'QUERYPATH': get_query_path(
            model_file=model_file,
            querytype=querytype),
        'ALGFLAG': get_alg_flag(engine),
        'STRATEGY': strategy,
        'QUERY_NUM': query_number,
        'MEMLIMIT': memlimit,
        'WORKERS': workers,
        'EXPERIMENT': experiment,
        'MODELCONF': conf_file
    }

    slurm_conf = get_slurm_conf(
        engine=engine,
        output_file_params=output_file_params,
        slurm_call_params=slurm_call_params,
        experiment=experiment)

    return Popen([
        'sbatch',
        '--dependency=afterok:' + compile_job_id,
        slurm_conf])


def get_default_values(engine):
    """Get default values for an engine."""
    nodes, workers, strategy = (None, None, None)

    if engine == 'czero':
        nodes, workers = ('1', '1')

    elif engine == 'classic':
        nodes, workers, strategy = ('1', '1', 'BestFS')

    elif engine == 'sharedczero':
        nodes = '1'

    elif engine == 'onthefly':
        nodes, strategy = ('1', '')

    return (nodes, workers, strategy)
