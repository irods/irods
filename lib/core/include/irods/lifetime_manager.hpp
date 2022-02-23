#ifndef IRODS_LIFETIME_MANAGER_HPP
#define IRODS_LIFETIME_MANAGER_HPP

#include "irods/rcMisc.h"

#include <utility>

namespace irods::experimental {
    /// \brief Tag which indicates lifetime_manager is holding an array.
    ///
    /// \since 4.2.8
    static struct array_delete {
    } array_delete;

    /// \class lifetime_manager
    ///
    /// \brief Manages the lifetime of a legacy iRODS structure.
    ///
    /// This class serves to wrap existing legacy iRODS C-style structures which
    /// are allocated on the free-store to enable RAII semantics. This class should
    /// only be used with structures allocated on the free-store.
    ///
    /// This class can be thought of as a partial implementation of a std::unique_ptr
    /// with a custom deleter. Once a lifetime_manager has acquired ownership, however,
    /// the ownership cannot be taken back and the memory will be freed on scope exit.
    ///
    /// Whenever a new struct needs to be supported, the appropriate instructions
    /// for deallocation should be provided in the destructor for this class.
    ///
    /// \since 4.2.8
    template <typename T>
    class lifetime_manager {
    public:
        // Disables copy
        lifetime_manager(const lifetime_manager&) = delete;
        lifetime_manager& operator=(const lifetime_manager&) = delete;

        /// \fn lifetime_manager(lifetime_manager&&)
        ///
        /// \brief Move constructor
        ///
        /// \param[in] _rhs - r-value reference to another lifetime_manager.
        ///
        /// \since 4.2.8
        lifetime_manager(lifetime_manager&& _rhs)
            : obj_{std::exchange(_rhs.obj_, nullptr)}
            , array_delete_{_rhs.array_delete_}
        {
        }

        /// \fn lifetime_manager& operator=(lifetime_manager&& _rhs)
        ///
        /// \brief Move assignment
        ///
        /// \param[in] _rhs - r-value reference to another lifetime_manager.
        ///
        /// \return lifetime_manager&
        ///
        /// \since 4.2.8
        lifetime_manager& operator=(lifetime_manager&& _rhs)
        {
            obj_ = std::exchange(_rhs.obj_, nullptr);
            array_delete_ = _rhs.array_delete_;
            return *this;
        }

        /// \fn explicit lifetime_manager(T& _obj)
        ///
        /// \brief Constructor taking reference to managed structure.
        ///
        /// Initializes the managed pointer to structure with address of
        /// the passed in reference. As array_delete was not indicated,
        /// the pointer is assumed to not point to an array. As such,
        /// operator delete[] will not be used on destruction.
        ///
        /// \param[in] _obj - Reference to allocated structure.
        ///
        /// \since 4.2.8
        explicit lifetime_manager(T& _obj)
            : obj_{&_obj}
            , array_delete_{}
        {
        }

        /// \brief Constructor taking array_delete tag and pointer to
        /// managed structure.
        ///
        /// Initializes the managed pointer to structure with passed-in
        /// argument. As array_delete was indicated, the pointer is
        /// assumed to point to an array. As such, operator delete[]
        /// will be used on destruction provided T is not a specially
        /// supported type.
        ///
        /// \param[in] array_delete - Tag struct which indicates that the
        /// passed-in pointer points to an array of structures.
        /// \param[in] _obj - Pointer to allocated structure.
        ///
        /// \since 4.2.8
        explicit lifetime_manager(struct array_delete, T* _obj)
            : obj_{_obj}
            , array_delete_{true}
        {
        }

        /// \fn ~lifetime_manager()
        ///
        /// \brief Destructor
        ///
        /// If T is a specially supported type, the implementation provided
        /// for its deallocation is used. Otherwise, the delete operator for
        /// T is invoked (or, delete[] if delete_array tag was indicated).
        ///
        /// \since 4.2.8
        ~lifetime_manager()
        {
            if (!obj_) {
                return;
            }
            else if constexpr(std::is_same_v<T, dataObjInfo_t>) {
                freeAllDataObjInfo(obj_);
            }
            else if constexpr(std::is_same_v<T, keyValPair_t>) {
                clearKeyVal(obj_);
                free(obj_);
            }
            else if constexpr(std::is_same_v<T, genQueryOut_t>) {
                freeGenQueryOut(&obj_);
            }
            else if constexpr(std::is_same_v<T, genQueryInp_t>) {
                freeGenQueryInp(&obj_);
            }
            else if constexpr(std::is_same_v<T, rodsObjStat_t>) {
                freeRodsObjStat(obj_);
            }
            else if constexpr(std::is_same_v<T, bytesBuf_t>) {
                freeBBuf(obj_);
            }
            else if constexpr(std::is_same_v<T, rError_t>) {
                freeRError(obj_);
            }
            else if constexpr(std::is_same_v<T, openedDataObjInp_t>) {
                clearKeyVal(&obj_->condInput);
                free(obj_);
            }
            else if (array_delete_) {
                delete [] obj_;
            }
            else {
                delete obj_;
            }
        }

        /// \returns Pointer to managed structure
        ///
        /// \since 4.2.9
        auto get() -> T* { return obj_; }

        /// \brief Release the pointer from the lifetime manager
        ///
        /// \return Pointer to the managed structure
        ///
        /// \since 4.2.9
        auto release() -> T* {
            if (!obj_) {
                return nullptr;
            }

            T* ret = obj_;
            obj_ = nullptr;
            return ret;
        } // release

    private:
        /// \var T* obj_
        ///
        /// \brief Pointer to structured data allocated on free-store
        ///
        /// \since 4.2.8
        T* obj_;

        /// \var bool array_delete_
        ///
        /// \brief Flag indicating whether pointer refers to an array
        ///
        /// If true, delete[] will be used on destruction provided T is
        /// not a specially handled type.
        ///
        /// \since 4.2.8
        bool array_delete_;
    }; // class lifetime_manager
} // namespace irods::experimental

#endif // #ifndef IRODS_LIFETIME_MANAGER_HPP
