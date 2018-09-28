from __future__ import print_function
import os
import re
import sys

if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest

from contextlib import contextmanager
from tempfile import NamedTemporaryFile
from textwrap import dedent as D_
from .. import lib
from ..configuration import IrodsConfig
from ..controller import IrodsController
from . import resource_suite
from ..core_file import temporary_core_file
from ..paths import (config_directory as irods_config_dir,
                     server_config_path)

#             Constants

GENQUERY_MODULE_BASENAME = 'genquery.py'

Allow_Intensive_Memory_Use = False

#  -------------
#  This function will be necessary until PREP tests are separate from the irods core repository:

def genquery_module_available():

    global Allow_Intensive_Memory_Use
    class EmptyModule(Exception): pass

    IRODS_CONFIG_DIR =  irods_config_dir()

    genquery_module_path = os.path.join( IRODS_CONFIG_DIR,  GENQUERY_MODULE_BASENAME)
    file_description = " module file '{}'".format (genquery_module_path)

    useable = False

    if not os.path.isfile( genquery_module_path ):
        print ("Not finding " + file_description ,file=sys.stderr)
    else:
        try:
            with open(genquery_module_path,'rt') as f:
                if not len(f.readline()) > 0:
                    raise EmptyModule
        except IOError:
            print ("No read permissions on " + file_description ,file=sys.stderr)
        except EmptyModule:
            print ("Null content in " + file_description ,file=sys.stderr)
        except Exception:
            print ("Unknown Error in accessing " + file_description ,file=sys.stderr)
        else:
            useable = True
            idx = -1
            #--> try import genquery module so that we can test optimally according to configuration
            try:
                sys.path.insert(0,IRODS_CONFIG_DIR)
                import genquery
                if getattr(genquery,'AUTO_FREE_QUERIES',None) is True:
                    Allow_Intensive_Memory_Use = True
                idx =  sys.path.index(IRODS_CONFIG_DIR)
            except ImportError: # not fatal, past versions were only importable via PREP
                pass
            except ValueError:
                idx = -1
            finally: #-- clean up module load path
                if idx >= 0:
                    cfg_dir = sys.path.pop(idx)
                    if cfg_dir != IRODS_CONFIG_DIR:
                        raise RuntimeError("Python module load path couldn't be restored")

    if not useable:
        print (" --- Not running genquery iterator tests --- " ,file=sys.stderr)

    return useable


@contextmanager
def generateRuleFile(names_list=None, **kw):

    if names_list is None:
        names_list=kw.pop('names_list', [])

    f = NamedTemporaryFile(mode='wt', dir='.', suffix='.r', delete=False, **kw)

    try:
        if isinstance(names_list,list):
            names_list.append(f.name)
        yield f

    finally:
        f.close()


