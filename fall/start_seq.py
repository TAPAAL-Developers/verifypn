# Input: modelconf, query max"
# Konstruér run-query kald
# Lav fil der laver et run-query for dist
# men 64 for den sekventielle

import sys
import time
import subprocesses

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
with open(modelconf, 'r') as modelconf_file:
    model_names = [model_name.replace('\n', '')
                   for model_name in modelconf_file.readlines()]

print(model_names)
run_query_template = (
    "timeout {time_out} bash /user/smni12/launchpad/master/fall/run-query.sh {model_file} {query_file} {query_number} {binary} {alg} {output_file}"
)

run_query_commands = []

for model_name in model_names:
    for query_number in range(1, int(no_queries) + 1):
        run_query_kwargs = {
            'model_file': model_file_template.format(model_name=model_name),
            'query_file': query_file_template.format(model_name=model_name),
            'query_number': query_number,
            'time_out': time_out,
            'binary': binary,
            'alg': alg,
            'output_file': '-'.join([name, model_name, str(query_number), '.log'])
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
    "#SBATCH --time={time_out}\n\n".format(
         time_out=time.strftime('%H:%M:%S', time.gmtime(int(time_out))))
))
print(slurm_header)
queries_per_slurm_job = 64 if alg in ['local', 'czero'] else 1

file_number = 0
remaining_commands = run_query_commands
slurm_job_files = []
processes = []

while remaining_commands:
    no_commands_in_file = (
    queries_per_slurm_job if len(remaining_commands) > queries_per_slurm_job 
    else len(remaining_commands)
    )

    commands_to_write_to_file = remaining_commands[:no_commands_in_file]
    print(commands_to_write_to_file)
    slurm_job_file_name = ''.join([name, str(file_number), '.slurmjob'])
    
with open(slurm_job_file_name, 'w') as slurm_job_file:
        slurm_job_file.write(slurm_header)
        slurm_job_file.write('\n\n'.join(commands_to_write_to_file))

    processes.append(subprocess.Popen(
    slurm_job_files.append(slurm_job_file_name)
    file_number += 1
    remaining_commands = remaining_commands[no_commands_in_file:]
print(slurm_job_files)
