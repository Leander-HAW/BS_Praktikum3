#!/bin/bash

# Dieses Skript fuehrt die Simulation für alle Ersetzungsalgorithmen, unterschiedliche
# Belegungen des zu sortierenden Felds und alle Framegroessen durch

# seed values fuer LINUX Zufallszahlengenerator"
# seed_values="2806 225 353 540 964 1088 1205 1288 2364 2492 2601 2680 5015 5321 6748 7413 7663 8555 8897 9174 9838"

# seed values fuer OS unabhaengigen Zufallszahlenerator
#seed_values="2806 1 1079 1262 135 1351 1356 1379 1502 1556 1685 1927 224 2339 2583 2704 2996 3125 3218 3222 3248 3277 3514 3758 40 4196 4212 4319 4388 4398 4565 4763 4827 4837 4949 4969 5007 5115 5204 5324 5386 5483 5499 5576 5763 6040 6209 6256 630 6546 6765 703 7184 7197 7234 7344 7438 7500 761 7699 7711 7749 8000 8087 858 9008 902 91 9352 9532 9961 9999"

# seed values zum vergleich mit der Musterloesung
seed_values="2806"

#seed_values="2807"

page_sizes="8 16 32 64"
#page_rep_algo="FIFO CLOCK AGING"
page_rep_algo="FIFO CLOCK AGING"
search_algo="quicksort"
search_algo="quicksort bubblesort"

ref_result_dir="./LogFiles_mit_SEED_2806"

# Simulation summary file
all_results=all_results

# clean up result file 
rm -rf results $all_results
mkdir results

for s in $page_sizes ; do
    # page size be be seed via C define Statement
    # compile 
    make clean
    make VMEM_PAGESIZE=$s 

    # iterate for all page replacement algorithms and all seed values
    for a in $page_rep_algo ; do
		for sa in $search_algo ; do 
			for seed in $seed_values ; do
				echo "Run simulation for seed = $seed search algo $sa and page rep. algo $a and page size $s"

				# delete all shared memory areas
				# ipcrm -ashm

				# start memory manageer
				./bin/mmanage -$a  &
				 mmanage_pid=$!

				 sleep 1  # wait for mmange to create shared objects

				 # start application, save pagefaults and results files for current seed 
				 outputfile="./results/output_${seed}_${sa}_${a}_${s}.txt"
				 ./bin/vmappl -$sa -seed=$seed > $outputfile

				 kill -s SIGINT $mmanage_pid
				 wait $mmanage_pid

				 # save pagefaults 
				 pagefaults=$(grep "Page fault" logfile.txt | tail -n1 | awk "{ print \$3 }")
				 pagefaults="${pagefaults//,/}"
				 globalcount=$(grep "Global count" logfile.txt | tail -n1 | awk "{ print \$6 }")
				 globalcount="${globalcount//:/}"
				 printf "seed = %6i page_rep_algo = %7s search_algo = %12s pagesize = %4i pagefaults %7s global_count %7s\n" "$seed" "$a" "$sa" "$s" "$pagefaults" "$globalcount" >> $all_results

				 # save result files and compare for seed=2806
				 mv logfile.txt ./results/logfile_${seed}_${sa}_${a}_${s}.txt  
				 if [ "$seed" = "2806" ]; then
					 echo "=============== COMPARE results for logfile_${sa}_${a}_${s}.txt =================="
					 diff -b -w results/logfile_${seed}_${sa}_${a}_${s}.txt  ${ref_result_dir}/logfile_${seed}_${sa}_${a}_${s}.txt
					 echo "=============== COMPARE results for output_${sa}_${a}_${s}.txt =================="
					 diff -b -w results/output_${seed}_${sa}_${a}_${s}.txt   ${ref_result_dir}/output_${seed}_${sa}_${a}_${s}.txt
					 echo "============================================================================"
				 fi
			done
		done
    done
done
# EOF

