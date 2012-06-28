


#ifndef __EIRODS_PLUGIN_BASE_H__
#define __EIRODS_PLUGIN_BASE_H__

namespace eirods {

    class plugin_base {
        public:
		plugin_base() {}
		virtual ~plugin_base() {}
		virtual bool delay_load( void* ) = 0;

	}; // class plugin_base

}; // namespace eirods

#endif // __EIRODS_PLUGIN_BASE_H__



