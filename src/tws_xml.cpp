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






#define ADD_ATTR_LONG( _struct_, _attr_ ) \
	snprintf(tmp, sizeof(tmp), "%ld",_struct_._attr_ ); \
	xmlNewProp ( ne, (xmlChar*) #_attr_, (xmlChar*) tmp )

#define ADD_ATTR_DOUBLE( _struct_, _attr_ ) \
	snprintf(tmp, sizeof(tmp), "%.10g", _struct_._attr_ ); \
	xmlNewProp ( ne, (xmlChar*) #_attr_, (xmlChar*) tmp )

#define ADD_ATTR_BOOL( _struct_, _attr_ ) \
	xmlNewProp ( ne, (xmlChar*) #_attr_, \
		(xmlChar*) (_struct_._attr_ ? "1" : "0") )

#define ADD_ATTR_STRING( _struct_, _attr_ ) \
	xmlNewProp ( ne, (xmlChar*) #_attr_, (xmlChar*) _struct_._attr_.c_str() );
	

void conv_ib2xml( xmlNodePtr parent, const IB::Contract& c )
{
	char tmp[128];
	
	xmlNodePtr ne = xmlNewChild( parent, NULL, (xmlChar*)"IBContract", NULL);
	
	ADD_ATTR_LONG( c, conId );
	ADD_ATTR_STRING( c, symbol );
	ADD_ATTR_STRING( c, secType );
	ADD_ATTR_STRING( c, expiry );
	ADD_ATTR_DOUBLE( c, strike );
	ADD_ATTR_STRING( c, right );
	ADD_ATTR_STRING( c, multiplier );
	ADD_ATTR_STRING( c, exchange );
	ADD_ATTR_STRING( c, primaryExchange );
	ADD_ATTR_STRING( c, currency );
	ADD_ATTR_STRING( c, localSymbol );
	ADD_ATTR_BOOL( c, includeExpired );
	ADD_ATTR_STRING( c, secIdType );
	ADD_ATTR_STRING( c, secId );
	ADD_ATTR_STRING( c, comboLegsDescrip );
	
	xmlAddChild(parent, ne);
}


void conv_ib2xml( xmlNodePtr parent, const IB::ContractDetails& cd )
{
	char tmp[128];
	
	xmlNodePtr ne = xmlNewChild( parent, NULL,
		(xmlChar*)"IBContractDetails", NULL);
	
	
	xmlAddChild(parent, ne);
}




#define GET_ATTR_LONG( _struct_, _attr_ ) \
	tmp = (char*) xmlGetProp( node, (xmlChar*) #_attr_ ); \
	_struct_->_attr_ = tmp ? atol( tmp ) : dfltContract.conId; \
	free(tmp)

#define GET_ATTR_DOUBLE( _struct_, _attr_ ) \
	tmp = (char*) xmlGetProp( node, (xmlChar*) #_attr_ ); \
	_struct_->_attr_ = tmp ? atof( tmp ) : dfltContract.conId; \
	free(tmp)

#define GET_ATTR_BOOL( _struct_, _attr_ ) \
	tmp = (char*) xmlGetProp( node, (xmlChar*) #_attr_ ); \
	_struct_->_attr_ = tmp ? atof( tmp ) : dfltContract.conId; \
	free(tmp)

#define GET_ATTR_STRING( _struct_, _attr_ ) \
	tmp = (char*) xmlGetProp( node, (xmlChar*) #_attr_ ); \
	_struct_->_attr_ = tmp ? std::string(tmp) : dfltContract._attr_; \
	free(tmp)



void conv_xml2ib( IB::Contract* c, const xmlNodePtr node )
{
	char* tmp;
	static const IB::Contract dfltContract;
	
	GET_ATTR_LONG( c, conId );
	GET_ATTR_STRING( c, symbol );
	GET_ATTR_STRING( c, secType );
	GET_ATTR_STRING( c, expiry );
	GET_ATTR_DOUBLE( c, strike );
	GET_ATTR_STRING( c, right );
	GET_ATTR_STRING( c, multiplier );
	GET_ATTR_STRING( c, exchange );
	GET_ATTR_STRING( c, primaryExchange );
	GET_ATTR_STRING( c, currency );
	GET_ATTR_STRING( c, localSymbol );
	GET_ATTR_BOOL( c, includeExpired );
	GET_ATTR_STRING( c, secIdType );
	GET_ATTR_STRING( c, secId );
	GET_ATTR_STRING( c, comboLegsDescrip );
	// TODO comboLegs
	// TODO underComp
}


void conv_xml2ib( IB::ContractDetails* cd, const xmlNodePtr node )
{
	char* tmp;
	static const IB::ContractDetails dfltCntrctDtls;
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
