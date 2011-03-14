#include "tws_xml.h"

#include "debug.h"
#include "twsUtil.h"
#include "ibtws/Contract.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>






#define ADD_ATTR_LONG( _attr_ ) \
	snprintf(tmp, sizeof(tmp), "%ld",c._attr_ ); \
	xmlNewProp ( ne, (xmlChar*) #_attr_, (xmlChar*) tmp )

#define ADD_ATTR_DOUBLE( _attr_ ) \
	snprintf(tmp, sizeof(tmp), "%.10g",c._attr_ ); \
	xmlNewProp ( ne, (xmlChar*) #_attr_, (xmlChar*) tmp )

#define ADD_ATTR_BOOL( _attr_ ) \
	xmlNewProp ( ne, (xmlChar*) #_attr_, (xmlChar*) (c._attr_ ? "1" : "0") )

#define ADD_ATTR_STRING( _attr_ ) \
	xmlNewProp ( ne, (xmlChar*) #_attr_, (xmlChar*) c._attr_.c_str() );
	

void conv_ib2xml( xmlNodePtr parent, const IB::Contract& c )
{
	char tmp[128];
	
	xmlNodePtr ne = xmlNewChild( parent, NULL, (xmlChar*)"IBContract", NULL);
	
	ADD_ATTR_LONG( conId );
	ADD_ATTR_STRING( symbol );
	ADD_ATTR_STRING( secType );
	ADD_ATTR_STRING( expiry );
	ADD_ATTR_DOUBLE( strike );
	ADD_ATTR_STRING( right );
	ADD_ATTR_STRING( multiplier );
	ADD_ATTR_STRING( exchange );
	ADD_ATTR_STRING( primaryExchange );
	ADD_ATTR_STRING( currency );
	ADD_ATTR_STRING( localSymbol );
	ADD_ATTR_BOOL( includeExpired );
	ADD_ATTR_STRING( secIdType );
	ADD_ATTR_STRING( secId );
	ADD_ATTR_STRING( comboLegsDescrip );
	
	xmlAddChild(parent, ne);
}



#define GET_ATTR_LONG( _attr_ ) \
	tmp = (char*) xmlGetProp( node, (xmlChar*) #_attr_ ); \
	c->_attr_ = tmp ? atol( tmp ) : dfltContract.conId; \
	free(tmp)

#define GET_ATTR_DOUBLE( _attr_ ) \
	tmp = (char*) xmlGetProp( node, (xmlChar*) #_attr_ ); \
	c->_attr_ = tmp ? atof( tmp ) : dfltContract.conId; \
	free(tmp)

#define GET_ATTR_BOOL( _attr_ ) \
	tmp = (char*) xmlGetProp( node, (xmlChar*) #_attr_ ); \
	c->_attr_ = tmp ? atof( tmp ) : dfltContract.conId; \
	free(tmp)

#define GET_ATTR_STRING( _attr_ ) \
	tmp = (char*) xmlGetProp( node, (xmlChar*) #_attr_ ); \
	c->_attr_ = tmp ? std::string(tmp) : dfltContract._attr_; \
	free(tmp)



void conv_xml2ib( IB::Contract* c, const xmlNodePtr node )
{
	char* tmp;
	static const IB::Contract dfltContract;
	
	GET_ATTR_LONG( conId );
	GET_ATTR_STRING( symbol );
	GET_ATTR_STRING( secType );
	GET_ATTR_STRING( expiry );
	GET_ATTR_DOUBLE( strike );
	GET_ATTR_STRING( right );
	GET_ATTR_STRING( multiplier );
	GET_ATTR_STRING( exchange );
	GET_ATTR_STRING( primaryExchange );
	GET_ATTR_STRING( currency );
	GET_ATTR_STRING( localSymbol );
	GET_ATTR_BOOL( includeExpired );
	GET_ATTR_STRING( secIdType );
	GET_ATTR_STRING( secId );
	GET_ATTR_STRING( comboLegsDescrip );
	// TODO comboLegs
	// TODO underComp
}




int main(int argc, char *argv[])
{
	xmlDocPtr doc = xmlNewDoc( (const xmlChar*) "1.0");
	xmlNodePtr root = xmlNewDocNode( doc, NULL, (xmlChar*)"root", NULL );
	xmlDocSetRootElement( doc, root );





	IB::Contract ic_orig, ic_conv, ic_conv2;
	ic_orig.strike = 25.0;
	conv_ib2xml( root, ic_orig );
	
	xmlDocDump(stdout, doc);
	
	xmlNodePtr xml_contract2 = xmlFirstElementChild( root );
	conv_xml2ib( &ic_conv2, xml_contract2 );
	xmlDocDump(stdout, doc);
	
	#define DBG_EQUAL_FIELD( _attr_ ) \
		qDebug() << #_attr_ << ( ic_orig._attr_ == ic_conv2._attr_ );
	DBG_EQUAL_FIELD( conId );
	DBG_EQUAL_FIELD( symbol );
	DBG_EQUAL_FIELD( secType );
	DBG_EQUAL_FIELD( expiry );
	DBG_EQUAL_FIELD( strike );
	DBG_EQUAL_FIELD( right );
	DBG_EQUAL_FIELD( multiplier );
	DBG_EQUAL_FIELD( exchange );
	DBG_EQUAL_FIELD( primaryExchange );
	DBG_EQUAL_FIELD( currency );
	DBG_EQUAL_FIELD( localSymbol );
	DBG_EQUAL_FIELD( includeExpired );
	DBG_EQUAL_FIELD( secIdType );
	DBG_EQUAL_FIELD( secId );
	DBG_EQUAL_FIELD( comboLegsDescrip );
	
	xmlFreeDoc(doc);
	
	return 0;
}
