LOOPTOTAL=10
CORERE=/etc/irods/core.re

echo -n "Swapping $CORERE $LOOPTOTAL times: "
for i in $(eval echo {1..$LOOPTOTAL}) ; do
    # counter
    echo -n ".";
    # initialize
    touch $CORERE
    sleep 1
    # save original core.re
    cp $CORERE $CORERE.orig
    # get initial state
    irule "msiAdmShowIRB" null ruleExecOut > 1
    # wait to cross a 1-second boundary
    sleep 1
    # update core.re
    echo "firstupdate{}" >> $CORERE
    # get second state
    irule "msiAdmShowIRB" null ruleExecOut > 2
    # update core.re again
    echo "secondupdate{}" >> $CORERE
    # get third state
    irule "msiAdmShowIRB" null ruleExecOut > 3
    # diff 1 and 2 -- should be different
    diff 1 2 > /dev/null
    if [ $? -eq 0 ] ; then
        echo "FAIL: 1 == 2, but should have been different"
        EXITEARLY=1
    fi
    # diff 2 and 3 -- should be different
    diff 2 3 > /dev/null
    if [ $? -eq 0 ] ; then
        echo "FAIL: 2 == 3, but should have been different"
        EXITEARLY=1
    fi
    # restore original core.re
    cp $CORERE.orig $CORERE
    # exit code
    if [ "$EXITEARLY" = "1" ] ; then
        exit 1
    fi
done
echo " ok"
exit 0
