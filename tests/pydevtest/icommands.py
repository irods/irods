from __future__ import with_statement
import os
import shutil
import subprocess
import datetime
import time
import psutil
import sys

'''Originally written by Antoine de Torcy'''

RODSLOGDIR = "/var/lib/irods/iRODS/server/log"

class RodsEnv(object):
    '''Contains Env/Auth variables.

    Used as argument for RodsSession.createEnvFiles().
    '''

    def __init__(self, host, port, defRes, homeColl, cwd, username, zone, auth, csnegotiation, cspolicy, enckeysize, encsaltsize, encnumhashrounds, encalgorithm):
        self.host = host
        self.port = port
        self.defRes = defRes
        self.homeColl = homeColl
        self.cwd = cwd
        self.username = username
        self.zone = zone
        self.auth = auth
        self.csnegotiation = csnegotiation
        self.cspolicy = cspolicy
        self.enckeysize = enckeysize
        self.encsaltsize = encsaltsize
        self.encnumhashrounds = encnumhashrounds
        self.encalgorithm = encalgorithm

class RodsSession(object):
    '''A set of methods to start, close and manage multiple
    iRODS client sessions at the same time, using icommands.
    '''

    def __init__(self, topDir, icommandsDir, sessionId = 'default_session'):
        self.topDir = topDir  # main directory to store session and log dirs
        self.icommandsDir = icommandsDir  # where the icommand binaries are
        self.sessionId = sessionId
        self.sessionDir = "%s/%s" % (self.topDir, self.sessionId)

    def __terminate(self, process):
        '''
        Kills a process.  Made available here for backwards compatibility
        with Python2.5 which does not include Popen.terminate().
        '''

        def terminate_win(process):
            import win32process
            return win32process.TerminateProcess(process._handle, -1)

        def terminate_nix(process):
            import os
            import signal
            return os.kill(process.pid, signal.SIGTERM)

        terminate_default = terminate_nix

        handlers = {
            "win32": terminate_win, 
            "linux2": terminate_nix
        }

        return handlers.get(sys.platform, terminate_default)(process)

    def createEnvFiles(self, myEnv):
        '''Creates session files in temporary directory.

        Argument myEnv must be instance of RodsEnv defined above.
        This method is to be called prior to calling self.runCmd('iinit').
        '''
        # will have to add some testing for existing files
        # and acceptable argument values
        os.makedirs(self.sessionDir)

        # create .irodsEnv file
        envFileAbsPath = "%s/%s" % (self.sessionDir, ".irodsEnv")
        ENVFILE = open(envFileAbsPath, "w")
        ENVFILE.write("irodsHost '%s'\n" % myEnv.host)
        ENVFILE.write("irodsPort '%s'\n" % myEnv.port)
        ENVFILE.write("irodsDefResource '%s'\n" % myEnv.defRes)
        ENVFILE.write("irodsHome '%s'\n" % myEnv.homeColl)
        ENVFILE.write("irodsCwd '%s'\n" % myEnv.cwd)
        ENVFILE.write("irodsUserName '%s'\n" % myEnv.username)
        ENVFILE.write("irodsZone '%s'\n" % myEnv.zone)
        ENVFILE.write("irodsClientServerNegotiation '%s'\n" % myEnv.csnegotiation)
        ENVFILE.write("irodsClientServerPolicy '%s'\n" % myEnv.cspolicy)
        ENVFILE.write("irodsEncryptionKeySize '%s'\n" % myEnv.enckeysize)
        ENVFILE.write("irodsEncryptionSaltSize '%s'\n" % myEnv.encsaltsize)
        ENVFILE.write("irodsEncryptionNumHashRounds '%s'\n" % myEnv.encnumhashrounds)
        ENVFILE.write("irodsEncryptionAlgorithm '%s'\n" % myEnv.encalgorithm)
        ENVFILE.close()

    def deleteEnvFiles(self):
        '''Deletes temporary sessionDir recursively.

        To be called after self.runCmd('iexit').
        '''
        shutil.rmtree(self.sessionDir)

    def sessionFileExists(self):
        '''Checks for the presence of .irodsEnv in temporary sessionDir.
        '''
        try:
            if '.irodsEnv' in os.listdir(self.sessionDir):
                return True
        except:
            return False
        else:
            return False

    def getZoneName(self):
        '''Returns current zone name from .irodsEnv or an empty string
        if the file does not exist.
        '''
        returnstring = ""
        if not self.sessionFileExists():
            return returnstring
        envfilename = "%s/.irodsEnv" % (self.sessionDir)
        envfile = open(envfilename)
        for line in envfile:
            if 'irodsZone' in line:
                returnstring = line.split()[1]
                returnstring = returnstring.replace('"',"")   # remove double quotes
                returnstring = returnstring.replace("'","")   # remove single quotes
        envfile.close()
        return returnstring

    def getUserName(self):
        '''Returns current irodsUserName from .irodsEnv or an empty string
        if the file does not exist.
        '''
        returnstring = ""
        if not self.sessionFileExists():
            return returnstring
        envfilename = "%s/.irodsEnv" % (self.sessionDir)
        envfile = open(envfilename)
        for line in envfile:
            if 'irodsUserName' in line:
                returnstring = line.split()[1]
                returnstring = returnstring.replace('"',"")   # remove double quotes
                returnstring = returnstring.replace("'","")   # remove single quotes
        envfile.close()
        return returnstring

    def getDefResource(self):
        '''Returns current default Resource Name from .irodsEnv or an empty string
        if the file does not exist.
        '''
        returnstring = ""
        if not self.sessionFileExists():
            return returnstring
        envfilename = "%s/.irodsEnv" % (self.sessionDir)
        envfile = open(envfilename)
        for line in envfile:
            if 'irodsDefResource' in line:
                returnstring = line.split()[1]
                returnstring = returnstring.replace('"',"")   # remove double quotes
                returnstring = returnstring.replace("'","")   # remove single quotes
        envfile.close()
        return returnstring

    def interruptCmd(self, icommand, argList, filename, filesize):
        '''Runs an icommand with optional argument list but
        terminates the icommand subprocess once the filename
        reaches filesize bytes or larger.

        Returns  0 if subprocess was terminated.
        Returns -1 if subprocess completed normally.
        Returns -2 if timeout is reached.

        Not currently checking against allowed icommands.
        '''

        # should probably also add a condition to restrict
        # possible values for icommandsDir
        myenv = os.environ.copy()
        myenv['irodsEnvFile'] = "%s/.irodsEnv" % (self.sessionDir)
        myenv['irodsAuthFileName'] = "%s/.irodsA" % (self.sessionDir)

        cmdStr = "%s/%s" % (self.icommandsDir, icommand)
        argList = [cmdStr] + argList

        global RODSLOGDIR
        proc = subprocess.Popen('ls -t '+RODSLOGDIR+'/rodsLog* | head -n1', stdout=subprocess.PIPE, shell=True)
        (myrodslogfile, err) = proc.communicate()
        with open(myrodslogfile.rstrip(),"a") as myrodslog:
            myrodslog.write(" --- interrupt icommand ["+' '.join(argList)+"] --- \n")
        myrodslog.close()

        p = subprocess.Popen(argList, stdout = subprocess.PIPE, \
            stderr = subprocess.PIPE, env = myenv)

        # set timeout in seconds
        timeout = 60  # 1 minute
        begin = time.time()
        # wait for filename to get big enough to terminate subprocess
        granularity = 0.01
        while ((time.time() - begin < timeout) and (not os.path.exists(filename))):
            # does not exist yet
            time.sleep(granularity)
        while ((time.time() - begin < timeout) and (os.stat(filename).st_size < filesize)):
            # not big enough yet
            time.sleep(granularity)
        # if timeout was reached, return -2
        if ((time.time() - begin) >= timeout):
            returncode = -2
        # else if subprocess did not complete by filesize threshold, we kill it
        elif p.poll() is None:
            if (sys.version_info >= (2,6)):
                # use native p.terminate() available in 2.6+
                p.terminate()
            else:
                # use private version of __terminate (uses os.kill())
                self.__terminate(p)
            # expected, so return 0
            returncode = 0
        # else the process finished before the filesize threshold was met
        else:
            # unexpected, so return -1
            returncode = -1

        return returncode

    def interruptCmdDelay(self, icommand, argList=[], delay=0):
        '''Runs an icommand with optional argument list but
        terminates the icommand subprocess after delay seconds.

        Returns 0  if subprocess was terminated.
        Returns -1 if subprocess completed normally.

        Not currently checking against allowed icommands.
        '''

        # should probably also add a condition to restrict
        # possible values for icommandsDir
        myenv = os.environ.copy()
        myenv['irodsEnvFile'] = "%s/.irodsEnv" % (self.sessionDir)
        myenv['irodsAuthFileName'] = "%s/.irodsA" % (self.sessionDir)

        cmdStr = "%s/%s" % (self.icommandsDir, icommand)
        argList = [cmdStr] + argList

        global RODSLOGDIR
        proc = subprocess.Popen('ls -t '+RODSLOGDIR+'/rodsLog* | head -n1', stdout=subprocess.PIPE, shell=True)
        (myrodslogfile, err) = proc.communicate()
        with open(myrodslogfile.rstrip(),"a") as myrodslog:
            myrodslog.write(" --- interrupt icommand delay("+str(delay)+") ["+' '.join(argList)+"] --- \n")
        myrodslog.close()

        p = subprocess.Popen(argList, stdout = subprocess.PIPE, \
            stderr = subprocess.PIPE, env = myenv)

        # wait for subprocess to complete
        granularity = 0.01
        t = 0.0
        while t < delay and p.poll() is None:
            time.sleep(granularity)
            t += granularity

        # if subprocess did not complete by delay, we kill it
        if p.poll() is None:
            p.terminate()
             # expected, so return 0
            returncode = 0
        # else the process finished before the delay expired
        else:
            # unexpected, so return -1
            returncode = -1

        return returncode

    def runCmd(self, icommand, argList=[], waitforresult=True, delay=0):
        '''Runs an icommand with optional argument list and
        returns tuple (stdout, stderr) from subprocess execution.

        Set of valid commands can be extended.
        '''
        valid_cmds = [  'iinit',
                        'ienv',
                        'ihelp',
                        'ils',
                        'icd',
                        'imkdir',
                        'ichmod',
                        'imeta',
                        'iget',
                        'iput',
                        'imv',
                        'icp',
                        'irepl',
                        'iquest',
                        'irm',
                        'irmtrash',
                        'iexit',
                        # added for test_originaldevtest.py
                        'ilsresc',
                        'imiscsvrinfo',
                        'iuserinfo',
                        'ipwd',
                        'ierror',
                        'iexecmd',
                        'ips',
                        'iqstat',
                        'ichksum',
                        'itrim',
                        'iphymv',
                        'ibun',
                        'iphybun',
                        'ireg',
                        'imcoll',
                        'irsync',
			'ixmsg',
                        # added for test_allrules.py
                        'irule',
                        'iqdel',
                        # added for ticket_suite.py
                        'iticket',
                     ]

        if icommand not in valid_cmds:
            # second value represents STDERR
            return ("","Invalid Command - '"+icommand+"' not allowed")

        # should probably also add a condition to restrict
        # possible values for icommandsDir
        myenv = os.environ.copy()
        myenv['irodsEnvFile'] = "%s/.irodsEnv" % (self.sessionDir)
        myenv['irodsAuthFileName'] = "%s/.irodsA" % (self.sessionDir)

        cmdStr = "%s/%s" % (self.icommandsDir, icommand)
        argList = [cmdStr] + argList

        global RODSLOGDIR
        proc = subprocess.Popen('ls -t '+RODSLOGDIR+'/rodsLog* | head -n1', stdout=subprocess.PIPE, shell=True)
        (myrodslogfile, err) = proc.communicate()
        with open(myrodslogfile.rstrip(),"a") as myrodslog:
            myrodslog.write(" --- icommand ["+' '.join(argList)+"] --- \n")
        myrodslog.close()

        if delay > 0:
            print "  runCmd: sleeping ["+str(delay)+"] seconds"
            time.sleep(delay)

        if waitforresult:
            return subprocess.Popen(argList, stdout = subprocess.PIPE, \
                stderr = subprocess.PIPE, env = myenv).communicate()
        else:
            return subprocess.Popen(argList, stdout = subprocess.PIPE, \
                stderr = subprocess.PIPE, env = myenv)


    def runAdminCmd(self, icommand, argList=[]):
        '''Runs the iadmin icommand with optional argument list and
        returns tuple (stdout, stderr) from subprocess execution.
        '''

        if icommand != 'iadmin':
            # second value represents STDERR
            return ("","Invalid Admin Command - '"+icommand+"' not allowed")

        # should probably also add a condition to restrict
        # possible values for icommandsDir
        myenv = os.environ.copy()
        myenv['irodsEnvFile'] = "%s/.irodsEnv" % (self.sessionDir)
        myenv['irodsAuthFileName'] = "%s/.irodsA" % (self.sessionDir)

        cmdStr = "%s/%s" % (self.icommandsDir, icommand)
        argList = [cmdStr] + argList

        global RODSLOGDIR
        proc = subprocess.Popen('ls -t '+RODSLOGDIR+'/rodsLog* | head -n1', stdout=subprocess.PIPE, shell=True)
        (myrodslogfile, err) = proc.communicate()
        with open(myrodslogfile.rstrip(),"a") as myrodslog:
           myrodslog.write(" --- iadmin ["+' '.join(argList)+"] --- \n")
        myrodslog.close()

        return subprocess.Popen(argList, stdin = subprocess.PIPE, \
            stdout = subprocess.PIPE, stderr = subprocess.PIPE, \
            env = myenv).communicate("yes\n")