class Test_Genquery_Iterator(resource_suite.ResourceBase, unittest.TestCase):

    plugin_name = IrodsConfig().default_rule_engine_plugin

    full_test = genquery_module_available()

    def setUp(self):

        super(Test_Genquery_Iterator, self).setUp()

        self.to_unlink = []

        # ------- set up variables specific to this group of tests

        self.dir_for_coll = 'testColn'

        self.bundled_coll = self.dir_for_coll + '.tar'

        self.assertFalse(os.path.exists(self.dir_for_coll))
        self.assertFalse(os.path.exists(self.bundled_coll))

        self.test_dir_path = os.path.abspath(self.dir_for_coll)
        self.test_tar_path = os.path.abspath(self.bundled_coll)

        self.server_config_path = server_config_path()

        self.test_admin_coll_path = ("/" + self.admin.zone_name +
                                     "/home/" + self.admin.username +
                                     "/" + os.path.split(self.test_dir_path)[-1] )


    # remove the next line when msiGetMoreRows always returns an accurate value for continueInx
    @unittest.skipIf(Allow_Intensive_Memory_Use, 'Replace nested multiples-of-256 pending rsGenQuery continueInx fix')
    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for python REP')
    def test_multiples_256(self):

        if not self.full_test : return
        lib.create_directory_of_small_files(self.dir_for_coll,512)
        lib.execute_command ('tar -cf {} {}'.format(self.bundled_coll, self.dir_for_coll))
        self.admin.assert_icommand('icd')
        self.admin.assert_icommand('iput -f {}'.format(self.bundled_coll))
        self.admin.assert_icommand('ibun -x {} .'.format(self.bundled_coll))

        IrodsController().stop()

        with temporary_core_file() as core:

            core.prepend_to_imports( 'from genquery import *')

            core.add_rule(D_('''\
                def my_rule_256(rule_args, callback, rei):
                    n=0
                    coll = rule_args[0]
                    for z1 in paged_iterator("DATA_NAME", "COLL_NAME = '{}'".format(coll), # 512 objects
                                           AS_LIST, callback):
                        for z2 in z1: n += 1
                    rule_args[0] = str(n)
                '''))

            IrodsController().start()

            rule_file = ""
            with generateRuleFile( names_list = self.to_unlink, **{'prefix':"test_mult256_"} ) as f:
                rule_file = f.name
                print(D_('''\
                def main(rule_args,callback,rei):
                    TestCollection = global_vars['*testcollection'][1:-1]
                    retval = callback.my_rule_256(TestCollection);
                    result = retval['arguments'][0]
                    callback.writeLine("stdout", "multiples of 256 test: {}".format(result))
                input *testcollection=$""
                output ruleExecOut
                '''), file=f, end='')

            run_irule = ("irule -F {} \*testcollection=\"'{}'\""
                                              ).format(rule_file, self.test_admin_coll_path,)

            self.admin.assert_icommand(run_irule, 'STDOUT_SINGLELINE', r'\s{}$'.format(512), use_regex=True)

            IrodsController().stop()

        IrodsController().start()


    #=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#

    # This test passed on the command-line and behaves well, but temporarily removing from CI . -DWM 2018/11

    # Remove the next line when msiGetMoreRows always returns an accurate value for continueInx
    @unittest.skipUnless(Allow_Intensive_Memory_Use, 'Skip nested multiples-of-256 pending rsGenQuery continueInx fix')
    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for python REP')
    def test_multiples_256_nested(self):

        if not self.full_test : return

        lib.create_directory_of_small_files(self.dir_for_coll,912)
        lib.execute_command('''sh -c "cd '{}' && rm ? ?? 1[0123]? 14[0123]"'''.format(self.dir_for_coll))
        lib.execute_command ('tar -cf {} {}'.format(self.bundled_coll, self.dir_for_coll))
        self.admin.assert_icommand('icd')
        self.admin.assert_icommand('iput -f {}'.format(self.bundled_coll))
        self.admin.assert_icommand('ibun -x {} .'.format(self.bundled_coll))

        IrodsController().stop()

        with temporary_core_file() as core:

            core.prepend_to_imports( 'from genquery import *') # import all names to core that are exported by genquery.py

            # with collection containing 144..911 inclusive
            core.add_rule(D_('''\
                def my_rule_256_nested(rule_args, callback, rei):
                    n=0
                    coll = rule_args[0]
                    for x1 in row_iterator("DATA_NAME",
                                           "COLL_NAME = '{}' and DATA_NAME < '4'".format(coll), # 256 objects
                                           AS_LIST, callback):
                        for y1 in row_iterator("DATA_NAME",
                                           "COLL_NAME = '{}' and DATA_NAME like '91_'".format(coll), # 2 objects
                                           AS_LIST, callback):
                            for z1 in paged_iterator("DATA_NAME",
                                                   "COLL_NAME = '{}' and DATA_NAME >= '4'".format(coll), # 512 objects
                                                   AS_LIST, callback):
                                for z2 in z1: n += 1
                    rule_args[0] = str(n)
                '''))

            IrodsController().start()

            rule_file = ""
            with generateRuleFile( names_list = self.to_unlink, **{'prefix':"test_mult256_n_"} ) as f:
                rule_file = f.name
                print(D_('''\
                def main(rule_args,callback,rei):
                    TestCollection = global_vars['*testcollection'][1:-1]
                    retval = callback.my_rule_256_nested(TestCollection);
                    result = retval['arguments'][0]
                    # import os
                    # with open("/run/shm/thing."+str(os.getpid()),"wt") as f:
                    #   f.write('result = {!r}'.format(result)+chr(10))
                    callback.writeLine("stdout", "multiples of 256 test: {}".format(result))
                input *testcollection=$""
                output ruleExecOut
                '''), file=f, end='')

            run_irule = ("irule -F {} \*testcollection=\"'{}'\"" # will loop 256 * 2 * 512 iters
                                              ).format(rule_file, self.test_admin_coll_path,)

            self.admin.assert_icommand(run_irule, 'STDOUT_SINGLELINE', r'\s{}$'.format(512*2*256), use_regex=True)

            IrodsController().stop()

        IrodsController().start()

    #=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#

    # This test passed on the command-line and behaves well, but temporarily removing from CI . -DWM 2018/11

    # remove the next line when msiGetMoreRows always returns an accurate value for continueInx
    @unittest.skipUnless(Allow_Intensive_Memory_Use, 'Skip nested paged iterators pending rsGenQuery continueInx fix')
    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for python REP')
    def test_nested_paged_iterators(self):

        if not self.full_test : return

        lib.create_directory_of_small_files(self.dir_for_coll,1000)
        lib.execute_command ('tar -cf {} {}'.format(self.bundled_coll, self.dir_for_coll))
        self.admin.assert_icommand('icd')
        self.admin.assert_icommand('iput -f {}'.format(self.bundled_coll))
        self.admin.assert_icommand('ibun -x {} .'.format(self.bundled_coll))

        rule_file = ""

        with generateRuleFile( names_list = self.to_unlink, **{'prefix':"qenquery_nest_paged"} ) as f:
            rule_file = f.name
            print(D_(
            '''\
            def main(rule_args,callback,rei):
                collect = global_vars['*testcollection'][1:-1]
                from genquery import row_iterator, paged_iterator, AS_LIST

                cond1 = "DATA_NAME like '1__' || like '2__' || like '3__' "
                cond2 = "DATA_NAME like '1' || like '2' "
                cond3 = "DATA_NAME like '7__' || like '8__' || like '9__' "

                condString1 = "COLL_NAME = '{}' and {}".format(collect,cond1)
                condString2 = "COLL_NAME = '{}' and {}".format(collect,cond2)
                condString3 = "COLL_NAME = '{}' and {}".format(collect,cond3)

                num_lines = 0
                for x_rows in paged_iterator( "order_asc(DATA_NAME)",condString1, AS_LIST,callback):
                    for x in x_rows:
                        for y in row_iterator( "order_asc(DATA_NAME)", condString2, AS_LIST,callback):
                            for z_rows in paged_iterator( "order_asc(DATA_NAME)",condString3, AS_LIST,callback):
                                for z in z_rows:
                                    output_line = x[0] + y[0] + z[0]
                                    num_lines += 1
                                    callback.writeLine('stdout', output_line)
                callback.writeLine('stderr', str(num_lines))


            INPUT  *testcollection=$""
            OUTPUT ruleExecOut
            '''), file=f, end='')

        command2run = ("irule -F " + rule_file + " " +
                       "\*testcollection=\"'{}'\"").format(self.test_admin_coll_path)

        out, err, _ = self.admin.run_icommand( command2run )
        self.assertEqual( err.strip(), str( 2 * 300 ** 2 ))
        nestedQueryError = None
        old_i = 0
        ## -- assert monotonic number sequence
        for i in map (int, (line for line in out.split("\n") if line.strip()) ):
            if nestedQueryError is None:
                nestedQueryError = False
            if i <= old_i : nestedQueryError = True
            old_i = i
        self.assertIsNotNone(nestedQueryError)
        self.assertFalse(nestedQueryError)


    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for python REP')
    def test_with_modified_rulebase_off_boundary(self):

        if self.full_test:
            IrodsController().stop()
            with lib.file_backed_up (self.server_config_path):
                self.boundary_with_size(512 + 7 * 8 * 9, self.modify_rulebase_and_tst, pre_start = True)
            IrodsController().start()


    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for python REP')
    def test_with_modified_rulebase_on_boundary(self):

        if self.full_test:
            IrodsController().stop()
            with lib.file_backed_up (self.server_config_path):
                #IrodsController().start()
                self.boundary_with_size(512 + 7 * 8 * 9, self.modify_rulebase_and_tst, pre_start = True)
                #IrodsController().stop()  # -- done internally by above call
            IrodsController().start()


    def boundary_with_size(self, N, func, pre_start = False):

        if pre_start:
            IrodsController().start() # -- for cases where Native REP was inserted to svr config

        lib.create_directory_of_small_files(self.dir_for_coll,N)
        lib.execute_command ('tar -cvf {} {}'.format(self.bundled_coll, self.dir_for_coll))
        self.admin.assert_icommand('icd')
        self.admin.assert_icommand('iput -f {}'.format(self.bundled_coll))
        self.admin.assert_icommand('ibun -x {} .'.format(self.bundled_coll))

        if func.__name__ == 'modify_rulebase_and_tst':
            self.admin.assert_icommand('ichmod -r read {} {}' .format( self.user0.username, self.dir_for_coll ))

        IrodsController().stop() # -- in preparation for core rule-base changes

        func(N)


    def create_py_rulefile_and_tst(self,N):

        with temporary_core_file() as core:

            core.prepend_to_imports( 'from genquery import test_python_RE_genquery_iterators' )

            rule_file = ""

            with generateRuleFile( names_list = self.to_unlink, **{'prefix':"test_core_py_mods_with_pyrule"} ) as f:

                rule_file = f.name

                rule_string = D_('''\
                                 def main(rule_args,callback,rei):
                                     a = "%s/%%" # collection  plus Sql match pattern
                                     b = global_vars['*rpi'][1:-1]  # rows_per_iter
                                     c = ""      # empty string (no verbose log dump)
                                     rpi = global_vars['*rpi'][1:-1]
                                     nobj = global_vars['*nobj'][1:-1]
                                     #callback.writeLine('serverLog', '*nobj = {}'.format(nobj))
                                     predicted = validate_output( nobj , rpi , "" )
                                     if b == "0": b = ""
                                     retv = callback.test_python_RE_genquery_iterators(a,b,c)
                                     a,b,c = retv['arguments']
                                     callback.writeLine('stdout', 'actual:    [{}][{}][{}]'.format(a,b,c))
                                     callback.writeLine('stdout', 'predicted: [{}]{}'.format( nobj,predicted))
                                 def validate_output(N, y, result):
                                     n = int(N); x = 0
                                     if y != '0': x=int(y)
                                     mfinal = 0 ; nlists = 0
                                     if x > 0: nlists = (n + x - 1) // x
                                     if nlists > 0: mfinal = x + n - nlists * x
                                     return "[{}][{}]".format(nlists,mfinal)
                                 input *rpi=$,*nobj=$
                                 output ruleExecOut
                         ''' % (self.test_admin_coll_path,))
                print(rule_string, file = f, end = '')

            IrodsController().start()

            irods_rule_language_option = ""  #default to python
            pattern=re.compile(r'^(\S+):\s*(\[.*\]$)',re.M)

            for rowsPerIter in ( "0", "7", "8", "9"): # if "0" then row_iterator; else paged_iterator
                cmd2run = '''irule {} -F {} \*rpi='"{}"' \*nobj="'{}'" '''.format(
                             irods_rule_language_option, rule_file, rowsPerIter, N )
                out, _, _  = self.admin.run_icommand( cmd2run )
                out_tuple = pattern.findall(out)
                self.assertNotEqual( out_tuple[0][0], out_tuple[1][0] ) # keys not a match ("actual" != "predicted")
                self.assertEqual   ( out_tuple[0][1], out_tuple[1][1] ) # but values should match

            IrodsController().stop()

    def modify_rulebase_and_tst (self,N):

        irods_config = IrodsConfig()
        orig = irods_config.server_config['plugin_configuration']['rule_engines']

        irods_config.server_config['plugin_configuration']['rule_engines'] = orig + [{
                'instance_name': 'irods_rule_engine_plugin-irods_rule_language-instance',
                'plugin_name': 'irods_rule_engine_plugin-irods_rule_language',
                'plugin_specific_configuration': {
                    "re_data_variable_mapping_set": [
                        "core"
                    ],
                    "re_function_name_mapping_set": [
                        "core"
                    ],
                    "re_rulebase_set": [
                        "core"
                    ],
                    "regexes_for_supported_peps": [
                        "ac[^ ]*",
                        "msi[^ ]*",
                        "[^ ]*pep_[^ ]*_(pre|post)"
                    ]
                },
                "shared_memory_instance": "irods_rule_language_rule_engine"
            }]

        irods_config.commit(irods_config.server_config, irods_config.server_config_path, make_backup=True)

        with temporary_core_file() as core:

            core.prepend_to_imports( 'from genquery import test_python_RE_genquery_iterators' )

            rule_file = ""

            with generateRuleFile( names_list = self.to_unlink, **{'prefix':"test_core_py_mods_"} ) as f:

                rule_file = f.name

                rule_string = D_('''\
                         irodsRule() {
                             *a = "%s/%%" # collection  plus Sql match pattern
                             *b = "*rpi"  # rows_per_iter
                             *c = ""      # empty string (no verbose log dump)
                             *predicted = ""
                             validate_output(*nobj,*rpi,*predicted)
                             #-- should assert that *a == *nobj
                             if ("*b" == "0") { *b = "" } # bc irule chokes on '\*rpi=""'
                             test_python_RE_genquery_iterators(*a,*b,*c)
                             writeLine("stdout", "actual:    [*a]:[*b][*c]")
                             writeLine("stdout", "predicted: [*nobj]:*predicted")
                         }
                         validate_output(*N , *y, *result) {
                             *n = int(*N) ; *x = 0
                             if ("*y" != "") { *x = int(*y) }
                             *mfinal = 0 ; *nlists = 0
                             if (*x > 0) { *nlists = int (( double(*n) + *x - 0.99) / *x) }
                             if (*nlists > 0) { *mfinal = *x + *n - (*nlists) * (*x)  }
                             *result = "[*nlists][*mfinal]"
                         }
                         input *rpi=$,*nobj=$
                         output ruleExecOut
                         ''' % (self.test_admin_coll_path,))
                print(rule_string, file = f, end = '')

            IrodsController().start()

            irods_rule_language_option = "-r irods_rule_engine_plugin-irods_rule_language-instance"
            pattern=re.compile(r'^(\S+):\s*(\[.*\]$)',re.M)

            for rowsPerIter in ( "0", "7", "8", "9"): # if "0" then row_iterator; else paged_iterator
                cmd2run = 'irule {} -F {} \*rpi="{}" \*nobj="{}"'.format(irods_rule_language_option, rule_file, rowsPerIter, N)
                out, _, _ = self.user0.run_icommand( cmd2run )
                out_tuple = pattern.findall(out)
                self.assertNotEqual( out_tuple[0][0], out_tuple[1][0]) # keys not a match ("actual" != "predicted")
                self.assertEqual(    out_tuple[0][1], out_tuple[1][1]) # but values should match

            IrodsController().stop()

    def tearDown(self):

        if self.full_test:

            # - clean up in admin home collection
            self.admin.assert_icommand('icd')
            self.admin.assert_icommand('irm -f {}'.format(self.bundled_coll))
            self.admin.assert_icommand('irm -rf {}'.format(self.dir_for_coll))

            # - clean up in local filesystem
            lib.execute_command('rm -fr {}/'.format(self.test_dir_path))
            lib.execute_command('rm -f {}'.format(self.test_tar_path))

            for f in self.to_unlink:
                os.unlink(f)

        super(Test_Genquery_Iterator, self).tearDown()

