// =-=-=-=-=-=-=-
// stl includes
#include <string>
#include <vector>

namespace irods {
// =-=-=-=-=-=-=-
// helper fcn to break up string into tokens for easy handling
    void string_tokenize(
        const std::string&          _str,
        const std::string&          _delim,
        std::vector< std::string >& _tok ) {

        // =-=-=-=-=-=-=-
        // Skip delimiters at beginning.
        std::string::size_type last_pos = _str.find_first_not_of( _delim, 0 );

        // =-=-=-=-=-=-=-
        // Find first "non-delimiter".
        std::string::size_type pos = _str.find_first_of( _delim, last_pos );

        // =-=-=-=-=-=-=-
        // iterate over string
        while ( std::string::npos != pos || std::string::npos != last_pos ) {
            // =-=-=-=-=-=-=-
            // Found a token, add it to the vector.
            std::string tt = _str.substr( last_pos, pos - last_pos );

            if ( tt[0] == ' ' ) {
                tt = tt.substr( 1 );
            }

            _tok.push_back( tt );

            // =-=-=-=-=-=-=-
            // Skip delimiters.  Note the "not_of"
            last_pos = _str.find_first_not_of( _delim, pos );

            // =-=-=-=-=-=-=-
            // Find next "non-delimiter"
            pos = _str.find_first_of( _delim, last_pos );

        } // while

    } // string_tokenize

} // namespace irods
