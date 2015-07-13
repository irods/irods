myTestRule {
    *status_original = $status;
    $status = *status_original + 1;
    *status_updated = $status
    if (*status_original != *status_updated) {
        failmsg(-1, "\$status variable explicily changed inside rule");
    }
}
INPUT null
OUTPUT ruleExecOut

