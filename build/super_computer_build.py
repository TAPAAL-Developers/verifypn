# Call compile
# Run tests
# Verify
# Analyse 

import os
# Find which repository this build script belongs to
my_directory = os.path.dirname(os.path.abspath(__file__))

repository = my_directory.split('/')[-2]
repository_location = '/'.join(my_directory.split('/')[0:-1])
print(my_directory)
print(repository)
print(repository_location)

# Load default settings
workers = 4
memlimit = 'unlimited'
strategy = 'dfs'
engine = 'distczero'
nodes = 1
querytypes = 'CTLCardinality'
timeout = 60
conf_file = 'completemodelconf'