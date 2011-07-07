#ifndef PROPERTIES_H
#define PROPERTIES_H

#include <string>


// debug verbosity 0...n
#define prop_debug_level 1 
#define PROP_DEBUG( _level, _msg )        \
	if( prop_debug_level >= _level ) {    \
		qDebug() << _msg;                 \
	}


namespace libconfig { class Config; }

class Properties {
	public:
		Properties();
		~Properties();
		
		static const Properties& globalProperties();
		
		bool readConfigFile( const std::string &confFileName );
		
		const std::string& getConfFileName() const { return confFileName; }

		bool exists(const std::string &key) const;
		bool lookupValue(const std::string &key, bool    &value) const;
		bool lookupValue(const std::string &key, int     &value) const;
		bool lookupValue(const std::string &key, std::string &value) const;


	private:
		static const Properties *_properties;
		
		libconfig::Config *conf;
		std::string confFileName;
};




class PropSub {
	public:
		std::string configName;
		
		void initDefaults() {}
		bool readProperties() { return true; } //TODO check this again!
		
	protected:
		PropSub(const Properties& prop,const std::string& cName) : configName(cName), prop(prop) {}
		
		bool exists(const std::string& key) const;
		bool get(const std::string& key, bool&    value) const;
		bool get(const std::string& key, int&     value) const;
		bool get(const std::string& key, std::string& value) const;
		
		const Properties& prop;
};



#endif
