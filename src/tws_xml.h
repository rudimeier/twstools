/*** tws_xml.h -- conversion between IB/API structs and xml
 *
 * Copyright (C) 2011-2018 Ruediger Meier
 * Author:  Ruediger Meier <sweet_f_a@gmx.de>
 * License: BSD 3-Clause, see LICENSE file
 *
 ***/

#ifndef TWS_XML_H
#define TWS_XML_H

/* it's a pain to get macro PRIdMAX on C++ */
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <twsapi/twsapi_config.h>

#ifndef TWSAPI_NO_NAMESPACE
namespace IB {
#endif
	class ComboLeg;
	class DeltaNeutralContract;
	class Contract;
	class ContractDetails;
	class Execution;
	class ExecutionFilter;
	class TagValue;
	class OrderComboLeg;
	class Order;
	class OrderState;
#ifndef TWSAPI_NO_NAMESPACE
}
using namespace IB;
#endif

typedef struct _xmlNode * xmlNodePtr;
typedef struct _xmlDoc * xmlDocPtr;


void conv_ib2xml( xmlNodePtr parent, const char* name, const ComboLeg& c );
void conv_ib2xml( xmlNodePtr parent, const char* name, const DeltaNeutralContract& c );
void conv_ib2xml( xmlNodePtr parent, const char* name, const Contract& c );
void conv_ib2xml( xmlNodePtr parent, const char* name,
	const ContractDetails& c );
void conv_ib2xml( xmlNodePtr parent, const char* name, const Execution& );
void conv_ib2xml( xmlNodePtr parent, const char* name,
	const ExecutionFilter& );
void conv_ib2xml( xmlNodePtr parent, const char* name, const TagValue& );
void conv_ib2xml( xmlNodePtr parent, const char* name,
	const OrderComboLeg& );
void conv_ib2xml( xmlNodePtr parent, const char* name, const Order& );
void conv_ib2xml( xmlNodePtr parent, const char* name, const OrderState& );

void conv_xml2ib( ComboLeg* c, const xmlNodePtr node );
void conv_xml2ib( DeltaNeutralContract* c, const xmlNodePtr node );
void conv_xml2ib( Contract* c, const xmlNodePtr node );
void conv_xml2ib( ContractDetails* c, const xmlNodePtr node );
void conv_xml2ib( Execution*, const xmlNodePtr node );
void conv_xml2ib( ExecutionFilter*, const xmlNodePtr node );
void conv_xml2ib( TagValue*, const xmlNodePtr node );
void conv_xml2ib( OrderComboLeg*, const xmlNodePtr node );
void conv_xml2ib( Order*, const xmlNodePtr node );
void conv_xml2ib( OrderState*, const xmlNodePtr node );


class ContractDetailsRequest;
class HistRequest;
class AccStatusRequest;
class ExecutionsRequest;
class OrdersRequest;
class PlaceOrder;
class MktDataRequest;

void to_xml( xmlNodePtr parent, const ContractDetailsRequest& );
void to_xml( xmlNodePtr parent, const HistRequest& );
void to_xml( xmlNodePtr parent, const AccStatusRequest& );
void to_xml( xmlNodePtr parent, const ExecutionsRequest& );
void to_xml( xmlNodePtr parent, const OrdersRequest& );
void to_xml( xmlNodePtr parent, const PlaceOrder& );
void to_xml( xmlNodePtr parent, const MktDataRequest& );

void from_xml( ContractDetailsRequest*, const xmlNodePtr node );
void from_xml( HistRequest*, const xmlNodePtr node );
void from_xml( AccStatusRequest*, const xmlNodePtr node );
void from_xml( ExecutionsRequest*, const xmlNodePtr node );
void from_xml( OrdersRequest*, const xmlNodePtr node );
void from_xml( PlaceOrder*, const xmlNodePtr node );
void from_xml( MktDataRequest*, const xmlNodePtr node );


class TwsRow;
class RowHist;
class RowAcc;
class RowExecution;

void to_xml( xmlNodePtr parent, const char* name, const RowHist& );
void to_xml( xmlNodePtr parent, const RowAcc& );
void to_xml( xmlNodePtr parent, const RowExecution& );

void from_xml( RowHist*, const xmlNodePtr node );
void from_xml( RowAcc*, const xmlNodePtr node );
void from_xml( RowExecution*, const xmlNodePtr node );

void to_xml( xmlNodePtr parent, const TwsRow& );
void from_xml( TwsRow*, const xmlNodePtr node );



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
		long buf_len;
		char *buf;
		xmlDocPtr curDoc;
		xmlNodePtr curNode;
};




