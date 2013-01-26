



#ifndef __OPERATION_RULE_EXECUTION_OBJECT_H__
#define __OPERATION_RULE_EXECUTION_OBJECT_H__

// =-=-=-=-=-=-=-
// irods includes
#include "rcConnect.h"

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
    class operation_rule_execution_manager {
    public: 
        // =-=-=-=-=-=-=-
        /// @brief Constructor
        operation_rule_execution_manager( const std::string&,   // instance name
                                          const std::string& ); // operation name

        // =-=-=-=-=-=-=-
        /// @brief execute rule for pre operation
        error exec_pre_op( rsComm_t*,                         // client connection
                           const std::vector< std::string >&, // args to operation
                           std::string& );                    // results of call to rule

        // =-=-=-=-=-=-=-
        /// @brief execute rule for post operation
        error exec_post_op( rsComm_t*,                        // client connection
                           const std::vector< std::string >&, // args to operation
                            std::string& );                   // results of call to rule

    private:
        // =-=-=-=-=-=-=-
        /// @brief name of the rule minus the _pre or _post
        std::string rule_name_;

        // =-=-=-=-=-=-=-
        /// @brief execute rule for post operation
        error exec_op( const std::string&,                // rule name 
                       rsComm_t*,                         // client connection
                       const std::vector< std::string >&, // args to operation
                       std::string& );                    // results of call to rule

    }; // class operation_rule_execution_manager

}; // namespace eirods


#endif // __OPERATION_RULE_EXECUTION_OBJECT_H__



