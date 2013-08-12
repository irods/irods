



#ifndef __OPERATION_RULE_EXECUTION_OBJECT_H__
#define __OPERATION_RULE_EXECUTION_OBJECT_H__

// =-=-=-=-=-=-=-
// irods includes
#include "rcConnect.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_operation_rule_execution_manager_base.h"

// =-=-=-=-=-=-=-
// stl includes
#include <string>

namespace eirods {

	// =-=-=-=-=-=-=-
	/**
	  * \class  operation_rule_execution_manager
	  * \author Jason M. Coposky 
	  * \date   January 2013
	  * \brief  class which builds operation rule names, queries for the rule existence,
      *         and provides an interface for execution in a pre and post operation condition
	  **/
    class operation_rule_execution_manager : public 
        operation_rule_execution_manager_base {
    public: 
        /// =-=-=-=-=-=-=-
        /// @brief Constructor
        operation_rule_execution_manager( 
            const std::string&,   // name of the plugin
            const std::string& ); // operation name

        /// =-=-=-=-=-=-=-
        /// @brief necessary virtual dtor
        virtual ~operation_rule_execution_manager() {}

        /// =-=-=-=-=-=-=-
        /// @brief execute rule for pre operation
        error exec_pre_op( 
            std::string& ); // results of call to rule

        /// =-=-=-=-=-=-=-
        /// @brief execute rule for post operation
        error exec_post_op( 
            std::string& ); // results of call to rule

    private:
        /// =-=-=-=-=-=-=-
        /// @brief name of the rule minus the _pre or _post
        std::string rule_name_;

        /// =-=-=-=-=-=-=-
        /// @brief execute rule for post operation
        error exec_op( 
            const std::string&, // rule name 
            std::string& );     // results of call to rule

    }; // class operation_rule_execution_manager

}; // namespace eirods


#endif // __OPERATION_RULE_EXECUTION_OBJECT_H__



