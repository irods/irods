#! /usr/bin/env python
#
# Copyright (c) 2011  Renaissance Computing Institute. All rights reserved.
#
# This software is open source. See the bottom of this file for the license.
# 
# Renaissance Computing Institute, 
# (A Joint Institute between the University of North Carolina at Chapel Hill,
# North Carolina State University, and Duke University)
# http://www.renci.org
# 
# For questions, comments please contact software@renci.org
#
import getopt
import os
import sys
import subprocess
import shlex
import time

def testStatusOperation(resource, user, password):
   args = 'curl-wos --operation=status ' + '--user=' + user  + \
   ' --password=' + password + ' --resource=' + resource +\
   ':8088/mgmt/statistics'
   args = shlex.split(args)
  # print args
   process = subprocess.Popen(args, stdout=subprocess.PIPE)
   process.wait()
   print 'result of testStatusOperation: '  + str(process.returncode)
   optTime = process.stdout.readline().rstrip()
   print optTime
   statusOut = process.stdout.readline().rstrip()
   print 'capacity, used ' + statusOut

def testDeleteOperation(resource, srcFile):
   args = 'curl-wos --operation=delete ' + '--resource=' + resource  + \
   ' --file=' + srcFile
   args = shlex.split(args)
  # print args
   process = subprocess.Popen(args, stdout=subprocess.PIPE)
   process.wait()
   print 'result of testDeleteOperation:'  + str(process.returncode)
   optTime = process.stdout.readline().rstrip()
   print optTime
   newOID = process.stdout.readline().rstrip()
   print 'stdout ' + newOID
   return newOID

def testPutOperation(resource, policy, srcFile):
   args = 'curl-wos --operation=put ' + '--resource=' + resource  + \
   ' --policy=' + policy  + ' --file=' + srcFile
   args = shlex.split(args)
  # print args
   process = subprocess.Popen(args, stdout=subprocess.PIPE)
   process.wait()
   print 'result of testPutOperation: '  + str(process.returncode)
   optTime = process.stdout.readline().rstrip()
   print optTime
   newOID = process.stdout.readline().rstrip()
   print 'stdout ' + newOID
   return newOID

def testGetOperation(resource, policy, newOID, destination):
   args = 'curl-wos --operation=get ' + '--resource=' + resource  + \
   ' --policy=' + policy  + ' --file=' + newOID + ' --destination=' + destination
   args = shlex.split(args)
  # print args
   process = subprocess.Popen(args, stdout=subprocess.PIPE)
   process.wait()
   print 'result of testGetOperation: '  + str(process.returncode)
   optTime = process.stdout.readline().rstrip()
   print optTime
   newOID = process.stdout.readline().rstrip()
   print 'stdout ' + newOID
   return newOID


def main():

   # Init the variables for the user args so they retain scope.
   resource = ""
   policy = ""
   srcFile = ""
   operation = ""
   destination = ""
   user = ""
   password = ""

   argFlags ="r:p:f:o:d:u:a:"
   argNames = ["resource=", "policy=", "file=", "operation="]
   argNames += ["destination=", "user=", "password="]
   try:
       opts, args = getopt.gnu_getopt(sys.argv[1:], argFlags, argNames)
   except getopt.GetoptError, err:
       print str(err)
       sys.exit()

   # Process copious amount of arguments into a class.
   for o, a in opts:
      if o in ("-r", "--resource"):
         resource = a
      if o in ( "--policy"):
         policy = a
      if o in ( "--file"):
         srcFile = a
      if o in ( "--operation"):
         operation = a
      if o in ( "--destination"):
         destination = a
      if o in ( "--user"):
         user = a
      if o in ( "--password"):
         password = a

   testStatusOperation(resource, user, password)		 
   newOID = testPutOperation(resource, policy, srcFile)		 
   print 'newOID is ' + newOID
   time.sleep(5)
   testStatusOperation(resource, user, password)		 
   testGetOperation(resource, policy, newOID, destination)		 
   testDeleteOperation(resource, newOID)		 
   time.sleep(5)
   testStatusOperation(resource, user, password)		 
   
if __name__=='__main__':
   main()
#****************************************************************************
#
# RENCI Open Source Software License
# The University of North Carolina at Chapel Hill
# 
# The University of North Carolina at Chapel Hill (the "Licensor") through 
# its Renaissance Computing Institute (RENCI) is making an original work of 
# authorship (the "Software") available through RENCI upon the terms set 
# forth in this Open Source Software License (this "License").  This License 
# applies to any Software that has placed the following notice immediately 
# following the copyright notice for the Software:  Licensed under the RENCI 
# Open Source Software License v. 1.0.
# 
# Licensor grants You, free of charge, a world-wide, royalty-free, 
# non-exclusive, perpetual, sublicenseable license to do the following to 
# deal in the Software without restriction, including without limitation the 
# rights to use, copy, modify, merge, publish, distribute, sublicense, 
# and/or sell copies of the Software, and to permit persons to whom the 
# Software is furnished to do so, subject to the following conditions:
# 
# . Redistributions of source code must retain the above copyright notice, 
# this list of conditions and the following disclaimers.
# 
# . Redistributions in binary form must reproduce the above copyright 
# notice, this list of conditions and the following disclaimers in the 
# documentation and/or other materials provided with the distribution.
# 
# . Neither You nor any sublicensor of the Software may use the names of 
# Licensor (or any derivative thereof), of RENCI, or of contributors to the 
# Software without explicit prior written permission.  Nothing in this 
# License shall be deemed to grant any rights to trademarks, copyrights, 
# patents, trade secrets or any other intellectual property of Licensor 
# except as expressly stated herein.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
# THE CONTIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR 
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
# OTHER DEALINGS IN THE SOFTWARE.
# 
# You may use the Software in all ways not otherwise restricted or 
# conditioned by this License or by law, and Licensor promises not to 
# interfere with or be responsible for such uses by You.  This Software may 
# be subject to U.S. law dealing with export controls.  If you are in the 
# U.S., please do not mirror this Software unless you fully understand the 
# U.S. export regulations.  Licensees in other countries may face similar 
# restrictions.  In all cases, it is licensee's responsibility to comply 
# with any export regulations applicable in licensee's jurisdiction.
# 
# ****************************************************************************/
