#! /bin/bash
# usage: get_res_parallel slurm-<number>.out
grep -A1 Num_files $1 |grep -v Num|grep -v "\-"