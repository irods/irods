/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef __EIRODS_STRING_TOKENIZE_HPP__
#define __EIRODS_STRING_TOKENIZE_HPP__

#include <string>
#include <vector>

namespace eirods {
    // =-=-=-=-=-=-=-
    // helper fcn to break up string into tokens for easy handling
    static void string_tokenize( std::string _s, std::vector< std::string >& _tok, std::string _delim = " " ) {
        using namespace std;

        // =-=-=-=-=-=-=-
        // Skip delimiters at beginning.
        string::size_type last_pos = _s.find_first_not_of( _delim, 0 );
 
        // =-=-=-=-=-=-=-
        // Find first "non-delimiter".
        string::size_type pos = _s.find_first_of( _delim, last_pos );

        // =-=-=-=-=-=-=-
        // iterate over string
        while( string::npos != pos || string::npos != last_pos ) {
            // =-=-=-=-=-=-=-
            // Found a token, add it to the vector.
            string tt = _s.substr( last_pos, pos - last_pos );
                        
            if( tt[0] == ' ' )
                tt = tt.substr( 1 );

            _tok.push_back( tt );
                        
            // =-=-=-=-=-=-=-
            // Skip delimiters.  Note the "not_of"
            last_pos = _s.find_first_not_of( _delim, pos );
                        
            // =-=-=-=-=-=-=-
            // Find next "non-delimiter"
            pos = _s.find_first_of( _delim, last_pos );

        } // while

    } // string_tokenize

}; // namespace eirods

#endif // __EIRODS_STRING_TOKENIZE_HPP__




