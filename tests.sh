#!/bin/bash

./freemem -w -w 1 > ./results/out_1_fw_nfw.txt
./freemem -w -n 1 > ./results/out_1_fw_nfnw.txt
./freemem -n -w 1 > ./results/out_1_fnw_nfw.txt
./freemem -n -n 1 > ./results/out_1_fnw_nfnw.txt
./freemem -w -w 2 > ./results/out_2_fw_nfw.txt
./freemem -w -n 2 > ./results/out_2_fw_nfnw.txt
./freemem -n -w 2 > ./results/out_2_fnw_nfw.txt
./freemem -n -n 2 > ./results/out_2_fnw_nfnw.txt
./freemem -w -w 3 > ./results/out_3_fw_nfw.txt
./freemem -w -n 3 > ./results/out_3_fw_nfnw.txt
./freemem -n -w 3 > ./results/out_3_fnw_nfw.txt
./freemem -n -n 3 > ./results/out_3_fnw_nfnw.txt
./freemem -w -w 4 > ./results/out_4_fw_nfw.txt
./freemem -w -n 4 > ./results/out_4_fw_nfnw.txt
./freemem -n -w 4 > ./results/out_4_fnw_nfw.txt
./freemem -n -n 4 > ./results/out_4_fnw_nfnw.txt
