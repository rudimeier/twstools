#include "tws_xml.h"

#include "debug.h"
#include "twsUtil.h"
#include "ibtws/Contract.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>





#define ADD_ATTR_INT( _struct_, _attr_ ) \
	if( !skip_defaults || _struct_._attr_ != dflt._attr_ ) { \
		snprintf(tmp, sizeof(tmp), "%d",_struct_._attr_ ); \
		xmlNewProp ( ne, (xmlChar*) #_attr_, (xmlChar*) tmp ); \
	}

#define ADD_ATTR_LONG( _struct_, _attr_ ) \
	if( !skip_defaults || _struct_._attr_ != dflt._attr_ ) { \
		snprintf(tmp, sizeof(tmp), "%ld",_struct_._attr_ ); \
		xmlNewProp ( ne, (xmlChar*) #_attr_, (xmlChar*) tmp ); \
	}

#define ADD_ATTR_DOUBLE( _struct_, _attr_ ) \
	if( !skip_defaults || _struct_._attr_ != dflt._attr_ ) { \
		snprintf(tmp, sizeof(tmp), "%.10g", _struct_._attr_ ); \
		xmlNewProp ( ne, (xmlChar*) #_attr_, (xmlChar*) tmp ); \
	}

#define ADD_ATTR_BOOL( _struct_, _attr_ ) \
	if( !skip_defaults || _struct_._attr_ != dflt._attr_ ) { \
		xmlNewProp ( ne, (xmlChar*) #_attr_, \
			(xmlChar*) (_struct_._attr_ ? "1" : "0") ); \
	}

#define ADD_ATTR_STRING( _struct_, _attr_ ) \
	if( !skip_defaults || _struct_._attr_ != dflt._attr_ ) { \
		xmlNewProp ( ne, (xmlChar*) #_attr_, \
			(xmlChar*) _struct_._attr_.c_str() ); \
	}


void conv_ib2xml( xmlNodePtr parent, const IB::ComboLeg& cl,
	bool skip_defaults )
{
	char tmp[128];
	static const IB::ComboLeg dflt;
	
	xmlNodePtr ne = xmlNewChild( parent, NULL, (xmlChar*)"ComboLeg", NULL);
	
	ADD_ATTR_LONG( cl, conId );
	ADD_ATTR_LONG( cl, ratio );
	ADD_ATTR_STRING( cl, action );
	ADD_ATTR_STRING( cl, exchange );
	ADD_ATTR_LONG( cl, openClose );
	ADD_ATTR_LONG( cl, shortSaleSlot );
	ADD_ATTR_STRING( cl, designatedLocation );
	ADD_ATTR_INT( cl, exemptCode );
	
	xmlAddChild(parent, ne);
}


void conv_ib2xml( xmlNodePtr parent, const IB::UnderComp& uc,
	bool skip_defaults )
{
	char tmp[128];
	static const IB::UnderComp dflt;
	
	xmlNodePtr ne = xmlNewChild( parent, NULL, (xmlChar*)"UnderComp", NULL);
	
	ADD_ATTR_LONG( uc, conId );
	ADD_ATTR_DOUBLE( uc, delta );
	ADD_ATTR_DOUBLE( uc, price );
	
	xmlAddChild(parent, ne);
}


void conv_ib2xml( xmlNodePtr parent, const IB::Contract& c, bool skip_defaults )
{
	char tmp[128];
	static const IB::Contract dflt;
	
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
	
	if( c.comboLegs != NULL ) {
		xmlNodePtr ncl = xmlNewChild( ne, NULL, (xmlChar*)"comboLegs", NULL);
		
		IB::Contract::ComboLegList::const_iterator it = c.comboLegs->begin();
		for ( it = c.comboLegs->begin(); it != c.comboLegs->end(); ++it) {
			conv_ib2xml( ncl, **it, skip_defaults );
		}
	}
	if( c.underComp != NULL ) {
		conv_ib2xml( ne, *c.underComp, skip_defaults );
	}
	
	xmlAddChild(parent, ne);
}


void conv_ib2xml( xmlNodePtr parent, const IB::ContractDetails& cd,
	bool skip_defaults )
{
	char tmp[128];
	static const IB::ContractDetails dflt;
	
	xmlNodePtr ne = xmlNewChild( parent, NULL,
		(xmlChar*)"IBContractDetails", NULL);
	
	conv_ib2xml( ne, cd.summary, skip_defaults );
	
	ADD_ATTR_STRING( cd, marketName );
	ADD_ATTR_STRING( cd, tradingClass );
	ADD_ATTR_DOUBLE( cd, minTick );
	ADD_ATTR_STRING( cd, orderTypes );
	ADD_ATTR_STRING( cd, validExchanges );
	ADD_ATTR_LONG( cd, priceMagnifier );
	ADD_ATTR_INT( cd, underConId );
	ADD_ATTR_STRING( cd, longName );
	ADD_ATTR_STRING( cd, contractMonth );
	ADD_ATTR_STRING( cd, industry );
	ADD_ATTR_STRING( cd, category );
	ADD_ATTR_STRING( cd, subcategory );
	ADD_ATTR_STRING( cd, timeZoneId );
	ADD_ATTR_STRING( cd, tradingHours );
	ADD_ATTR_STRING( cd, liquidHours );
	
	// BOND values
	ADD_ATTR_STRING( cd, cusip );
	ADD_ATTR_STRING( cd, ratings );
	ADD_ATTR_STRING( cd, descAppend );
	ADD_ATTR_STRING( cd, bondType );
	ADD_ATTR_STRING( cd, couponType );
	ADD_ATTR_BOOL( cd, callable );
	ADD_ATTR_BOOL( cd, putable );
	ADD_ATTR_DOUBLE( cd, coupon );
	ADD_ATTR_BOOL( cd, convertible );
	ADD_ATTR_STRING( cd, maturity );
	ADD_ATTR_STRING( cd, issueDate );
	ADD_ATTR_STRING( cd, nextOptionDate );
	ADD_ATTR_STRING( cd, nextOptionType );
	ADD_ATTR_BOOL( cd, nextOptionPartial );
	ADD_ATTR_STRING( cd, notes );
	
	xmlAddChild(parent, ne);
}



#define GET_ATTR_INT( _struct_, _attr_ ) \
	tmp = (char*) xmlGetProp( node, (xmlChar*) #_attr_ ); \
	_struct_->_attr_ = tmp ? atoi( tmp ) : dflt._attr_; \
	free(tmp)

#define GET_ATTR_LONG( _struct_, _attr_ ) \
	tmp = (char*) xmlGetProp( node, (xmlChar*) #_attr_ ); \
	_struct_->_attr_ = tmp ? atol( tmp ) : dflt._attr_; \
	free(tmp)

#define GET_ATTR_DOUBLE( _struct_, _attr_ ) \
	tmp = (char*) xmlGetProp( node, (xmlChar*) #_attr_ ); \
	_struct_->_attr_ = tmp ? atof( tmp ) : dflt._attr_; \
	free(tmp)

#define GET_ATTR_BOOL( _struct_, _attr_ ) \
	tmp = (char*) xmlGetProp( node, (xmlChar*) #_attr_ ); \
	_struct_->_attr_ = tmp ? atoi( tmp ) : dflt._attr_; \
	free(tmp)

#define GET_ATTR_STRING( _struct_, _attr_ ) \
	tmp = (char*) xmlGetProp( node, (xmlChar*) #_attr_ ); \
	_struct_->_attr_ = tmp ? std::string(tmp) : dflt._attr_; \
	free(tmp)


void conv_xml2ib( IB::ComboLeg* cl, const xmlNodePtr node )
{
	char* tmp;
	static const IB::ComboLeg dflt;

	GET_ATTR_LONG( cl, conId );
	GET_ATTR_LONG( cl, ratio );
	GET_ATTR_STRING( cl, action );
	GET_ATTR_STRING( cl, exchange );
	GET_ATTR_LONG( cl, openClose );
	GET_ATTR_LONG( cl, shortSaleSlot );
	GET_ATTR_STRING( cl, designatedLocation );
	GET_ATTR_INT( cl, exemptCode );
}

void conv_xml2ib( IB::UnderComp* uc, const xmlNodePtr node )
{
	char* tmp;
	static const IB::UnderComp dflt;
	
	GET_ATTR_LONG( uc, conId );
	GET_ATTR_DOUBLE( uc, delta );
	GET_ATTR_DOUBLE( uc, price );
}

void conv_xml2ib( IB::Contract* c, const xmlNodePtr node )
{
	char* tmp;
	static const IB::Contract dflt;
	
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
	
	for( xmlNodePtr p = node->children; p!= NULL; p=p->next) {
		if(p->name && (strcmp((char*) p->name, "comboLegs") == 0)) {
			if( c->comboLegs == NULL ) {
				c->comboLegs = new IB::Contract::ComboLegList();
			} else {
				c->comboLegs->clear();
			}
			for( xmlNodePtr q = p->children; q!= NULL; q=q->next) {
				IB::ComboLeg *cl = new IB::ComboLeg();
				conv_xml2ib( cl, q );
				c->comboLegs->push_back(cl);
			}
		} else if( p->name && (strcmp((char*) p->name, "UnderComp") == 0)) {
			if( c->underComp == NULL ) {
				c->underComp = new IB::UnderComp();
			}
			conv_xml2ib( c->underComp, p );
		}
		
	}
}


void conv_xml2ib( IB::ContractDetails* cd, const xmlNodePtr node )
{
	char* tmp;
	static const IB::ContractDetails dflt;
	
	xmlNodePtr xc = xmlFirstElementChild( node );
	conv_xml2ib( &cd->summary, xc );
	assert( strcmp((char*)xc->name,"IBContract") == 0 ); //TODO
	
	GET_ATTR_STRING( cd, marketName );
	GET_ATTR_STRING( cd, tradingClass );
	GET_ATTR_DOUBLE( cd, minTick );
	GET_ATTR_STRING( cd, orderTypes );
	GET_ATTR_STRING( cd, validExchanges );
	GET_ATTR_LONG( cd, priceMagnifier );
	GET_ATTR_INT( cd, underConId );
	GET_ATTR_STRING( cd, longName );
	GET_ATTR_STRING( cd, contractMonth );
	GET_ATTR_STRING( cd, industry );
	GET_ATTR_STRING( cd, category );
	GET_ATTR_STRING( cd, subcategory );
	GET_ATTR_STRING( cd, timeZoneId );
	GET_ATTR_STRING( cd, tradingHours );
	GET_ATTR_STRING( cd, liquidHours );
	
	// BOND values
	GET_ATTR_STRING( cd, cusip );
	GET_ATTR_STRING( cd, ratings );
	GET_ATTR_STRING( cd, descAppend );
	GET_ATTR_STRING( cd, bondType );
	GET_ATTR_STRING( cd, couponType );
	GET_ATTR_BOOL( cd, callable );
	GET_ATTR_BOOL( cd, putable );
	GET_ATTR_DOUBLE( cd, coupon );
	GET_ATTR_BOOL( cd, convertible );
	GET_ATTR_STRING( cd, maturity );
	GET_ATTR_STRING( cd, issueDate );
	GET_ATTR_STRING( cd, nextOptionDate );
	GET_ATTR_STRING( cd, nextOptionType );
	GET_ATTR_BOOL( cd, nextOptionPartial );
	GET_ATTR_STRING( cd, notes );
}




bool IbXml::skip_defaults = false;

IbXml::IbXml( const char* rootname )
{
	doc = xmlNewDoc( (const xmlChar*) "1.0");
	root = xmlNewDocNode( doc, NULL, (const xmlChar*) rootname, NULL );
	xmlDocSetRootElement( doc, root );
}

IbXml::~IbXml()
{
	xmlFreeDoc(doc);
}

void IbXml::setSkipDefaults( bool b )
{
	skip_defaults = b;
}

void IbXml::dump() const
{
	xmlDocFormatDump(stdout, doc, 1);
}

void IbXml::add( const IB::Contract& c )
{
	conv_ib2xml( root, c, skip_defaults );
}

void IbXml::add( const IB::ContractDetails& cd )
{
	conv_ib2xml( root, cd, skip_defaults );
}

xmlDocPtr IbXml::getDoc() const
{
	return doc;
}

xmlNodePtr IbXml::getRoot() const
{
	return root; 
}



