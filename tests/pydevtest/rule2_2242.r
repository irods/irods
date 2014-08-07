test {
    $status = "1";
}

acPreProcForWriteSessionVariable(*x) {
    writeLine("stdout", "bwahahaha");
    succeed;
}

INPUT *A="status"
OUTPUT ruleExecOut

