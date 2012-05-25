/*** tws_xml.h -- conversion between IB/API structs and xml
 *
 * Copyright (C) 2011-2012 Ruediger Meier
 *
 * Author:  Ruediger Meier <sweet_f_a@gmx.de>
 *
 * This file is part of twstools.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the author nor the names of any contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ***/

#ifndef TWS_XML_H
#define TWS_XML_H


namespace IB {
	class ComboLeg;
	class UnderComp;
	class Contract;
	class ContractDetails;
	class Execution;
	class ExecutionFilter;
	class TagValue;
	class Order;
	class OrderState;
}


typedef struct _xmlNode * xmlNodePtr;
typedef struct _xmlDoc * xmlDocPtr;


void conv_ib2xml( xmlNodePtr parent, const char* name, const IB::ComboLeg& c );
void conv_ib2xml( xmlNodePtr parent, const char* name, const IB::UnderComp& c );
void conv_ib2xml( xmlNodePtr parent, const char* name, const IB::Contract& c );
void conv_ib2xml( xmlNodePtr parent, const char* name,
	const IB::ContractDetails& c );
void conv_ib2xml( xmlNodePtr parent, const char* name, const IB::Execution& );
void conv_ib2xml( xmlNodePtr parent, const char* name,
	const IB::ExecutionFilter& );
void conv_ib2xml( xmlNodePtr parent, const char* name, const IB::TagValue& );
void conv_ib2xml( xmlNodePtr parent, const char* name, const IB::Order& );
void conv_ib2xml( xmlNodePtr parent, const char* name, const IB::OrderState& );

void conv_xml2ib( IB::ComboLeg* c, const xmlNodePtr node );
void conv_xml2ib( IB::UnderComp* c, const xmlNodePtr node );
void conv_xml2ib( IB::Contract* c, const xmlNodePtr node );
void conv_xml2ib( IB::ContractDetails* c, const xmlNodePtr node );
void conv_xml2ib( IB::Execution*, const xmlNodePtr node );
void conv_xml2ib( IB::ExecutionFilter*, const xmlNodePtr node );
void conv_xml2ib( IB::TagValue*, const xmlNodePtr node );
void conv_xml2ib( IB::Order*, const xmlNodePtr node );
void conv_xml2ib( IB::OrderState*, const xmlNodePtr node );


class ContractDetailsRequest;
class HistRequest;
class AccStatusRequest;
class ExecutionsRequest;
class OrdersRequest;
class PlaceOrder;
class CancelOrder;
class MktDataRequest;

void to_xml( xmlNodePtr parent, const ContractDetailsRequest& );
void to_xml( xmlNodePtr parent, const HistRequest& );
void to_xml( xmlNodePtr parent, const AccStatusRequest& );
void to_xml( xmlNodePtr parent, const ExecutionsRequest& );
void to_xml( xmlNodePtr parent, const OrdersRequest& );
void to_xml( xmlNodePtr parent, const PlaceOrder& );
void to_xml( xmlNodePtr parent, const CancelOrder& );
void to_xml( xmlNodePtr parent, const MktDataRequest& );

void from_xml( ContractDetailsRequest*, const xmlNodePtr node );
void from_xml( HistRequest*, const xmlNodePtr node );
void from_xml( AccStatusRequest*, const xmlNodePtr node );
void from_xml( ExecutionsRequest*, const xmlNodePtr node );
void from_xml( OrdersRequest*, const xmlNodePtr node );
void from_xml( PlaceOrder*, const xmlNodePtr node );
void from_xml( CancelOrder*, const xmlNodePtr node );
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
	_struct_->_attr_ = tmp ? atoi( tmp ) : dflt._attr_; \
	free(tmp)

#define GET_ATTR_LONG( _struct_, _attr_ ) \
	tmp = (char*) xmlGetProp( node, (xmlChar*) #_attr_ ); \
	_struct_->_attr_ = tmp ? atol( tmp ) : dflt._attr_; \
	free(tmp)

#define GET_ATTR_LONGLONG( _struct_, _attr_ ) \
	tmp = (char*) xmlGetProp( node, (xmlChar*) #_attr_ ); \
	_struct_->_attr_ = tmp ? atoll( tmp ) : dflt._attr_; \
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
		snprintf(tmp, sizeof(tmp), "%lld",_struct_._attr_ ); \
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
