#include "irods_stacktrace.hpp"
#include "rodsErrorTable.h"
#include "irods_log.hpp"

#include <iostream>

#include <execinfo.h>
#include <cstdlib>
#include <cxxabi.h>

namespace irods {
    static const int max_stack_size = 500;
    error stacktrace::trace( void ) {
        error result = SUCCESS();
        std::vector<void*> buffer(max_stack_size);
        stack_.clear();
        const int size = backtrace(buffer.data(), max_stack_size);
        if (size) {
            char** symbols = backtrace_symbols(buffer.data(), size);
            if (symbols) {
                for (int i = 1; i < size; ++i) {
                    char* symbol = symbols[i];
                    if (symbol) {
                        std::string demangled;
                        std::string offset;
                        demangle_symbol( symbol, demangled, offset );
                        stack_entry_t entry;
                        entry.function = demangled;
                        entry.offset = offset;
                        entry.address = buffer[i];
                        stack_.push_back( entry );
                    } else {
                        result = ERROR( NULL_VALUE_ERR, "Corrupt stack trace. Symbol is NULL." );
                    }
                }
                free( symbols );
            } else {
                result = ERROR( NULL_VALUE_ERR, "Cannot generate stack symbols" );
            }
        } else {
            result = ERROR( NULL_VALUE_ERR, "Stack trace is empty" );
        }
        return result;
    }

    error stacktrace::dump(std::ostream& out_stream_) {
        error result = SUCCESS();
        uint64_t max_offset_length{0};
        for (const auto& entry : stack_) {
            if (entry.offset.length() > max_offset_length) {
                max_offset_length = entry.offset.length();
            }
        }
        out_stream_ << std::endl << "Dumping stack trace\n";
        uint64_t frame_index{0};
        for (auto it = std::begin(stack_); it != std::end(stack_); ++it, ++frame_index) {
            const auto& entry{*it};
            out_stream_ << "<" << frame_index << ">";
            out_stream_ << "\t" << "Offset: " << entry.offset;
            const uint64_t pad_amount{max_offset_length - entry.offset.length()};
            for (uint64_t i=0; i<pad_amount; ++i) {
                out_stream_ << " ";
            }
            out_stream_ << "\t" << "Address: " << entry.address;
            out_stream_ << "\t" << entry.function;
            out_stream_ << "\n";
        }
        out_stream_ << std::endl;
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
