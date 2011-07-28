#ifndef TWS_XML_H
#define TWS_XML_H


namespace IB {
	class ComboLeg;
	class UnderComp;
	class Contract;
	class ContractDetails;
	class Execution;
}


typedef struct _xmlNode * xmlNodePtr;
typedef struct _xmlDoc * xmlDocPtr;


void conv_ib2xml( xmlNodePtr parent, const char* name, const IB::ComboLeg& c );
void conv_ib2xml( xmlNodePtr parent, const char* name, const IB::UnderComp& c );
void conv_ib2xml( xmlNodePtr parent, const char* name, const IB::Contract& c );
void conv_ib2xml( xmlNodePtr parent, const char* name,
	const IB::ContractDetails& c );
void conv_ib2xml( xmlNodePtr parent, const char* name, const IB::Execution& );

void conv_xml2ib( IB::ComboLeg* c, const xmlNodePtr node );
void conv_xml2ib( IB::UnderComp* c, const xmlNodePtr node );
void conv_xml2ib( IB::Contract* c, const xmlNodePtr node );
void conv_xml2ib( IB::ContractDetails* c, const xmlNodePtr node );
void conv_xml2ib( IB::Execution*, const xmlNodePtr node );




class TwsXml
{
	public:
		TwsXml();
		virtual ~TwsXml();
		
		static void setSkipDefaults( bool );
		static xmlNodePtr newDocRoot();
		static void dumpAndFree( xmlNodePtr root );
		
		bool openFile( const char *filename );
		xmlDocPtr nextXmlDoc();
		xmlNodePtr nextXmlRoot();
		xmlNodePtr nextXmlNode();
		
		static const bool &skip_defaults;
		
	private:
		void resize_buf();
		
		static bool _skip_defaults;
		
		void *file; // FILE*
		long buf_size;
		char *buf;
		xmlDocPtr curDoc;
		xmlNodePtr curNode;
};




#endif