### an example usage
#def testsuite():
#    working_path = "/Users/trel/Desktop/irodstesting/sessions"
#    icommands_bin = "/Users/trel/Desktop/irodstesting/iRODS/clients/icommands/bin"
#    session_id = datetime.datetime.now().strftime("%Y%m%dT%H%M%S.%f")
#    userInfo = RodsEnv( 'trelpancake',
#                        '1247',
#                        'trelpancakeResource',
#                        '/tempZone/home/rods',
#                        '/tempZone/home/rods',
#                        'rods',
#                        'tempZone',
#                        'rods')
#    mysession = RodsSession(working_path, icommands_bin, session_id)
#    mysession.createEnvFiles(userInfo)
#
#    mysession.runCmd('iinit', [userInfo.auth])
#
#    output =  mysession.runCmd('ils')
#    print output[0]
#
#    print "\nimeta ls -d beetle.jpg:\n"
#    output = mysession.runCmd('imeta',['ls', '-d', 'beetle.jpg'])
#    print output[0]
#
#    mysession.runCmd('icd',['testcoll0'])
#
#    output =  mysession.runCmd('ils')
#    print output[0]
#
#    mysession.runCmd('icd',['..'])
#
#    print "\nimeta ls -C testcoll0:\n"
#    output = mysession.runCmd('imeta',['ls', '-C', 'testcoll0'])
#    print output[0]
#
#    mysession.runCmd('iexit', ['full'])
#    mysession.deleteEnvFiles()
