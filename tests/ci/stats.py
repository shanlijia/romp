#!/usr/bin/python
import matplotlib.pyplot as plt
import numpy as np
import os
import pandas as pd
import subprocess
import sys

from matplotlib.backends.backend_pdf import PdfPages
'''
This script takes as input the log from romp and generates 
access history contention statistics.
'''


def processLog(logName, pdfHandle):
  print('processing log:', logName)
  contended_object_list = []
  totalAccess = 0;
  totalWrite = 0
  contendedAccess = 0;
  contendedWrite = 0;
  recordOverflow=0
  with open(logName, 'r') as f:
    lines = f.readlines();
    num_contention_object = 0;
    for line in lines:
      line = line.strip('\n')
      if 'gNumCheckAccessCall#' in line:
        totalAccess = float(line.split('#')[1]);
      elif 'gNumContendedAccess#' in line:
        contendedAccess = float(line.split('#')[1])  
      elif 'gNumModAccessHistory#' in line:
        totalWrite = float(line.split('#')[1])
      elif 'gNumContendedMod#' in line:
        contendedWrite = float(line.split('#')[1]) 
      elif 'gNumAccessHistoryOverflow#' in line:
        recordOverflow = line.split('#')[1]
      elif 'ObjectContention' in line:
        num_contention_object += 1;
        line = line.strip('\n')
        split_line = line.split('#');
        address = split_line[1]
        numContendedAccess = float(split_line[3])
        numAccess = float(split_line[5])
        numContendedMod = float(split_line[7])
        numMod = float(split_line[9])
        contended_object_list.append((address,numAccess, numContendedAccess, 
          numMod, numContendedMod)) 

    sortedContentionObjs = sorted(contended_object_list, key=lambda x: x[2], reverse=True)
    if len(sortedContentionObjs) > 20:
      topContentionObjs = sortedContentionObjs[0:19];
    else:
      topContentionObjs = sortedContentionObjs;

  listUncontendRead = []
  listUncontendWrite = []
  listContendedRead = []
  listContendedWrite = []
  listAddress= []
  for obj in topContentionObjs:
    objTotalAccess = obj[1]
    objContendedAccess = obj[2]
    objTotalWrite = obj[3]
    objContendedWrite = obj[4]
    #derived obj values 
    objUncontendAccess = objTotalAccess - objContendedAccess;
    objUncontendWrite = objTotalWrite - objContendedWrite;
    objUncontendRead = objUncontendAccess - objUncontendWrite;
    objContendedRead = objContendedAccess - objContendedWrite;
    listAddress.append(obj[0])
    listUncontendRead.append(objUncontendRead);    
    listUncontendWrite.append(objUncontendWrite);
    listContendedRead.append(objContendedRead);
    listContendedWrite.append(objContendedWrite);

  #derived global values 
  uncontendAccess = totalAccess - contendedAccess;
  uncontendWrite = totalWrite - contendedWrite 
  uncontendRead = uncontendAccess - uncontendWrite
  contendedRead = contendedAccess - contendedWrite
  try:
    contendedAccessRatio = (contendedRead + contendedWrite)/totalAccess
    writeContentionRatio = contendedWrite / (contendedWrite + contendedRead)
    readContentionRatio = contendedRead / (contendedRead + contendedWrite)
  except:
    print("exception encountered")
    return
  myColors=['green', 'blue', 'orange', 'red']
  d = {'ucr':[uncontendRead], 'ucw':[uncontendWrite], 'cr':[contendedRead], 'cw':[contendedWrite]}
  df = pd.DataFrame(data=d)
  print(df.shape)
  d = {'lucr':listUncontendRead, 'lucw':listUncontendWrite, 'lcr':listContendedRead,'lcw':listContendedWrite}
  objdf = pd.DataFrame(data=d)
  print(objdf.shape)

  fig = plt.figure()
  fig.suptitle(os.path.basename(logName) + ' #overflow access:' + recordOverflow, fontsize=10)
  stats = '(cr+cw)/total:' + str(contendedAccessRatio) + '\ncr/(cr+cw):' + str(readContentionRatio) + '\ncw/(cr+cw):' + str(writeContentionRatio);
  ax1 = fig.add_subplot(121)
  ax1.set_title(stats, fontsize=6)
  ax1.set_ylabel('#Accesses')
  ax2 = fig.add_subplot(122)
  ax2.set_ylabel('#Accesses')
  ax2.set_title('contended access history objects', fontsize=8)
  df.plot.bar(stacked=True,color=myColors,ax=ax1,xticks=[])
  objdf.plot.bar(stacked=True,color=myColors,ax=ax2,legend=False)
  plt.savefig(pdfHandle, format='pdf')

def getLogName(app):
  return app + '_log'

def main():
  inputDirectoryName = sys.argv[1]
  outputFileName = sys.argv[2]
  pdf = PdfPages(outputFileName)
  apps = list(filter(lambda x:('.inst' in x and 'log' not in x),os.listdir(inputDirectoryName)))
  apps = [os.path.join(os.path.abspath(inputDirectoryName), x) for x in apps]
  for app in apps:
    print('exec:', app)
    outputLog = getLogName(app)
    with open(outputLog, 'w+') as f:
      process = subprocess.run([app, '5000','10'], stdout=f, stderr=f)
    print('generated output log:', outputLog)
  for app in apps:
    outputLog = getLogName(app)
    processLog(outputLog, pdf)
  pdf.close(); 

if __name__ == "__main__":
    main()
