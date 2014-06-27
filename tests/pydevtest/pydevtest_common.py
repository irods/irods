import re
import os
import socket
import shlex
import time
import psutil
from signal import signal, SIGPIPE, SIG_DFL

# Ignore SIG_PIPE and don't throw exceptions on it...
# Added due to problems on cen5
# (http://docs.python.org/library/signal.html)
signal(SIGPIPE,SIG_DFL)


# =-=-=-=-=-=-=-
# global variable dictating that we are running in a
# toplogical testing framework.  skip certain tests
# and define various hostnames to resources
RUN_IN_TOPOLOGY=False;

# =-=-=-=-=-=-=-
# global variable dictating that we are running as a
# resource server during topological testing
RUN_AS_RESOURCE_SERVER=False;

if os.name != "nt":
    import fcntl
    import struct

def get_interface_ip(ifname):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    return socket.inet_ntoa( fcntl.ioctl( s.fileno(),
        0x8915,  # SIOCGIFADDR
        struct.pack('256s', ifname[:15]) )[20:24])

def get_lan_ip():
    ip = socket.gethostbyname(socket.gethostname())
    if ip.startswith("127.") and os.name != "nt":
        interfaces = ["eth0","eth1","eth2","wlan0","wlan1","wifi0","ath0","ath1","ppp0"]
        for ifname in interfaces:
            try:
                ip = get_interface_ip(ifname)
                break;
            except IOError:
                pass
    return ip

def get_hostname():
    return socket.gethostname()

def get_irods_top_level_dir():
    configdir = "/etc/irods/irods.config"
    topleveldir = "/var/lib/irods"
    if( not os.path.isfile( configdir )):
        topleveldir = os.path.dirname( os.path.dirname( os.path.dirname( os.path.abspath(__file__) ) ) )
    return topleveldir

def get_irods_config_dir():
    configfile = "/etc/irods/irods.config"
    configdir = os.path.dirname( configfile )
    if( not os.path.isfile( configfile )):
        configdir = get_irods_top_level_dir() + "/iRODS/config"
    return configdir

def create_directory_of_small_files(directory_name_suffix, file_count):
    if not os.path.exists(directory_name_suffix):
        os.mkdir(directory_name_suffix)
    for i in range(file_count):
        target = open (("%s/%d" % (directory_name_suffix, i)), 'w')
        target.write("iglkg3fqfhwpwpo-"+"A"*i)
        target.close()

def create_local_testfile(filename):
    filepath = os.path.abspath(filename)
    f = open(filepath,'wb')
    f.write("TESTFILE -- ["+filepath+"]")
    f.close()
    return filepath

def check_icmd_outputtype(fullcmd,outputtype):
    allowed_outputtypes = ["LIST","EMPTY","ERROR",""]
    if outputtype not in allowed_outputtypes:
        print "  full command: ["+fullcmd+"]"
        print "  allowed outputtypes: "+str(allowed_outputtypes)
        print "  unknown outputtype requested: ["+outputtype+"]"
        assert False, "hard fail, bad icommand output format requested"

def getiCmdOutput(mysession,fullcmd):
    parameters = shlex.split(fullcmd) # preserves quoted substrings
    print "running icommand: "+mysession.getUserName()+"["+fullcmd+"]"
    if parameters[0] == "iadmin":
        output = mysession.runAdminCmd(parameters[0],parameters[1:])
    else:
        output = mysession.runCmd(parameters[0],parameters[1:])
    # return output array
    #   [0] is stdout
    #   [1] is stderr
    return output

