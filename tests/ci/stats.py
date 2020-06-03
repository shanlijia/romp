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
    print(lines)
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
  contendedAccessRatio = (contendedRead + contendedWrite)/totalAccess
  writeContentionRatio = contendedWrite / (contendedWrite + contendedRead)
  readContentionRatio = contendedRead / (contendedRead + contendedWrite)
  myColors=['green', 'blue', 'orange', 'red']
  df = pd.DataFrame({
                   'uncontended read': [uncontendRead], 
                   'uncontended write': [uncontendWrite],
                   'contended read': [contendedRead],
                   'contended write': [contendedWrite],
                   })
  objDf = pd.DataFrame({
                   'obj uncontended read': listUncontendRead, 
                   'obj uncontended write': listUncontendWrite,
                   'obj contended read': listContendedRead,
                   'obj contended write': listContendedWrite,
                   })

  plt.figure(1)
  plt.suptitle(os.path.basename(logName) + ' #overflow access:' + recordOverflow, fontsize=8)
  plt.subplot(1,2,1)
  df[['uncontended read', 
    'uncontended write', 
    'contended read', 
    'contended write']].plot(ax=plt.gca(),kind='bar', stacked=True, color=myColors)
  plt.gca().get_legend().remove();
  plt.xticks([])
  plt.ylabel('#Accesses')
  title = '(cr+cw)/total:' + str(contendedAccessRatio) + '\ncr/(cr+cw):' +\
             str(readContentionRatio) + '\ncw/(cr+cw):' + str(writeContentionRatio);
  plt.title(title, fontsize=6)

  plt.subplot(1,2,2)
  objDf[['obj uncontended read', 
    'obj uncontended write', 
    'obj contended read', 
    'obj contended write']].plot(ax=plt.gca(),kind='bar', stacked=True, color=myColors)
  plt.gca().get_legend().remove();
  plt.ylabel('#Accesses')
  plt.title('Contended Access History Objects', fontsize=10)
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
      process = subprocess.run([app, '5000'], stdout=f, stderr=f)
    print('generated output log:', outputLog)
  for app in apps:
    outputLog = getLogName(app)
    processLog(outputLog, pdf)
  pdf.close(); 

if __name__ == "__main__":
    main()
