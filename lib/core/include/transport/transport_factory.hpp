#ifndef IRODS_TRANSPORT_FACTORY_HPP
#define IRODS_TRANSPORT_FACTORY_HPP

/// \file

#include "transport.hpp"

namespace irods::experimental::io
{
    /// The base interface implemented by the standard iRODS transport factories.
    ///
    /// \since 4.2.9
    template <typename CharT,
              typename Traits = std::char_traits<CharT>>
    class transport_factory
    {
    public:
        virtual ~transport_factory() {}

        /// Instantiates a new transport object.
        ///
        /// \throws std::exception If the transport object cannot be instantiated. This is
        ///                        not a requirement and is left of to the implementation for
        ///                        what should happen on failure.
        ///
        /// \return A pointer to a ::transport<CharT, Traits> object.
        virtual transport<CharT, Traits>* operator()() = 0;
    }; // class transport_factory
} // namespace irods::experimental::io

#endif // IRODS_TRANSPORT_FACTORY_HPP