def getiCmdBoolean(mysession,fullcmd,outputtype="",expectedresults=""):
    result = False # should start as failing, then get set to pass
    parameters = shlex.split(fullcmd) # preserves quoted substrings
    # expectedresults needs to be a list
    if isinstance(expectedresults, str): # converts a string to a list
        expectedresults = [expectedresults]
    # get output from icommand
    output = getiCmdOutput(mysession,fullcmd)
    # check result listing for expected results
    if (outputtype == "LIST" or outputtype == "ERROR"):
        print "  Expecting "+outputtype+": "+str(expectedresults)
        print "  stdout:"
        print "    | "+"\n    | ".join(output[0].splitlines())
        print "  stderr: ["+output[1].rstrip('\n')+"]"
        # generate lines based on outputtype
        if (outputtype == "LIST"):
            lines = output[0].splitlines()
        else:
            lines = output[1].splitlines()
        # look for expected results in the output lines
        for line in lines:
            foundcount = 0
            for er in expectedresults:
                print "  searching ["+line.rstrip('\n')+"] for ["+er+"] ...",
                if not re.search(re.escape(er),line.rstrip('\n')) == None:
                    foundcount += 1
                    print "found ("+str(foundcount)+" of "+str(len(expectedresults))+")"
                else:
                    print "NOTFOUND"
            if foundcount == len(expectedresults):
                print "    --> stopping search - expected result(s) found"
                result = True
                break
            else:
                print "    --> did not find expected result(s)"
    # check that icommand returned no result
    elif (outputtype == "EMPTY" or outputtype == ""):
        print "  Expecting EMPTY output"
        print "  stdout: ["+",".join(output[0].splitlines())+"]"
        print "  stderr: ["+output[1].strip()+"]"
        if output[0] == "":
            result = True
    # bad test formatting
    else:
        print "  WEIRD - SHOULD ALREADY HAVE BEEN CAUGHT ABOVE"
        print "  unknown outputtype requested: ["+outputtype+"]"
        assert False, "WEIRD - DUPLICATE BRANCH - hard fail, bad icommand format"
    # return error if stderr is populated unexpectedly
    if (outputtype != "ERROR" and output[1] != ""):
        return False
    # return value
    return result

def assertiCmd(mysession,fullcmd,outputtype="",expectedresults=""):
    ''' Runs an icommand, detects output type, and searches for
    values in expected results list.

    Asserts that this result is correct.

    Returns elapsed runtime.
    '''
    begin = time.time()
    print "\n"
    print "ASSERTING PASS"
    check_icmd_outputtype(fullcmd,outputtype)
    assert getiCmdBoolean(mysession,fullcmd,outputtype,expectedresults)
    elapsed = time.time() - begin
    return elapsed

def assertiCmdFail(mysession,fullcmd,outputtype="",expectedresults=""):
    ''' Runs an icommand, detects output type, and searches for
    values in expected results list.

    Asserts that this result is NOT correct.

    Returns elapsed runtime.
    '''
    begin = time.time()
    print "\n"
    print "ASSERTING FAIL"
    check_icmd_outputtype(fullcmd,outputtype)
    assert not getiCmdBoolean(mysession,fullcmd,outputtype,expectedresults)
    elapsed = time.time() - begin
    return elapsed

def interruptiCmd(mysession,fullcmd,filename,filesize):
    ''' Runs an icommand, but does not let it complete.

    This function terminates the icommand once filename reaches (>=)
    filesize in bytes.

    Asserts that the icommand was successfully terminated early.

    Returns 0 or -1 or -2.
    '''
    parameters = shlex.split(fullcmd) # preserves quoted substrings
    print "\n"
    print "INTERRUPTING iCMD"
    print "running icommand: "+mysession.getUserName()+"["+fullcmd+"]"
    print "  filename set to: ["+filename+"]"
    print "  filesize set to: ["+str(filesize)+"] bytes"
    resultcode = mysession.interruptCmd(parameters[0],parameters[1:],filename,filesize)
    if resultcode == 0:
        print "  resultcode: [0], interrupted successfully"
    elif resultcode == -1:
        print "  resultcode: [-1], icommand completed"
    else:
        print "  resultcode: [-2], icommand timeout"
    assert 0 == resultcode, "0 == resultcode"
    return resultcode

def interruptiCmdDelay(mysession,fullcmd,delay):
    ''' Runs an icommand, but does not let it complete.

    This function terminates the icommand after delay seconds.

    Asserts that the icommand was successfully terminated early.

    Returns 0 or -1.
    '''
    parameters = shlex.split(fullcmd) # preserves quoted substrings
    print "\n"
    print "INTERRUPTING iCMD"
    print "running icommand: "+mysession.getUserName()+"["+fullcmd+"]"
    print "  timeout set to: ["+str(delay)+" seconds]"
    resultcode = mysession.interruptCmdDelay(parameters[0],parameters[1:],delay)
    if resultcode == 0:
        print "  resultcode: [0], interrupted successfully"
    else:
        print "  resultcode: [-1], icommand completed"
    assert 0 == resultcode, "0 == resultcode"
    return resultcode

