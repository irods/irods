#ifndef IRODS_SERVER_API_CALL_HPP__
#define IRODS_SERVER_API_CALL_HPP__
#include "irods_server_api_table.hpp"
#include "rodsErrorTable.hpp"
#include "rodsDef.hpp"

namespace irods {

    template< typename INP_T, typename OUT_T >
    int server_api_call( int _api_index, rsComm_t * _comm, INP_T _input, bytesBuf_t * _input_buffer, OUT_T * _output, bytesBuf_t * _output_buffer ) {
        api_entry_table& table = get_server_api_table();
        bool has_entry = table.has_entry( _api_index );
        if ( !has_entry )
        {
            return SYS_UNMATCHED_API_NUM;
        }
        return table[_api_index]->svrHandler( _input, _comm, _input_buffer, _output, _output_buffer );
    }

    template< typename INP_T, typename OUT_T >
    int server_api_call( int _api_index, rsComm_t * _comm, INP_T _input, OUT_T * _output ) {
        return server_api_call( _api_index, _comm, _input, NULL, _output, NULL );
    }

    template< typename INP_T, typename OUT_T >
    int server_api_call( int _api_index, rsComm_t * _comm, INP_T _input, bytesBuf_t * _input_buffer, OUT_T * _output ) {
        return server_api_call( _api_index, _comm, _input, _input_buffer, _output );
    }

    template< typename INP_T, typename OUT_T >
    int server_api_call( int _api_index, rsComm_t * _comm, INP_T _input, OUT_T * _output, bytesBuf_t * _output_buffer ) {
        return server_api_call( _api_index, _comm, _input, NULL, _output, _output_buffer );
    }

    template< typename INP_T >
    int server_api_call( int _api_index, rsComm_t * _comm, INP_T _input ) {
        return server_api_call< INP_T, void * >( _api_index, _comm, _input, NULL, NULL, NULL );
    }

}; // namespace irods
#endif // IRODS_SERVER_API_CALL_HPP__
