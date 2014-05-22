test {
  irods_msvc_test( "1", "2", "3", *out );
  writeLine('stdout', *out);
}
input null
output ruleExecOut
