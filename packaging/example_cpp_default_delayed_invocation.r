{
	"policy" : "irods_policy_enqueue_rule",
        "delay_conditions" : "<PLUSET>1s</PLUSET>",
	"payload" : {
	    "policy" : "irods_policy_execute_rule",
            "payload" : {
	        "policy_to_invoke" : "irods_policy_engine_example",
                "policy_enforcement_point" : "pep_api_nopes",
                "event_name" : "CREATE",
                "parameters" : {
                    "key0" : "value0",
                    "key1" : "value1"
                 }
             }
        }
}
INPUT null
OUTPUT ruleExecOut

