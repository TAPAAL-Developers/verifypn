# Input: modelconf, query max"
# Konstruér run-query kald
# Lav fil der laver et run-query for dist
# men 64 for den sekventielle

import sys

binary = sys.argv[1]
alg = sys.argv[2]
modelconf = sys.argv[3]
no_queries = sys.argv[4]
time_out = sys.argv[5]
name = sys.argv[6]

model_database = "/user/smni12/launchpad/modelDatabase/allModels"
model_file_template = model_database + "/{model_name}/model.pnml"
query_file_template = model_database + "/{model_name}/CTLCardinality.xml"

# Open modelconf
with open(modelconf, 'r'):
    model_names = modelconf.readlines()

run_query_template = (
    "bash run-query.sh {model_file} {query_file} {query_number}"
    "{binary} {alg} {output_file}"
)

run_query_commands = []

for model_name in model_names:
    for query_number in range(1, int(no_queries) + 1):
        run_query_kwargs = {
            'model_file': model_file_template.format(model_name=model_name),
            'query_file': query_file_template.format(model_name=model_name),
            'query_number': query_number,
            'binary': binary,
            'alg': alg,
            'output_file': '-'.join([name, model_name, query_number])
        }
        run_query_commands.append(
            run_query_template.format(**run_query_kwargs)
        )

slurm_header = '\n'.join((
    "#!/bin/sh",
    "#SBATCH --no-requeue",
    "#SBATCH --mem 1000000",
    "#SBATCH --nodes=1",
    "#SBATCH --partition=production",
    "#SBATCH --ntasks=64",
    "#SBATCH --mail-type=ALL # Type of email notification- BEGIN,END,FAIL,ALL",
    "#SBATCH --mail-user={{d803f16@cs.aau.dk}}",
    "#SBATCH --time={time_out}".format(time_out=time_out)
))

queries_per_slurm_job = 64 if alg in ['local', 'czero'] else 1000000

file_number = 0
remaining_commands = run_query_commands
slurm_job_files = []

while remaining_commands:
    no_commands_in_file = 64 if len(remaining_commands) > 64 else len(remaining_commands)
    commands_to_write_to_file = remaining_commands[:no_commands_in_file]

    slurm_job_file_name = name + '.slurmjob'
    with open(slurm_job_file_name, 'w') as slurm_job_file:
        slurm_job_file.write(slurm_header)
        slurm_job_file.write('\n'.join(commands_to_write_to_file))

    slurm_job_files.append(slurm_job_file_name)
    file_number += 1
    remaining_commands = remaining_commands[no_commands_in_file:]
