#!/usr/bin/env bash
CSV_HEADER="benchmark,#check-access,#modification,#exceeds-threshold, ah-overflow-threshold, #dup-mem-access, #contObj, avg-cont-ratio, avg-mod-ratio"
#APPS=`pwd`/results/exec/*.inst
APPS=./test01.inst
TIMEOUTCMD=${TIMEOUTCMD:-"timeout"}
TIMEOUTMIN="10"
STATS_FILE=stats.csv

echo "$CSV_HEADER" >> "$STATS_FILE"

for app in $APPS
do
  echo $app	
  $TIMEOUTCMD $TIMEOUTMIN"m" "$app" 5000 &> ${app}_log
  python stats.py ${app}_log &> ${app}_log_2
  echo $avgModRatio
  echo "$app, $numCheckAccess, $numAHMod, $numAHOverflow, $threshold, $dupMemAccess, $numContentionObj, $avgContRatio, $avgModRatio" >> "$STATS_FILE"

done

