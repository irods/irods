import sys
import os
import subprocess
if (sys.version_info >= (2,7)):
	import unittest
else:
	import unittest2 as unittest
import pydevtest_sessions as s




class Test_ixmsg(unittest.TestCase):
	serverConfigFile = "/etc/irods/server.config"
	xmsgHost = 'localhost'
	xmsgPort = '1279'


	def setUp(self):
		# Set up admin session
		s.admin_up()

		# add Xmsg settings to server.config
		os.system("cp %s %s_orig" % (self.serverConfigFile, self.serverConfigFile))
		os.system('echo "xmsgHost %s" >> %s' % (self.xmsgHost, self.serverConfigFile))
		os.system('echo "xmsgPort %s" >> %s' % (self.xmsgPort, self.serverConfigFile))
		
		# apparently needed by the server too...
		my_env = os.environ.copy()
		my_env['xmsgHost'] = self.xmsgHost
		my_env['xmsgPort'] = self.xmsgPort	
		
		# restart server with Xmsg
		args = ['/var/lib/irods/iRODS/irodsctl', 'restart']
		subprocess.Popen(args, env=my_env).communicate()


	def tearDown(self):
		# revert to original server.config
		os.system("mv -f %s_orig %s" % (self.serverConfigFile, self.serverConfigFile))
		
		# restart server
		my_env = os.environ.copy()
		args = ['/var/lib/irods/iRODS/irodsctl', 'restart']
		subprocess.Popen(args, env=my_env).communicate()

		# Close admin session
		s.admin_down()


	def test_send_and_receive_one_xmsg(self):
		message = 'Hello World!'
		
		# set up Xmsg in client environment
		my_env = os.environ.copy()
		my_env['xmsgHost'] = self.xmsgHost
		my_env['xmsgPort'] = self.xmsgPort

		# send msg
		args = ['/usr/bin/ixmsg', 's', '-M "{0}"'.format(message)]
		subprocess.Popen(args, env=my_env).communicate()	# couldn't get ixmsg to work non-interactively in assertiCmd()...
		
		# receive msg
		args = ['/usr/bin/ixmsg', 'r', '-n 1']
		res = subprocess.Popen(args, env=my_env, stdout=subprocess.PIPE).communicate()
		
		# assertion
		print 'looking for "{0}" in "{1}"'.format(message, res[0].rstrip())
		assert (res[0].find(message) >= 0)
		

