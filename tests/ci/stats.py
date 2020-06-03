#!/bin/python
import sys
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
import pandas as pd

'''
This script takes as input the log from romp and generates 
access history contention statistics.
'''
contended_object_list = []
totalAccess = 0;
totalWrite = 0
contendedAccess = 0;
contendedWrite = 0;
recordOverflow=0

with open(sys.argv[1]) as f:
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

print num_contention_object
sortedContentionObjs = sorted(contended_object_list, key=lambda x: x[2], reverse=True)

#topContentionObjs = sortedContentionObjs[0:len(sortedContentionObjs) - 1]
topContentionObjs = sortedContentionObjs
 

print topContentionObjs

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

print len(listUncontendRead), len(listUncontendWrite), len(listContendedRead), len(listContendedWrite)

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

summaryDf = pd.DataFrame({
                    'contend access ratio': [contendedAccessRatio],
                    'write contend ratio' : [writeContentionRatio],
                    'read contend ratio' : [readContentionRatio],
                    })

plt.figure(1)
plt.subplot(2,2,1)
plt.suptitle(sys.argv[1] + ' #overflow access:' + recordOverflow)
df[['uncontended read', 
    'uncontended write', 
    'contended read', 
    'contended write']].plot(ax=plt.gca(),kind='bar', stacked=True, color=myColors)
plt.ylabel('#Accesses')
plt.title('Total Accesses')
plt.subplot(2,2,2)
objDf[['obj uncontended read', 
    'obj uncontended write', 
    'obj contended read', 
    'obj contended write']].plot(ax=plt.gca(),kind='bar', stacked=True, color=myColors)
plt.ylabel('#Accesses')
plt.title('Contended Access History Objects')

plt.subplot(2,2,3)
summaryDf.plot(ax=plt.gca(), kind='bar', stacked=False)

#plt.show()

pdf = PdfPages('test.pdf')
plt.savefig(pdf, format='pdf')
pdf.close();
