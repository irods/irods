


#ifndef ___EIRODS_PLUGIN_H__
#define ___EIRODS_PLUGIN_H__



namespace eirods {

    class eirods_plugin {
        public:
		eirods_plugin() {}
		virtual ~eirods_plugin() {}
		virtual bool delay_load( void* ) = 0;

	}; // class eirods_plugin
	
	eirods_plugin* load_plugin( const std::string, const std::string );

}; // namespace eirods



#endif // ___EIRODS_PLUGIN_H__



