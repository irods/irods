from __future__ import print_function
import os
import re
import sys
import ast

if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest

from contextlib import contextmanager
from tempfile import NamedTemporaryFile
from textwrap import dedent
from .. import lib
from ..configuration import IrodsConfig
from ..controller import IrodsController
from . import resource_suite
from ..core_file import temporary_core_file
from ..paths import (config_directory as irods_config_dir,
                     server_config_path)

GENQUERY_MODULE_BASENAME = 'genquery.py'

Allow_Intensive_Memory_Use = False

def _module_attribute(module,name,default=None):
    value = getattr(module,name,default)
    if callable(value):
        return value()
    return value

#  -------------
#  This function will be necessary until PREP tests are separate from the irods core repository:

def genquery_module_available():

    global Allow_Intensive_Memory_Use
    class EmptyModule(Exception): pass

    IRODS_CONFIG_DIR =  irods_config_dir()

    genquery_module_path = os.path.join( IRODS_CONFIG_DIR,  GENQUERY_MODULE_BASENAME)
    file_description = " module file '{}'".format (genquery_module_path)

    usable = False

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
            usable = True
            idx = -1
            #--> try import genquery module so that we can test optimally according to configuration
            try:
                sys.path.insert(0,IRODS_CONFIG_DIR)
                import genquery
                if _module_attribute(genquery,'AUTO_CLOSE_QUERIES'): Allow_Intensive_Memory_Use = True
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

    if not usable:
        print (" --- Not running genquery iterator tests --- " ,file=sys.stderr)

    return usable


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

    Statement_Table_Size = 50

    plugin_name = IrodsConfig().default_rule_engine_plugin

    full_test = genquery_module_available()

    def setUp(self):

        super(Test_Genquery_Iterator, self).setUp()

        self.to_unlink = []
        self.dir_cleanup = True

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


    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for python REP')
    def test_query_objects_case_insensitive(self):

        if not self.full_test : return
        os.mkdir(self.dir_for_coll)
        filename = "MyFiLe.TsT"
        for dummy in range(3):
            fullpath = os.path.join(self.dir_for_coll,filename)
            open(fullpath,"wb").write(b"bytes")
            filename = filename.lower() if dummy == 0 else filename.upper()
        lib.execute_command ('tar -cf {} {}'.format(self.bundled_coll, self.dir_for_coll))
        self.admin.assert_icommand('icd')
        self.admin.assert_icommand('iput -f {}'.format(self.bundled_coll))
        self.admin.assert_icommand('ibun -x {} .'.format(self.bundled_coll))
        test_coll = self.test_admin_coll_path

        with generateRuleFile(names_list = self.to_unlink,
                              prefix = "query_case_insensitive_") as f:
            rule_file = f.name
            rule_text = dedent('''\
            from genquery import *
            def main(args,callback,rei):
                q1 = Query(callback,'DATA_ID',
                     "COLL_NAME = '{{c}}' and DATA_NAME = '{{d}}'".format(c='{test_coll}',d='{filename}'),
                     case_sensitive=True)
                q2 = Query(callback,'DATA_ID',
                     "COLL_NAME = '{{c}}' and DATA_NAME = '{{d}}'".format(c='{test_coll}'.upper(),d='{filename}'.lower()),
                     case_sensitive=False)
                L= [ len([i for i in q]) for q in (q1,q2) ]
                callback.writeLine('stdout', repr(L).replace(chr(0x20),''))
            OUTPUT ruleExecOut
            ''')
            print(rule_text.format(**locals()), file=f, end='')

        output, err, rc = self.admin.run_icommand("irule -F " + rule_file)
        self.assertTrue(rc == 0, "icommand status ret = {r} output = '{o}' err='{e}'".format(r=rc,o=output,e=err))
        self.assertEqual(output.strip(), "[1,3]")

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for python REP')
    def test_query_objects(self):
        if not self.full_test : return
        os.mkdir(self.dir_for_coll)
        interesting = [0,1,2,100,254,255,256,257,258,510,511,512,513,514]
        octals = ['%04o'%x for x in interesting]
        max_count = sorted(interesting)[-1]
        for x in range(max_count+1):
             fname = os.path.join(self.dir_for_coll,'%04o'%x)
             with open(fname,'wb') as fb:
                 fb.write(os.urandom(x))
        lib.execute_command ('tar -cf {} {}'.format(self.bundled_coll, self.dir_for_coll))
        self.admin.assert_icommand('icd')
        self.admin.assert_icommand('iput -f {}'.format(self.bundled_coll))
        self.admin.assert_icommand('ibun -x {} .'.format(self.bundled_coll))
        test_coll = self.test_admin_coll_path
        rule_file = ""
        output=""
        rule_header = dedent("""\
            from genquery import *
            def main(rule_args,callback,rei):
                TestCollection = "{test_coll}"
            """)
        rule_footer = dedent("""\
            INPUT null
            OUTPUT ruleExecOut
            """)

        frame_rule = lambda text,indent=4: \
            rule_header+('\n'+' '*indent+('\n'+' '*indent).join(dedent(text).split("\n"))
                        )+"\n"+rule_footer
        #--------------------------------
        test_cases = [(
            frame_rule('''\
                L = []
                for j in {octals}:
                    cond_string= "COLL_NAME = '{{c}}' and DATA_NAME < '{{ltparam}}'".format(c=TestCollection,ltparam=j)
                    q = Query(callback,
                              columns=["DATA_NAME"],
                              conditions=cond_string)
                    L.append(q.total_rows())
                callback.writeLine("stdout", repr(L))
                '''),
            "total_rows_",
            lambda : ast.literal_eval(output) == interesting
        ), ( #----------------
            frame_rule('''\
                row_format = lambda row:"{{COLL_NAME}}/{{order(DATA_NAME)}}".format(**row)
                q = Query(callback,
                          columns=["order(DATA_NAME)","COLL_NAME"],
                          output=AS_DICT,
                          offset=0,limit=1,
                          conditions="COLL_NAME = '{{c}}'".format(c=TestCollection))
                res_1=[row_format(r) for r in q]
                row_count = q.total_rows()
                res_2=[row_format(r) for r in q.copy(offset=row_count-1)]
                callback.writeLine('stdout',repr( [len(x) for x in (res_1,res_2)] +\
                                                  [
                                                   (res_1 + res_2 ==
                                                    [TestCollection+"/"+x for x in ["%04o"%y for y in (0,{max_count})]])
                                                  ] ))
                '''),
            "offset_limit_",
            lambda : output.replace(" ","").strip() == "[1,1,True]"
        ), ( #----------------
            frame_rule('''\
                q = Query(callback,
                          columns=["DATA_ID"],
                          conditions="COLL_NAME = '{{c}}'".format(c=TestCollection))
                q_iter = iter( q )
                first_row = next(q_iter)
                callback.writeLine('stdout',repr([1+len([row for row in q_iter]),
                                                  q.copy().total_rows()]))
                '''),
            "copy_query_",
            lambda : ast.literal_eval(output) == [ max_count+1 ]*2
        ), ( #----------------
            frame_rule('''\
                from genquery import Option
                callback.msiModAVUMetadata("-C",TestCollection,"set", "aa", "bb", "cc")
                callback.msiModAVUMetadata("-C",TestCollection,"add", "aa", "bb", "dd")
                q = Query(callback,
                          columns=["META_COLL_ATTR_NAME","COLL_NAME"],
                          conditions="META_COLL_ATTR_NAME = 'aa' and COLL_NAME = '{{c}}'".format(c=TestCollection)
                )
                d0 = [ i for i in q ]
                d1 = [ i for i in q.copy(options=Option.NO_DISTINCT) ]
                callback.msiModAVUMetadata("-C",TestCollection,"rmw", "aa", "bb", "%")
                b = repr([len(d0),len(d1)])
                callback.writeLine('stdout',b)
                callback.msiDeleteUnusedAVUs()
                '''),
            "no_distinct_",
            lambda : ast.literal_eval(output) == [1,2]
        ), ( #----------------
            frame_rule('''\
                import itertools
                q = Query(callback,
                          columns=["order_asc(DATA_NAME)"],
                          conditions="DATA_NAME like '03%' and COLL_NAME = '{{c}}'".format(c=TestCollection)
                )
                q2 = q.copy(conditions="DATA_NAME like '05%' and COLL_NAME = '{{c}}'".format(c=TestCollection))
                chained_queries = itertools.chain(q, q2)
                compare_this = [x for x in ('%04o'%y for y in range({max_count}+1)) if x[:2] in ('03','05')]
                callback.writeLine('stdout',repr([x for x in chained_queries] == compare_this and len(compare_this)==128))
                '''),
            "iter_chained_queries_",
            lambda : ast.literal_eval(output) is True
        ), ( #----------------
            frame_rule('''\
                #import pprint # - for DEBUG only
                import itertools
                q = Query(callback,
                          columns=["DATA_NAME"],
                          conditions="COLL_NAME = '{{c}}'".format(c=TestCollection)
                )
                n_results={max_count}+1
                chunking_results = dict()
                for chunk_size in set([1,2,3,4,5,6,7,8,10,16,32,50,64,100,128,200,256,400,510,512,513,
                 n_results-2,n_results-1,n_results,n_results+1,n_results+2]):
                    q_iter = iter(q.copy())
                    L=[]
                    while True:
                        chunk = [row for row in itertools.islice(q_iter, 0, chunk_size)]
                        if not chunk: break
                        L.append(chunk)
                    chunking_results[chunk_size] = L
                check_Lresult = lambda chunkSize,L:\
                  [int(x[0],8) for x in L] == list(range(0,n_results,chunkSize)) and \
                  (0==len(L) or 1+((n_results-1)%chunkSize) == len(L[-1]))
                #callback.writeLine('serverLog',pprint.pformat(chunking_results)) # - for DEBUG only
                callback.writeLine('stdout',repr(list(check_Lresult(k,chunking_results[k]) for k in chunking_results)))
                '''),
            "chunked_queries_via_islice_",
            lambda : list(filter(lambda result:result is False, ast.literal_eval(output))) == []
        ), ('','','')]
        #--------------------------------
        for rule_text, file_prefix, assertion in test_cases:
            if not rule_text: break
            with generateRuleFile(names_list = self.to_unlink,
                                  prefix = file_prefix) as f:
                rule_file = f.name
                print(rule_text.format(**locals()), file=f, end='')
            output, err, rc = self.admin.run_icommand("irule -F " + rule_file)
            self.assertTrue(rc == 0, "icommand status ret = {r} output = '{o}' err='{e}'".format(r=rc,o=output,e=err))
            assertResult=assertion()
            if  not  assertResult: print( "failing output ====> " + output + "\n<====" )
            self.assertTrue(assertResult, "test failed for prefix: {}".format(file_prefix))

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

            core.add_rule(dedent('''\
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
                print(dedent('''\
                def main(rule_args,callback,rei):
                    TestCollection = irods_rule_vars['*testcollection'][1:-1]
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

    @staticmethod
    def int_from_bitfields( sub_ints , shift_incr ):
        value = 0
        shift = 0
        for i in sub_ints:
            value |= ( i << shift )
            shift += shift_incr
        return value

    @unittest.skipUnless(Allow_Intensive_Memory_Use, 'Skip non nested repeats pending rsGenQuery continueInx fix')
    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for python REP')
    def test_repeat_nonnested_with_brk__4438(self):
        self.repeat_nonnested(use_break = True,
                              int_for_compare = self.int_from_bitfields((1,1,1,0),shift_incr=10))

    @unittest.skipUnless(Allow_Intensive_Memory_Use, 'Skip non nested repeats pending rsGenQuery continueInx fix')
    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for python REP')
    def test_repeat_nonnested_without_brk__4438(self):
        self.repeat_nonnested(use_break = False,
                              int_for_compare = self.int_from_bitfields((256,2+256,512,0),shift_incr=10))

    def repeat_nonnested(self, use_break, int_for_compare):
        if not self.full_test : return
        Do_Break = use_break
        Outer_Loop_Reps = 2 * self.Statement_Table_Size + 1
        lib.create_directory_of_small_files(self.dir_for_coll,912)
        lib.execute_command('''sh -c "cd '{}' && rm ? ?? 1[0123]? 14[0123]"'''.format(self.dir_for_coll))
        lib.execute_command ('tar -cf {} {}'.format(self.bundled_coll, self.dir_for_coll))
        self.admin.assert_icommand('icd')
        self.admin.assert_icommand('iput -f {}'.format(self.bundled_coll))
        self.admin.assert_icommand('ibun -x {} .'.format(self.bundled_coll))

        IrodsController().stop()

        with temporary_core_file() as core:

            core.prepend_to_imports('from genquery import *') # import all names to core that are exported by genquery.py

            # with collection containing 144..911 inclusive
            core.add_rule(dedent('''\
                def my_rule_256_nonnested(rule_args, callback, rei):
                    coll = rule_args[0]
                    L = []
                    for repeat in range( {Outer_Loop_Reps} ):
                        n = 0
                        for w1 in row_iterator("DATA_NAME",
                                               "COLL_NAME = '{{}}' and DATA_NAME < '4'".format(coll), # 256 objects
                                               AS_LIST, callback):
                            n += 1
                            if {Do_Break}: break  ## ___ (256 << 0)
                        for x1 in row_iterator("DATA_NAME",
                                               "COLL_NAME = '{{}}' and DATA_NAME like '91_' || < '4'".format(coll), # 2 + 256 objects
                                               AS_LIST, callback):
                            n += 0x400
                            if {Do_Break}: break  ## ___ (2+256)<<10
                        for y1 in row_iterator("DATA_NAME",
                                               "COLL_NAME = '{{}}' and DATA_NAME >= '4'".format(coll), # 512 objects
                                               AS_LIST, callback):
                            n += 0x100000
                            if {Do_Break}: break ## ___ (512 << 20)
                        for z1 in row_iterator("DATA_NAME",
                                               "COLL_NAME = '{{}}' and DATA_NAME = '999'".format(coll), # 0 objects
                                               AS_LIST, callback):
                            n |= 0x40000000
                        L.append(n)
                        rule_args[0] = str(L[0]) if len(L) == {Outer_Loop_Reps} and L == L[:1] * len(L) else "-1"
                '''.format(**locals())))

            IrodsController().start()

            rule_file = ""
            with generateRuleFile( names_list = self.to_unlink, **{'prefix':"test_mult256_nn_"} ) as f:
                rule_file = f.name
                print(dedent('''\
                def main(rule_args,callback,rei):
                    TestCollection = irods_rule_vars['*testcollection'][1:-1]
                    retval = callback.my_rule_256_nonnested(TestCollection);
                    result = retval['arguments'][0]
                    callback.writeLine("stdout", "nonnested repeats test: {}".format(result))
                input *testcollection=$""
                output ruleExecOut
                '''), file=f, end='')

            run_irule = ("irule -F {} \*testcollection=\"'{}'\""
                                              ).format(rule_file, self.test_admin_coll_path,)

            self.admin.assert_icommand(run_irule, 'STDOUT_SINGLELINE', r'\s+{}$'.format(int_for_compare), use_regex=True)

            IrodsController().stop()

        IrodsController().start()
    #=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#

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
            core.add_rule(dedent('''\
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
                print(dedent('''\
                def main(rule_args,callback,rei):
                    TestCollection = irods_rule_vars['*testcollection'][1:-1]
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

        Do_Long_Nested_Test = False

        lib.create_directory_of_small_files(self.dir_for_coll,1000)
        lib.execute_command ('tar -cf {} {}'.format(self.bundled_coll, self.dir_for_coll))
        self.admin.assert_icommand('icd')
        self.admin.assert_icommand('iput -f {}'.format(self.bundled_coll))
        self.admin.assert_icommand('ibun -x {} .'.format(self.bundled_coll))

        rule_file = ""

        cond1 = "DATA_NAME like '1__' "
        cond2 = "DATA_NAME = '1' "
        cond3 = "DATA_NAME like '7__' || like '8__' || like '9__' "

        ( term1 , term2 , term3 ) = ( 100, 1, 300 )

        if Do_Long_Nested_Test :

            cond1 += " || like '2__' || like '3__' "
            term1 += 200

            cond2 += " || = '2' "
            term2 += 1

        with generateRuleFile( names_list = self.to_unlink, **{'prefix':"qenquery_nest_paged"} ) as f:
            rule_file = f.name
            print(dedent('''\
            def main(rule_args,callback,rei):
                collect = irods_rule_vars['*testcollection'][1:-1]
                from genquery import row_iterator, paged_iterator, AS_LIST

                condString1 = "COLL_NAME = '" + collect + "' and {cond1}"
                condString2 = "COLL_NAME = '" + collect + "' and {cond2}"
                condString3 = "COLL_NAME = '" + collect + "' and {cond3}"

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
            ''').format(**locals()), file=f, end='')

        command2run = ("irule -F " + rule_file + " " +
                       "\*testcollection=\"'{}'\"").format(self.test_admin_coll_path)

        out, err, _ = self.admin.run_icommand( command2run )
        self.assertEqual( err.strip(), str( term1 * term2 * term3 ))
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

    #=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#=-=-=-=-=-#

    @unittest.skipUnless(Allow_Intensive_Memory_Use, 'This test will fail in iRODS > vsn 4.2.6 if queries not auto-closed.')
    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for python REP')
    def test_queries_are_freeing_statements__4438(self):
        if not self.full_test : return
        self.dir_cleanup = False
        outer_loop_reps = 2 * self.Statement_Table_Size + 1
        collection_to_query = "/" + self.admin.zone_name + "/home/" + self.admin.username
        IrodsController().stop()
        with temporary_core_file() as core:
            core.prepend_to_imports( 'import genquery')
            core.add_rule(dedent('''\
                def attempt_to_fill_statement_table (rule_args, callback, rei):
                    valid_count = []
                    for m in range( {0} ):
                        n = 0
                        for z in genquery.row_iterator("COLL_NAME", "COLL_NAME = '{1}'", genquery.AS_LIST, callback):
                            n += 1
                        valid_count.append(n == 1)
                    callback.writeLine("stdout", str( 1 if len( valid_count ) == {0} and all (valid_count) else 0) )
                ''').format(outer_loop_reps , collection_to_query))
            IrodsController().start()
            run_irule = "irule attempt_to_fill_statement_table null ruleExecOut"
            self.admin.assert_icommand(run_irule, 'STDOUT_SINGLELINE', r'^1$', use_regex=True)
            IrodsController().stop()
        IrodsController().start()

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

                rule_string = dedent('''\
                                 def main(rule_args,callback,rei):
                                     a = "%s/%%" # collection  plus Sql match pattern
                                     b = irods_rule_vars['*rpi'][1:-1]  # rows_per_iter
                                     c = ""      # empty string (no verbose log dump)
                                     rpi = irods_rule_vars['*rpi'][1:-1]
                                     nobj = irods_rule_vars['*nobj'][1:-1]
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

                rule_string = dedent('''\
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

            self.admin.assert_icommand('icd')

            if self.dir_cleanup:
                # - clean up in admin home collection
                self.admin.assert_icommand('irm -f {}'.format(self.bundled_coll))
                self.admin.assert_icommand('irm -rf {}'.format(self.dir_for_coll))

                # - clean up in local filesystem
                lib.execute_command('rm -fr {}/'.format(self.test_dir_path))
                lib.execute_command('rm -f {}'.format(self.test_tar_path))

            for f in self.to_unlink:
                os.unlink(f)

        super(Test_Genquery_Iterator, self).tearDown()

