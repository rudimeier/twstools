#ifndef PROPERTIES_H
#define PROPERTIES_H PROPERTIES_H

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QPair>



class QTime;


#define properties Properties::globalProperties()

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
		
		bool readConfigFile( const QString &confFileName );
		
		const QString& getConfFileName() const { return confFileName; }
		QByteArray getConfigBlob() const;

		bool exists(const QString &key) const;
		bool lookupValue(const QString &key, bool    &value) const;
		bool lookupValue(const QString &key, int     &value) const;
		bool lookupValue(const QString &key, float   &value) const;
		bool lookupValue(const QString &key, double  &value) const;
		bool lookupValue(const QString &key, QString &value) const;
		bool lookupValue(const QString &key, QVector<double>  &vector) const;
		bool lookupValue(const QString &key, QVector<QString> &vector) const;

		bool lookupValue(const QString &key, quint16 &value) const;


	private:
		static const Properties *_properties;
		
		static const QString FILE_IO_ERROR_MSQ;      // %1 = configFile
		static const QString PARSE_ERROR_MSG;        // %1 = configFile, %2 = error, %3 = line
		static const QString SETTING_NOT_FOUND_MSG;  // %1 = configFile, %2 = key
		static const QString SETTING_TYPE_WRONG_MSG; // %1 = type %2 = configFile, %3 = key

		libconfig::Config *conf;
		QString           confFileName;
};




class PropSub {
	public:
		QString configName;
		
		void initDefaults() {}
		bool readProperties() { return true; } //TODO check this again!
		
	protected:
		PropSub(const Properties& prop,const QString& cName) : configName(cName), prop(prop) {}
		
		bool exists(const QString& key) const;
		bool get(const QString& key, bool&    value) const;
		bool get(const QString& key, int&     value) const;
		bool get(const QString& key, float&   value) const;
		bool get(const QString& key, double&  value) const;
		bool get(const QString& key, QString& value) const;
		bool get(const QString& key, QTime&   value) const;
		bool get(const QString& key, QPair<double,double>& pair) const;
		bool get(const QString& key, QVector<double>&      vector) const;
		bool get(const QString& key, QVector<QString>&     vector) const;
		bool get(const QString& key, quint16&  value) const;
		
		const Properties& prop;
};



#endif