#define GET_ATTR_INT( _struct_, _attr_ ) \
	tmp = (char*) xmlGetProp( node, (xmlChar*) #_attr_ ); \
	if( tmp ) { \
		_struct_->_attr_ = atoi( tmp ); \
		free(tmp); \
	}

#define GET_ATTR_LONG( _struct_, _attr_ ) \
	tmp = (char*) xmlGetProp( node, (xmlChar*) #_attr_ ); \
	if( tmp ) { \
		_struct_->_attr_ = atol( tmp ); \
		free(tmp); \
	}

#define GET_ATTR_LONGLONG( _struct_, _attr_ ) \
	tmp = (char*) xmlGetProp( node, (xmlChar*) #_attr_ ); \
	if( tmp ) { \
		_struct_->_attr_ = atoll( tmp ); \
		free(tmp); \
	}

#define GET_ATTR_DOUBLE( _struct_, _attr_ ) \
	tmp = (char*) xmlGetProp( node, (xmlChar*) #_attr_ ); \
	if( tmp ) { \
		_struct_->_attr_ = atof( tmp ); \
		free(tmp); \
	}

#define GET_ATTR_BOOL( _struct_, _attr_ ) \
	tmp = (char*) xmlGetProp( node, (xmlChar*) #_attr_ ); \
	if( tmp ) { \
		_struct_->_attr_ = atoi( tmp ); \
		free(tmp); \
	}

#define GET_ATTR_STRING( _struct_, _attr_ ) \
	tmp = (char*) xmlGetProp( node, (xmlChar*) #_attr_ ); \
	if( tmp ) { \
		_struct_->_attr_ = std::string(tmp); \
		free(tmp); \
	}


#define ADD_ATTR_INT( _struct_, _attr_ ) \
	if( !TwsXml::skip_defaults || _struct_._attr_ != dflt._attr_ ) { \
		snprintf(tmp, sizeof(tmp), "%d",_struct_._attr_ ); \
		xmlNewProp ( ne, (xmlChar*) #_attr_, (xmlChar*) tmp ); \
	}

#define ADD_ATTR_LONG( _struct_, _attr_ ) \
	if( !TwsXml::skip_defaults || _struct_._attr_ != dflt._attr_ ) { \
		snprintf(tmp, sizeof(tmp), "%ld",_struct_._attr_ ); \
		xmlNewProp ( ne, (xmlChar*) #_attr_, (xmlChar*) tmp ); \
	}

#define ADD_ATTR_LONGLONG( _struct_, _attr_ ) \
	if( !TwsXml::skip_defaults || _struct_._attr_ != dflt._attr_ ) { \
		snprintf(tmp, sizeof(tmp), "%" PRIdMAX, (intmax_t)_struct_._attr_ ); \
		xmlNewProp ( ne, (xmlChar*) #_attr_, (xmlChar*) tmp ); \
	}

#define ADD_ATTR_DOUBLE( _struct_, _attr_ ) \
	if( !TwsXml::skip_defaults || _struct_._attr_ != dflt._attr_ ) { \
		snprintf(tmp, sizeof(tmp), "%.10g", _struct_._attr_ ); \
		xmlNewProp ( ne, (xmlChar*) #_attr_, (xmlChar*) tmp ); \
	}

#define ADD_ATTR_BOOL( _struct_, _attr_ ) \
	if( !TwsXml::skip_defaults || _struct_._attr_ != dflt._attr_ ) { \
		xmlNewProp ( ne, (xmlChar*) #_attr_, \
			(xmlChar*) (_struct_._attr_ ? "1" : "0") ); \
	}

#define ADD_ATTR_STRING( _struct_, _attr_ ) \
	if( !TwsXml::skip_defaults || _struct_._attr_ != dflt._attr_ ) { \
		xmlNewProp ( ne, (xmlChar*) #_attr_, \
			(xmlChar*) _struct_._attr_.c_str() ); \
	}


#define A_ADD_ATTR_INT( _ne_, _struct_, _attr_ ) \
	snprintf(tmp, sizeof(tmp), "%d",_struct_._attr_ ); \
	xmlNewProp ( _ne_, (xmlChar*) #_attr_, (xmlChar*) tmp )

#define A_ADD_ATTR_LONG( _ne_, _struct_, _attr_ ) \
	snprintf(tmp, sizeof(tmp), "%ld",_struct_._attr_ ); \
	xmlNewProp ( _ne_, (xmlChar*) #_attr_, (xmlChar*) tmp )

#define A_ADD_ATTR_DOUBLE( _ne_, _struct_, _attr_ ) \
	snprintf(tmp, sizeof(tmp), "%.10g", _struct_._attr_ ); \
	xmlNewProp ( _ne_, (xmlChar*) #_attr_, (xmlChar*) tmp )

#define A_ADD_ATTR_STRING( _ne_, _struct_, _attr_ ) \
	xmlNewProp ( _ne_, (xmlChar*) #_attr_, \
		(const xmlChar*) _struct_._attr_.c_str() )


#endif
