#ifndef IRODS_SOCKET_INFORMATION_HPP
#define IRODS_SOCKET_INFORMATION_HPP

#include <vector>
#include <string>

std::string socket_fd_to_remote_address(const int fd);
std::vector<int> get_open_socket_file_descriptors(void);


#endif
