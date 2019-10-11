#ifndef IRODS_PATH_RECURSION_HPP
#define IRODS_PATH_RECURSION_HPP

#include "rodsPath.h"
#include "parseCommandLine.h"

#include <string>
#include <sstream>
#include <map>
#include <iomanip>
#include <chrono>

#include <boost/filesystem.hpp>

namespace irods
{
    // When a directory structure is walked before the icommand
    // operation, only a single occurrence of the canonical path
    // is allowed.
    typedef std::map< std::string, std::string > recursion_map_t;

    // The parameter path is the user path (not yet canonical).
    // Returns true if the user path is a directory which has not
    // yet been examined, or, throws an irods::exception if the path has
    // already been examined (it's in the set<> already).
    // The bool paramter is true if the "--link" flag was specified
    bool is_path_valid_for_recursion(boost::filesystem::path const &, recursion_map_t &, bool);

    // Called in from places where file system loop detection is not desired/needed,
    // regardless of whether or not the recursion_map_t has been initialized by
    // check_directories_for_loops().
    //
    // The rodsArguments_t object is for checking for the "--link" flag, and the
    // character buffer is for the user filename.
    // Checks for existence of path as a symlink or a directory.
    // Will throw irods::exception if boost fs errors occur in the process.
    bool is_path_valid_for_recursion( rodsArguments_t const * const, const char * );

    // Throws an irods::exception if a file system loop was detected
    void check_for_filesystem_loop(boost::filesystem::path const &,    // Canonical path
                                   boost::filesystem::path const &,    // user path (just for emitting decent exception messages)
                                   recursion_map_t &);

    // This is what the icommand xxxxUtil() function uses to scan recursively
    // for directories/symlinks. The path is the user's non-canonical path, and
    // the recursion_map_t is statically defined by the caller as needed.
    // The bool paramter is true if the "--link" flag was specified
    int check_directories_for_loops( boost::filesystem::path const &, irods::recursion_map_t &, bool);

    // Issue 3988: For irsync and iput mostly, scan all source physical directories
    // for loops before doing any actual file transfers. Returns a 0 for success or
    // rodsError error (< 0).  The bool specifies whether the "--link" flag was specified.
    int scan_all_source_directories_for_loops(irods::recursion_map_t &, const std::vector<std::string>&, bool);

    // This function does the filesystem loop and sanity check
    // for both irsync and iput
    int file_system_sanity_check( irods::recursion_map_t &,
                                  rodsArguments_t const * const,
                                  rodsPathInp_t const * const);

    // Issue 4006: disallow mixed files and directory sources with the
    // recursive (-r) option.
    int disallow_file_dir_mix_on_command_line( rodsArguments_t const * const rodsArgs,
                                               rodsPathInp_t const * const rodsPathInp );

    // exporting this env variable will actually allow the scantime result to be printed
    static const char *chrono_env = "IRODS_SCAN_TIME";

    class scantime
    {
        public:
            explicit scantime();
            virtual ~scantime();
            std::string get_duration_string() const;
        private:
            std::chrono::time_point<std::chrono::high_resolution_clock> start_;
    };
} // namespace irods

#endif // IRODS_PATH_RECURSION_HPP
