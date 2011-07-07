
#include <cstdio>
#include <libconfig.h++>
#include "properties.h"
#include "debug.h"
#include <QtCore/QString>



// %1 = configFile
static const QString FILE_IO_ERROR_MSQ("file io error %1");
// %1 = configFile, %2 = error, %3 = line
static const QString PARSE_ERROR_MSG("parse error in %1: %2, line %3");
// %1 = configFile, %2 = key
static const QString SETTING_NOT_FOUND_MSG("setting not found in %1: %2");
// %1 = type %2 = configFile, %3 = key
static const QString SETTING_TYPE_WRONG_MSG("wrong setting type (%1) in %2: %3");


const Properties* Properties::_properties = NULL;

Properties::Properties()
{
	if( _properties == NULL ) {
		conf = new libconfig::Config();
		_properties = this;
	} else {
		Q_ASSERT(false);
	}
}

Properties::~Properties()
{
	delete conf;
	Properties::_properties = NULL;
}

const Properties& Properties::globalProperties()
{
	Q_ASSERT( _properties != NULL );
	return *_properties;
}

bool Properties::readConfigFile( const std::string &confFileName )
{
	QString errorMsg;
	try{
 		conf->readFile(confFileName.c_str());
		this->confFileName = confFileName;
		conf->setAutoConvert(true);
		PROP_DEBUG( 1, QString("auto convert readConfigFile: %1")
			.arg(conf->getAutoConvert() ? "true" : "false") );
		return true;
	} catch(libconfig::FileIOException) {
		errorMsg = FILE_IO_ERROR_MSQ.arg(QString::fromStdString(confFileName));
	} catch(libconfig::ParseException e ) {
		errorMsg = PARSE_ERROR_MSG.arg(QString::fromStdString(confFileName))
			.arg(e.getError()).arg(e.getLine());
	}
	PROP_DEBUG( 0, errorMsg );
	return false;
}

bool Properties::exists(const std::string &key) const
{
	return conf->exists(key.c_str());
}

bool Properties::lookupValue(const std::string &key, bool &value) const
{
	QString errorMsg;
	try {
		value = (bool) conf->lookup(key);
		return true;
	} catch (libconfig::SettingNotFoundException) { 
		errorMsg = SETTING_NOT_FOUND_MSG;
	} catch(libconfig::SettingTypeException) {
		errorMsg = SETTING_TYPE_WRONG_MSG.arg("bool");
	}
	
	PROP_DEBUG( 3, errorMsg.arg(QString::fromStdString(confFileName))
		.arg(QString::fromStdString(key)));
	return false;
}

bool Properties::lookupValue(const std::string &key, int &value) const
{
	QString errorMsg;
	try {
		value = (int) conf->lookup(key);
		return true;
	} catch (libconfig::SettingNotFoundException) { 
		errorMsg = SETTING_NOT_FOUND_MSG;
	} catch(libconfig::SettingTypeException) {
		errorMsg = SETTING_TYPE_WRONG_MSG.arg("int");
	}
	
	PROP_DEBUG( 3, errorMsg.arg(QString::fromStdString(confFileName))
		.arg(QString::fromStdString(key)));
	return false;
}

bool Properties::lookupValue(const std::string &key, std::string &value) const
{
	QString errorMsg;
	try {
		value = (const char*) conf->lookup(key);
		return true;
	} catch (libconfig::SettingNotFoundException) { 
		errorMsg = SETTING_NOT_FOUND_MSG;
	} catch(libconfig::SettingTypeException) {
		errorMsg = SETTING_TYPE_WRONG_MSG.arg("string");
	}
	
	PROP_DEBUG( 3, errorMsg.arg(QString::fromStdString(confFileName))
		.arg(QString::fromStdString(key)));
	return false;
}

bool PropSub::exists(const std::string& key) const
{
	return prop.exists(configName+"."+key);
}

bool PropSub::get(const std::string& key, bool& value) const
{
	return prop.lookupValue(configName+"."+key,value);
}

bool PropSub::get(const std::string& key, int& value) const
{
	return prop.lookupValue(configName+"."+key,value);
}

bool PropSub::get(const std::string& key, std::string& value) const
{
	return prop.lookupValue(configName+"."+key,value);
}



