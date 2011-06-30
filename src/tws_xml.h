#ifndef TWS_XML_H
#define TWS_XML_H


namespace IB {
	class Contract;
}


typedef struct _xmlNode * xmlNodePtr;


void conv_ib2xml( xmlNodePtr parent, const IB::Contract& c );
void conv_xml2ib( IB::Contract* c, const xmlNodePtr node );



#endif
