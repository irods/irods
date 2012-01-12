# iRODS Rule Base
# The recovery procedure is added to each micro-service after the symbol " ::: "
#
acRegisterData{ acGetResource; msiRegisterData ::: recover_msiRegisterData; }
acDeleteData {ON(msiCheckPermission("delete") == 0) {msiDeleteData ::: recover_msiDeleteData; }}
acDeleteData {ON(msiCheckOwner == 0) {msiDeleteData ::: recover_msiDeleteData; }}
acGetResource {ON($rescName != "null") { } }
acGetResource {ON($rescName == "null") {msiGetDefaultResourceForData; }}
