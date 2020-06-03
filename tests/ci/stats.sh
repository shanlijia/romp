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
  numCheckAccess=$(cat ${app}_log | egrep -o "gNumCheckAccessCall: [0-9]+" | egrep -o "[0-9]+")      
  numContendedAccess=$(cat ${app}_log | egrep -o "gNumContendedAccess: [0-9]+" | egrep -o "[0-9]+")      
  numAHMod=$(cat ${app}_log | egrep -o "gNumModAccessHistory: [0-9]+" | egrep -o "[0-9]+")      
  numContendedAHMod=$(cat ${app}_log | egrep -o "gNumContendedMod: [0-9]+" | egrep -o "[0-9]+")      
  numAHOverflow=$(cat ${app}_log | egrep -o "gNumAccessHistoryOverflow: [0-9]+" | egrep -o "[0-9]+")      
  threshold=$(cat ${app}_log | egrep -o "REC_NUM_THRESHOLD: [0-9]+" | egrep -o "[0-9]+")      
 # dupMemAccess=$(cat ${app}_log | egrep -o "num dup memory access: [0-9]+" | egrep -o "[0-9]+")      
  numContentionObj=$(cat ${app}_log_2 | egrep -o "num contention objects: [0-9]+" | egrep -o "[0-9]+")
  avgContRatio=$(cat ${app}_log_2 | egrep -o "average contention ratio: [0-9]+\.[0-9]+" | egrep -o "[0-9]+\.[0-9]+")
  avgModRatio=$(cat ${app}_log_2 | egrep -o "average mod ratio: [0-9]+\.[0-9]+" | egrep -o "[0-9]+\.[0-9]+")
  echo $avgModRatio
  echo "$app, $numCheckAccess, $numAHMod, $numAHOverflow, $threshold, $dupMemAccess, $numContentionObj, $avgContRatio, $avgModRatio" >> "$STATS_FILE"

done

