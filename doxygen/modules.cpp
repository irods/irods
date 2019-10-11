/**
 * @file
 */

/**
@defgroup api API
@{
  @defgroup core Client Core Facility

  @defgroup capi_input_data_structures Input Data Structures

  @defgroup clientAPI Client API - Normally called by iRODS clients (client-server)
  @{
    @defgroup data_object Client Data Object Operations

    @defgroup collection_object Client Collection Operations

    @defgroup metadata Client Metadata Operations

    @defgroup rules Client Rule Operations

    @defgroup database Client Database Object Operations

    @defgroup authentication Client Authentication Operations

    @defgroup administration Client Administration Operations

    @defgroup delayed_execution Client Delayed Execution Operations

    @defgroup xmessage Client XMessage Operations

    @defgroup miscellaneous Client Miscellaneous Operations
  @}

  @defgroup serverAPI Server API - Normally called by iRODS servers (server-server)
  @{
    @defgroup server_datatransfer Server Data Transfer Operations

    @defgroup server_filedriver Server File Driver Operations

    @defgroup server_structuredfile Server Structured File (e.g. tar) Operations

    @defgroup server_icat Server iCAT Operations

    @defgroup server_miscellaneous Server Miscellaneous Operations
  @}
@}
**/



/**
@defgroup msi Microservices
@{
  @defgroup msi_workflow Workflow Microservices

  @defgroup msi_core Core Microservices
  @{
    @defgroup msi_ruleengine Rule Engine Microservices

    @defgroup msi_helper Helper Microservices
    @brief Can be called by client through irule.

    @defgroup msi_lowlevel Data Object Low-level Microservices
    @brief Can be called by client through irule.

    @defgroup msi_dataobject Data Object Microservices
    @brief Can be called by client through irule.

    @defgroup msi_collection Collection Microservices

    @defgroup msi_proxy Proxy Command Microservices

    @defgroup msi_icat iCAT Microservices
    @brief iCAT System Services

    @defgroup msi_string String Manipulation Microservices

    @defgroup msi_email Email Microservices

    @defgroup msi_kv Key-Value (Attr-Value) Microservices

    @defgroup msi_otheruser Other User Microservices

    @defgroup msi_system System Microservices
    @brief Can only be called by the server process

    @defgroup msi_admin Admin Microservices
    @brief Can only be called by an administrator
  @}
@}
**/

