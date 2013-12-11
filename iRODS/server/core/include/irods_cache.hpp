/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

namespace irods {

    /**
       Abstract base class that should be subclassed in order to encapsulate the method by which objects are retrieved from their
       source in order to populate the cache
    **/
// should this just be a functor?
    template <typename KeyType, typename ObjectType>
    class cache_fill_interface {
    public:
        virtual bool get_object_from_source( KeyType _key, ObjectType& _object ) = 0;
    }; // class cache_fill_interface

    /**
       Generic cache for storing objects that may otherwise be slow to retrieve in a store with relatively fast access
    **/
// Aging strategies?
    template <typename KeyType, typename ObjectType>
    class cache {
    public:
        cache( cache_fill_interface<KeyType, ObjectType>* _filler ) {
            filler = filler_;
        }
        virtual ~cache( void );

        bool contains( KeyType _key ) {
            ObjectType object;
            return fetch_object( _key, object );
        }

        bool  get( KeyType _key, ObjectType& _object ) {
            return fetch_object( _key, _object );
        }

        void clear( void ) {
            cache_map_.clear();
        }

    private:
        typedef std::map<KeyType, ObjectType> cache_map;
        cache_map  cached_objects_;
        cache_fill_interface<KeyType, ObjectType>* filler_;

        bool fetch_object( KeyType _key, ObjectType& _object ) {
            bool result = false;
            cache_map::const_iterator it = cached_objects_.find( _key );
            if ( it == cached_objects_.end() ) {
                if ( filler_->get_object_from_source( _key, _object ) ) {
                    result = true;
                }
            }
            else {
                _object = *it;
                result = true;
            }
            return result;
        }
    }; // class cache

}; // namespace irods
