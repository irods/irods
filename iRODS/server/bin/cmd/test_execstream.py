#!/usr/bin/env python

# test script that will print an arbitrary buffer of randomish data based on the length given in the argument.
# this script is meant to test streaming from remote execution, and is used in jargon unit testing of remote execution
# author: mike conway - DICE


import sys
import getopt


def main():
    args = sys.argv[1:]
    
    argsLen = len(args)
    
    if (argsLen == 0):
       bytesToPrint = 1024
    else:
       bytesToPrint = int(args[0])
       
    printedSoFar = 0

    #print "bytes to print: %d" % (bytesToPrint)
    
    bufToPrint = "xxxxxxxxxjjjjjfffffffffffffffffff88888888888888888888888888888888888888888888888888888888eeeeeeeeeeeeeeeennnnnnnn21183usehfas8eura89utjio;sadjfa;sofjas;difjas;eifjas;dfkjas;dfj"
    
    lenOfBuf = len(bufToPrint)
    
    #print "lenOfBuf: %d" % (lenOfBuf)
    
    printThisTime = 0
    while (printedSoFar < bytesToPrint) :
    	printThisTime = bytesToPrint - printedSoFar
    	#print "i am going to print %d this time\n" % (printThisTime)
    	if (printThisTime > lenOfBuf) :
    	    #print "i am going to print the full length of the buff"
    	    sys.stdout.write(bufToPrint)
    	    printedSoFar += lenOfBuf
    	else :
    	    #print "i am going to print a subset of the buffer"
    	    sys.stdout.write(bufToPrint[0:printThisTime - 1])
    	    printedSoFar += printThisTime
    
        #print "i have printed %d bytes so far\n" % (printedSoFar)
    
       
   
if __name__ == "__main__":
    sys.exit(main())

