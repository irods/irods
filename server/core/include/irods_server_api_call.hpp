#ifndef IRODS_SERVER_API_CALL_HPP__
#define IRODS_SERVER_API_CALL_HPP__

#include "irods_server_api_table.hpp"
#include "rodsErrorTable.h"
#include "rodsDef.h"

namespace irods {

    template< typename INP_T, typename OUT_T >
    int server_api_call(
        int          _api_index,
        rsComm_t *   _comm,
        INP_T  *     _input,
        bytesBuf_t * _input_buffer,
        OUT_T **     _output,
        bytesBuf_t * _output_buffer ) {
        api_entry_table& table = get_server_api_table();
        bool has_entry = table.has_entry( _api_index );
        if ( !has_entry ) {
            return SYS_UNMATCHED_API_NUM;
        }

        return table[_api_index].get()->call_wrapper(
                   table[_api_index].get(),
                   _comm,
                   _input,
                   _input_buffer,
                   _output,
                   _output_buffer );
    }

    template< typename INP_T, typename OUT_T >
    int server_api_call(
        int        _api_index,
        rsComm_t * _comm,
        INP_T *    _input,
        OUT_T **   _output ) {
        api_entry_table& table = get_server_api_table();
        bool has_entry = table.has_entry( _api_index );
        if ( !has_entry ) {
            return SYS_UNMATCHED_API_NUM;
        }

        return table[_api_index]->call_wrapper(
                   table[_api_index].get(),
                   _comm,
                   _input,
                   _output );
    }

    template< typename INP_T, typename OUT_T >
    int server_api_call(
        int          _api_index,
        rsComm_t *   _comm,
        INP_T *      _input,
        bytesBuf_t * _input_buffer,
        OUT_T **     _output ) {
        api_entry_table& table = get_server_api_table();
        bool has_entry = table.has_entry( _api_index );
        if ( !has_entry ) {
            return SYS_UNMATCHED_API_NUM;
        }
        return table[_api_index]->call_wrapper(
                   table[_api_index].get(),
                   _comm,
                   _input,
                   _input_buffer,
                   _output );
    }

    template< typename INP_T >
    int server_api_call(
        int          _api_index,
        rsComm_t *   _comm,
        INP_T *      _input,
        bytesBuf_t * _input_buffer ) {
        api_entry_table& table = get_server_api_table();
        bool has_entry = table.has_entry( _api_index );
        if ( !has_entry ) {
            return SYS_UNMATCHED_API_NUM;
        }
        return table[_api_index]->call_wrapper(
                   table[_api_index].get(),
                   _comm,
                   _input,
                   _input_buffer );
    }

    template< typename INP_T, typename OUT_T >
    int server_api_call(
        int          _api_index,
        rsComm_t *   _comm,
        INP_T *      _input,
        OUT_T **     _output,
        bytesBuf_t * _output_buffer ) {
        api_entry_table& table = get_server_api_table();
        bool has_entry = table.has_entry( _api_index );
        if ( !has_entry ) {
            return SYS_UNMATCHED_API_NUM;
        }
        return table[_api_index]->call_wrapper(
                   table[_api_index].get(),
                   _comm,
                   _input,
                   _output,
                   _output_buffer );
    }

    template< typename INP_T >
    int server_api_call(
        int        _api_index,
        rsComm_t * _comm,
        INP_T *    _input ) {
        api_entry_table& table = get_server_api_table();
        bool has_entry = table.has_entry( _api_index );
        if ( !has_entry ) {
            return SYS_UNMATCHED_API_NUM;
        }
        return table[_api_index]->call_wrapper(
                   table[_api_index].get(),
                   _comm,
                   _input );
    }

    template <typename INP_T, typename OUT_T>
    int server_api_call_without_policy(
        int         _api_index,
        rsComm_t*   _comm,
        INP_T*      _input,
        bytesBuf_t* _input_buffer,
        OUT_T**     _output,
        bytesBuf_t* _output_buffer)
    {
        auto& table = get_server_api_table();

        if (!table.has_entry(_api_index)) {
            return SYS_UNMATCHED_API_NUM;
        }

        return table[_api_index]->call_handler_without_policy(
                   _comm,
                   _input,
                   _input_buffer,
                   _output,
                   _output_buffer);
    }

    template <typename INP_T, typename OUT_T>
    int server_api_call_without_policy(
        int       _api_index,
        rsComm_t* _comm,
        INP_T*    _input,
        OUT_T**   _output)
    {
        auto& table = get_server_api_table();

        if (!table.has_entry(_api_index)) {
            return SYS_UNMATCHED_API_NUM;
        }

        return table[_api_index]->call_handler_without_policy(_comm, _input, _output);
    }

    template <typename INP_T, typename OUT_T>
    int server_api_call_without_policy(
        int         _api_index,
        rsComm_t*   _comm,
        INP_T*      _input,
        bytesBuf_t* _input_buffer,
        OUT_T**     _output)
    {
        auto& table = get_server_api_table();

        if (!table.has_entry(_api_index)) {
            return SYS_UNMATCHED_API_NUM;
        }

        return table[_api_index]->call_handler_without_policy(
                   _comm,
                   _input,
                   _input_buffer,
                   _output);
    }

    template <typename INP_T>
    int server_api_call_without_policy(
        int         _api_index,
        rsComm_t*   _comm,
        INP_T*      _input,
        bytesBuf_t* _input_buffer)
    {
        auto& table = get_server_api_table();

        if (!table.has_entry(_api_index)) {
            return SYS_UNMATCHED_API_NUM;
        }

        return table[_api_index]->call_handler_without_policy(
                   _comm,
                   _input,
                   _input_buffer);
    }

    template <typename INP_T, typename OUT_T>
    int server_api_call_without_policy(
        int         _api_index,
        rsComm_t*   _comm,
        INP_T*      _input,
        OUT_T**     _output,
        bytesBuf_t* _output_buffer)
    {
        auto& table = get_server_api_table();

        if (!table.has_entry(_api_index)) {
            return SYS_UNMATCHED_API_NUM;
        }

        return table[_api_index]->call_handler_without_policy(
                   _comm,
                   _input,
                   _output,
                   _output_buffer);
    }

    template <typename INP_T>
    int server_api_call_without_policy(
        int       _api_index,
        rsComm_t* _comm,
        INP_T*    _input)
    {
        auto& table = get_server_api_table();

        if (!table.has_entry(_api_index)) {
            return SYS_UNMATCHED_API_NUM;
        }

        return table[_api_index]->call_handler_without_policy(_comm, _input);
    }

} // namespace irods

#endif // IRODS_SERVER_API_CALL_HPP__

