#include "irods_stacktrace.hpp"
#include "rodsErrorTable.h"
#include "irods_log.hpp"

#include <iostream>

#include <execinfo.h>
#include <stdlib.h>
#include <cxxabi.h>

namespace irods {

    static const int max_stack_size = 50;

    stacktrace::stacktrace( void ) {

    }

    stacktrace::~stacktrace( void ) {
        // TODO - stub
    }

    error stacktrace::trace( void ) {
        error result = SUCCESS();
        void** buffer = new void*[max_stack_size];
        stack_.clear();
        int size = backtrace( buffer, max_stack_size );
        if ( size ) {
            char** symbols = backtrace_symbols( buffer, size );
            if ( symbols ) {
                for ( int i = 1; i < size; ++i ) {
                    char* symbol = symbols[i];
                    if ( symbol ) {
                        std::string demangled;
                        std::string offset;
                        demangle_symbol( symbol, demangled, offset );
                        stack_entry_t entry;
                        entry.function = demangled;
                        entry.offset = offset;
                        entry.address = buffer[i];
                        stack_.push_back( entry );
                    }
                    else {
                        result = ERROR( NULL_VALUE_ERR, "Corrupt stack trace. Symbol is NULL." );
                    }
                }
                free( symbols );
            }
            else {
                result = ERROR( NULL_VALUE_ERR, "Cannot generate stack symbols" );
            }
        }
        else {
            result = ERROR( NULL_VALUE_ERR, "Stack trace is empty" );
        }
        delete [] buffer;
        return result;
    }

    error stacktrace::dump( std::ostream& strm_ ) {
        error result = SUCCESS();
        unsigned int max_function_length = 0;
        for ( stacklist::const_iterator it = stack_.begin(); it != stack_.end(); ++it ) {
            stack_entry_t entry = *it;
            if ( entry.function.length() > max_function_length ) {
                max_function_length = entry.function.length();
            }
        }
        int frame = 0;
        strm_ << std::endl << "Dumping stack trace" << std::endl;
        for ( stacklist::const_iterator it = stack_.begin(); it != stack_.end(); ++it ) {
            stack_entry_t entry = *it;
            strm_ << "<" << frame << ">";
            strm_ << "\t" << entry.function;
            int pad_amount = max_function_length - entry.function.length();
            for ( int i = 0; i < pad_amount; ++i ) {
                strm_ << " ";
            }
            strm_ << "\t" << "Offset: " << entry.offset;
            strm_ << "\t" << "Address: " << entry.address;
            strm_ << std::endl;
            ++frame;
        }
        strm_ << std::endl;
        return result;
    }

    error stacktrace::demangle_symbol(
        const std::string& _symbol,
        std::string& _rtn_name,
        std::string& _rtn_offset ) {
        error result = SUCCESS();
        _rtn_name = _symbol; // if we cannot demangle the symbol return the original.
        _rtn_offset.clear();

        // find the open paren
        size_t startpos = _symbol.find( "(" );
        size_t offsetpos = _symbol.find( "+", startpos );
        size_t nameendpos = _symbol.find( ")", startpos );

        if ( startpos != std::string::npos && nameendpos != std::string::npos ) {
            ++startpos;
            std::string name_symbol;
            std::string offset_string;
            if ( offsetpos != std::string::npos ) { // handle the case where there is no offset
                name_symbol = _symbol.substr( startpos, offsetpos - startpos );
                ++offsetpos;
                offset_string = _symbol.substr( offsetpos, nameendpos - offsetpos );
            }
            else {
                name_symbol = _symbol.substr( startpos, nameendpos - startpos );
            }
            char* name_buffer;
            int status;
            name_buffer = abi::__cxa_demangle( name_symbol.c_str(), NULL, NULL, &status );
            if ( status == 0 ) {
                _rtn_name = name_buffer;
                if ( !offset_string.empty() ) {
                    _rtn_offset = offset_string;
                }
                free( name_buffer );
            }

        }
        return result;
    }

}; // namespace irods
