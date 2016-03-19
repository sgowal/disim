#!/bin/bash
#$ -N calibration_fitness
#$ -S /bin/bash
#$ -cwd
#$ -j y

# set the job base directory (a unique directory in /scratch)
JOB_BASE_DIR=/scratch/${USER}/job${JOB_ID}
 
# create the working directory
if [ ! -d $JOB_BASE_DIR ]
then
   echo "(`date`) Create base directory $JOB_BASE_DIR"
   mkdir -p $JOB_BASE_DIR
fi

# Grab arguments
d="./calibrate_IDM_MOBIL_fitness_SMP.py"
d="$d --record-path=$JOB_BASE_DIR"
while [ $# -gt 0 ]; do
  d=" $d $1"
  shift
done

# Get fitness
echo `$d` > logs/job${JOB_ID}.out

# delete base dir
rm -r $JOB_BASE_DIR
