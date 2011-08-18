/*** tws_meta.cpp -- helper structs for IB/API messages
 *
 * Copyright (C) 2010, 2011 Ruediger Meier
 *
 * Author:  Ruediger Meier <sweet_f_a@gmx.de>
 *
 * This file is part of atem.
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

#include "tws_meta.h"
#include "tws_xml.h"

#include "tws_util.h"
#include "debug.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <limits.h>
#include <string.h>
#include <time.h>







/// stupid static helper
std::string ibDate2ISO( const std::string &ibDate )
{
	struct tm tm;
	char buf[255];
	char *tmp;
	
	memset(&tm, 0, sizeof(struct tm));
	tmp = strptime( ibDate.c_str(), "%Y%m%d", &tm);
	if( tmp != NULL && *tmp == '\0' ) {
		strftime(buf, sizeof(buf), "%F", &tm);
		return buf;
	}
	
	memset(&tm, 0, sizeof(struct tm));
	tmp = strptime( ibDate.c_str(), "%Y%m%d%t%H:%M:%S", &tm);
	if(  tmp != NULL && *tmp == '\0' ) {
		strftime(buf, sizeof(buf), "%F %T", &tm);
		return buf;
	}
	
	return "";
}

typedef const char* string_pair[2];

const string_pair short_wts_[]= {
	{"TRADES", "T"},
	{"MIDPOINT", "M"},
	{"BID", "B"},
	{"ASK", "A"},
	{"BID_ASK", "BA"},
	{"HISTORICAL_VOLATILITY", "HV"},
	{"OPTION_IMPLIED_VOLATILITY", "OIV"},
	{"OPTION_VOLUME", "OV"},
	{NULL, "NNN"}
};

const string_pair short_bar_size_[]= {
	{"1 secs",   "s01"},
	{"5 secs",   "s05"},
	{"15 secs",  "s15"},
	{"30 secs",  "s30"},
	{"1 min",    "m01"},
	{"2 mins",   "m02"},
	{"3 mins",   "m03"},
	{"5 mins",   "m05"},
	{"15 mins",  "m15"},
	{"30 mins",  "m30"},
	{"1 hour",   "h01"},
	{"4 hour",   "h04"},
	{"1 day",    "eod"},
	{"1 week",   "w01"},
	{"1 month",  "x01"},
	{"3 months", "x03"},
	{"1 year",   "y01"},
	{NULL, "00N"}
};

const char* short_string( const string_pair* pairs, const char* s_short )
{
	const string_pair *i = pairs;
	while( (*i)[0] != 0 ) {
		if( strcmp((*i)[0], s_short)==0 ) {
			return (*i)[1];
		}
		i++;
	}
	return (*i)[1];
}

const char* short_wts( const char* wts )
{
	return short_string( short_wts_, wts );
}

const char* short_bar_size( const char* bar_size )
{
	return short_string( short_bar_size_, bar_size );
}





ContractDetailsRequest * ContractDetailsRequest::fromXml( xmlNodePtr xn )
{
	ContractDetailsRequest *cdr = new ContractDetailsRequest();
	
	for( xmlNodePtr p = xn->children; p!= NULL; p=p->next) {
		if( p->type == XML_ELEMENT_NODE
			&& strcmp((char*)p->name, "reqContract") == 0 )  {
			IB::Contract c;
			conv_xml2ib( &c, p);
			cdr->initialize(c);
		}
	}
	
	return cdr;
}

const IB::Contract& ContractDetailsRequest::ibContract() const
{
	return _ibContract;
}

bool ContractDetailsRequest::initialize( const IB::Contract& c )
{
	_ibContract = c;
	return true;
}








bool HistRequest::initialize( const IB::Contract& c, const std::string &e,
	const std::string &d, const std::string &b,
	const std::string &w, int u, int f )
{
	_ibContract = c;
	_endDateTime = e;
	_durationStr = d;
	_barSizeSetting = b;
	_whatToShow = w;
	_useRTH = u;
	_formatDate = f;
	return true;
}


std::string HistRequest::toString() const
{
	char buf_c[512];
	char buf_a[1024];
	snprintf( buf_c, sizeof(buf_c), "%s\t%s\t%s\t%s\t%s\t%g\t%s",
		_ibContract.symbol.c_str(),
		_ibContract.secType.c_str(),
		_ibContract.exchange.c_str(),
		_ibContract.currency.c_str(),
		_ibContract.expiry.c_str(),
		_ibContract.strike,
		_ibContract.right.c_str() );
	
	snprintf( buf_a, sizeof(buf_a), "%s\t%s\t%s\t%s\t%d\t%d\t%s",
		_endDateTime.c_str(),
		_durationStr.c_str(),
		_barSizeSetting.c_str(),
		_whatToShow.c_str(),
		_useRTH,
		_formatDate,
		buf_c );
	
	return std::string(buf_a);
}


void HistRequest::clear()
{
	_ibContract = IB::Contract();
	_whatToShow.clear();
}



#define GET_ATTR_STRING( _struct_, _name_, _attr_ ) \
	tmp = (char*) xmlGetProp( node, (const xmlChar*) _name_ ); \
	_struct_->_attr_ = tmp ? std::string(tmp) \
		: dflt._attr_; \
	free(tmp)

#define GET_ATTR_INT( _struct_, _name_, _attr_ ) \
	tmp = (char*) xmlGetProp( node, (const xmlChar*) _name_ ); \
	_struct_->_attr_ = tmp ? atoi( tmp ) : dflt._attr_; \
	free(tmp)

#define GET_ATTR_DOUBLE( _struct_, _name_, _attr_ ) \
	tmp = (char*) xmlGetProp( node, (const xmlChar*) _name_ ); \
	_struct_->_attr_ = tmp ? atof( tmp ) : dflt._attr_; \
	free(tmp)
#define GET_ATTR_BOOL( _struct_, _name_, _attr_ ) \
	tmp = (char*) xmlGetProp( node, (const xmlChar*) _name_ ); \
	_struct_->_attr_ = tmp ? atoi( tmp ) : dflt._attr_; \
	free(tmp)


HistRequest * HistRequest::fromXml( xmlNodePtr node )
{
	char* tmp;
	static const HistRequest dflt;
	
	HistRequest *hR = new HistRequest();
	
	for( xmlNodePtr p = node->children; p!= NULL; p=p->next) {
		if( p->type == XML_ELEMENT_NODE
			&& strcmp((char*)p->name, "reqContract") == 0 )  {
			conv_xml2ib( &hR->_ibContract, p);
		}
	}
	
	GET_ATTR_STRING( hR, "endDateTime", _endDateTime );
	GET_ATTR_STRING( hR, "durationStr", _durationStr );
	GET_ATTR_STRING( hR, "barSizeSetting", _barSizeSetting );
	GET_ATTR_STRING( hR, "whatToShow", _whatToShow );
	GET_ATTR_INT( hR, "useRTH", _useRTH );
	GET_ATTR_INT( hR, "formatDate", _formatDate );
	
	return hR;
}








GenericRequest::GenericRequest() :
	_reqType(NONE),
	_reqId(0),
	_ctime(0)
{
}


GenericRequest::ReqType GenericRequest::reqType() const
{
	return _reqType;
}


int GenericRequest::reqId() const
{
	return _reqId;
}


int GenericRequest::age() const
{
	return (nowInMsecs() - _ctime);
}


void GenericRequest::nextRequest( ReqType t )
{
	_reqType = t;
	_reqId++;
	_ctime = nowInMsecs();
}


void GenericRequest::close()
{
	_reqType = NONE;
}










HistTodo::HistTodo() :
	doneRequests(*(new std::list<HistRequest*>())),
	leftRequests(*(new std::list<HistRequest*>())),
	errorRequests(*(new std::list<HistRequest*>())),
	checkedOutRequest(NULL)
{
}


HistTodo::~HistTodo()
{
	std::list<HistRequest*>::const_iterator it;
	for( it = doneRequests.begin(); it != doneRequests.end(); it++ ) {
		delete *it;
	}
	delete &doneRequests;
	for( it = leftRequests.begin(); it != leftRequests.end(); it++ ) {
		delete *it;
	}
	delete &leftRequests;
	for( it = errorRequests.begin(); it != errorRequests.end(); it++ ) {
		delete *it;
	}
	delete &errorRequests;
	if( checkedOutRequest != NULL ) {
		delete checkedOutRequest;
	}
}


void HistTodo::dumpLeft( FILE *stream ) const
{
	std::list<HistRequest*>::const_iterator it = leftRequests.begin();
	while( it != leftRequests.end() ) {
		fprintf( stream, "[%p]\t%s\n",
		         *it,
		         (*it)->toString().c_str() );
		it++;
	}
}


int HistTodo::countDone() const
{
	return doneRequests.size();
}


int HistTodo::countLeft() const
{
	return leftRequests.size();
}


void HistTodo::checkout()
{
	assert( checkedOutRequest == NULL );
	checkedOutRequest = leftRequests.front();
	leftRequests.pop_front();
}


int HistTodo::checkoutOpt( PacingGod *pG, const DataFarmStates *dfs )
{
	assert( checkedOutRequest == NULL );
	
	std::map<std::string, HistRequest*> hashByFarm;
	std::map<std::string, int> countByFarm;
	for( std::list<HistRequest*>::const_iterator it = leftRequests.begin();
		it != leftRequests.end(); it++ ) {
		std::string farm = dfs->getHmdsFarm((*it)->ibContract());
		if( hashByFarm.find(farm) == hashByFarm.end() ) {
			hashByFarm[farm] = *it;
			countByFarm[farm] = 1;
		} else {
			countByFarm[farm]++;
		}
	}
	
	HistRequest *todo_hR = leftRequests.front();
	int countTodo = 0;
	for( std::map<std::string, HistRequest*>::const_iterator
		    it = hashByFarm.begin(); it != hashByFarm.end(); it++ ) {
		const std::string &farm = it->first;
		HistRequest *tmp_hR = it->second;
		const IB::Contract& c = tmp_hR->ibContract();
		if( pG->countLeft( c ) > 0 ) {
			if( farm.empty() ) {
				// 1. the unknown ones to learn farm quickly
				todo_hR = tmp_hR;
				break;
			} else if( countTodo < countByFarm[farm] ) {
				// 2. get from them biggest list
				todo_hR = tmp_hR;
				countTodo = countByFarm[farm];
			}
		}
	}
	
	std::list<HistRequest*>::iterator it = leftRequests.begin();
	while( it != leftRequests.end() ) {
		if( *it == todo_hR ) {
			break;
		}
		it++;
	}
	assert( it != leftRequests.end() );
	int wait = pG->goodTime( todo_hR->ibContract() );
	if( wait <= 0 ) {
		leftRequests.erase( it );
		checkedOutRequest = todo_hR;
	}
	
	return wait;
}


void HistTodo::cancelForRepeat( int priority )
{
	assert( checkedOutRequest != NULL );
	if( priority <= 0 ) {
		leftRequests.push_front(checkedOutRequest);
	} else if( priority <=1 ) {
		leftRequests.push_back(checkedOutRequest);
	} else {
		errorRequests.push_back(checkedOutRequest);
	}
	checkedOutRequest = NULL;
}


const HistRequest& HistTodo::current() const
{
	assert( checkedOutRequest != NULL );
	return *checkedOutRequest;
}


void HistTodo::tellDone()
{
	assert( checkedOutRequest != NULL );
	doneRequests.push_back(checkedOutRequest);
	checkedOutRequest = NULL;
}


void HistTodo::add( const HistRequest& hR )
{
	HistRequest *p = new HistRequest(hR);
	leftRequests.push_back(p);
}








ContractDetailsTodo::ContractDetailsTodo() :
	curIndex(-1),
	contractDetailsRequests(*(new std::vector<ContractDetailsRequest>()))
{
}

ContractDetailsTodo::~ContractDetailsTodo()
{
	delete &contractDetailsRequests;
}

int ContractDetailsTodo::countLeft() const
{
	return contractDetailsRequests.size() - curIndex - 1;
}

void ContractDetailsTodo::checkout()
{
	assert( countLeft() > 0 );
	curIndex++;
}

const ContractDetailsRequest& ContractDetailsTodo::current() const
{
	assert( curIndex >= 0 && curIndex < (int)contractDetailsRequests.size() );
	return contractDetailsRequests.at(curIndex);
}

void ContractDetailsTodo::add( const ContractDetailsRequest& cdr )
{
	contractDetailsRequests.push_back(cdr);
}








WorkTodo::WorkTodo() :
	_contractDetailsTodo( new ContractDetailsTodo() ),
	_histTodo( new HistTodo() )
{
}


WorkTodo::~WorkTodo()
{
	if( _histTodo != NULL ) {
		delete _histTodo;
	}
	if( _contractDetailsTodo != NULL ) {
		delete _contractDetailsTodo;
	}
}


GenericRequest::ReqType WorkTodo::nextReqType() const
{
	if( acc_status_todo ) {
		acc_status_todo = false;
		return GenericRequest::ACC_STATUS_REQUEST;
	} else if( executions_todo ) {
		executions_todo = false;
		return GenericRequest::EXECUTIONS_REQUEST;
	} else if( orders_todo ) {
		orders_todo = false;
		return GenericRequest::ORDERS_REQUEST;
	} else if( _contractDetailsTodo->countLeft() > 0 ) {
		return GenericRequest::CONTRACT_DETAILS_REQUEST;
	} else if( _histTodo->countLeft() > 0 ) {
		return GenericRequest::HIST_REQUEST;
	} else {
		return GenericRequest::NONE;
	}
}

ContractDetailsTodo* WorkTodo::contractDetailsTodo() const
{
	return _contractDetailsTodo;
}

const ContractDetailsTodo& WorkTodo::getContractDetailsTodo() const
{
	return *_contractDetailsTodo;
}

HistTodo* WorkTodo::histTodo() const
{
	return _histTodo;
}

const HistTodo& WorkTodo::getHistTodo() const
{
	return *_histTodo;
}


void WorkTodo::addSimpleRequest( GenericRequest::ReqType reqType )
{
	switch( reqType ) {
	case GenericRequest::ACC_STATUS_REQUEST:
		acc_status_todo = true;
		break;
	case GenericRequest::EXECUTIONS_REQUEST:
		executions_todo = true;
		break;
	case GenericRequest::ORDERS_REQUEST:
		orders_todo = true;
		break;
	default:
		assert(false);
	}
}


int WorkTodo::read_file( const std::string & fileName )
{
	int retVal = -1;
	
	TwsXml file;
	if( ! file.openFile(fileName.c_str()) ) {
		return retVal;
	}
	retVal = 0;
	xmlNodePtr xn;
	while( (xn = file.nextXmlNode()) != NULL ) {
		assert( xn->type == XML_ELEMENT_NODE  );
		if( strcmp((char*)xn->name, "request") == 0 ) {
			char* tmp = (char*) xmlGetProp( xn, (const xmlChar*) "type" );
			if( tmp == NULL ) {
				fprintf(stderr, "Warning, no request type specified.\n");
			} else if( strcmp( tmp, "contract_details") == 0 ) {
				PacketContractDetails *pcd = PacketContractDetails::fromXml(xn);
				_contractDetailsTodo->add(pcd->getRequest());
				retVal++;
			} else if ( strcmp( tmp, "historical_data") == 0 ) {
				PacketHistData *phd = PacketHistData::fromXml(xn);
				_histTodo->add( phd->getRequest() );
				retVal++;
			} else {
				fprintf(stderr, "Warning, unknown request type '%s' ignored.\n",
					tmp );
			}
			free(tmp);
		} else {
			fprintf(stderr, "Warning, unknown request tag '%s' ignored.\n",
				xn->name );
		}
	}
	
	return retVal;
}








Packet::Packet() :
	mode(CLEAN)
{
}

Packet::~Packet()
{
}

bool Packet::empty() const
{
	return (mode == CLEAN);
}

bool Packet::finished() const
{
	return (mode == CLOSED);
}








PacketContractDetails::PacketContractDetails() :
	cdList(new std::vector<IB::ContractDetails>())
{
	reqId = -1;
	request = NULL;
}

PacketContractDetails::~PacketContractDetails()
{
	delete cdList;
	if( request != NULL ) {
		delete request;
	}
}

PacketContractDetails * PacketContractDetails::fromXml( xmlNodePtr root )
{
	PacketContractDetails *pcd = new PacketContractDetails();
	
	for( xmlNodePtr p = root->children; p!= NULL; p=p->next) {
		if( p->type == XML_ELEMENT_NODE ) {
			if( strcmp((char*)p->name, "query") == 0 ) {
				pcd->request = ContractDetailsRequest::fromXml(p);
			}
			if( strcmp((char*)p->name, "response") == 0 ) {
				for( xmlNodePtr q = p->children; q!= NULL; q=q->next) {
					if( q->type == XML_ELEMENT_NODE
						&& strcmp((char*)q->name, "ContractDetails") == 0 )  {
						IB::ContractDetails cd;
						conv_xml2ib(&cd, q);
						pcd->cdList->push_back(cd);
					}
				}
			}
		}
	}
	return pcd;
}

const ContractDetailsRequest& PacketContractDetails::getRequest() const
{
	return *request;
}

void PacketContractDetails::record( int reqId,
	const ContractDetailsRequest& cdr )
{
	assert( empty() && this->reqId == -1 && request == NULL
		&& cdList->empty() );
	this->reqId = reqId;
	this->request = new ContractDetailsRequest( cdr );
	mode = RECORD;
}

void PacketContractDetails::setFinished()
{
	assert( mode == RECORD );
	mode = CLOSED;
}


void PacketContractDetails::clear()
{
	mode = CLEAN;
	reqId = -1;
	cdList->clear();
	if( request != NULL ) {
		delete request;
		request = NULL;
	}
}


void PacketContractDetails::append( int reqId, const IB::ContractDetails& c )
{
	if( cdList->empty() ) {
		this->reqId = reqId;
	}
	assert( this->reqId == reqId );
	assert( mode == RECORD );
	
	cdList->push_back(c);
}


void PacketContractDetails::dumpXml()
{
	xmlNodePtr root = TwsXml::newDocRoot();
	xmlNodePtr npcd = xmlNewChild( root, NULL,
		(const xmlChar*)"request", NULL );
	xmlNewProp( npcd, (const xmlChar*)"type",
		(const xmlChar*)"contract_details" );
	
	xmlNodePtr nqry = xmlNewChild( npcd, NULL, (xmlChar*)"query", NULL);
	conv_ib2xml( nqry, "reqContract", request->ibContract() );
	
	xmlNodePtr nrsp = xmlNewChild( npcd, NULL, (xmlChar*)"response", NULL);
	for( size_t i=0; i<cdList->size(); i++ ) {
		conv_ib2xml( nrsp, "ContractDetails", (*cdList)[i] );
	}
	
	TwsXml::dumpAndFree( root );
}








void PacketHistData::Row::clear()
{
	date = "";
	open = 0.0;
	high = 0.0;
	low = 0.0;
	close = 0.0;
	volume = 0;
	count = 0;
	WAP = 0.0;
	hasGaps = false;
}

PacketHistData::Row * PacketHistData::Row::fromXml( xmlNodePtr node )
{
	char* tmp;
	static const Row dflt = {"", -1.0, -1.0, -1.0, -1.0, -1, -1, -1.0, 0 };
	Row *row = new Row();
	
	GET_ATTR_STRING( row, "date", date );
	GET_ATTR_DOUBLE( row, "open", open );
	GET_ATTR_DOUBLE( row, "high", high );
	GET_ATTR_DOUBLE( row, "low", low );
	GET_ATTR_DOUBLE( row, "close", close );
	GET_ATTR_INT( row, "volume", volume );
	GET_ATTR_INT( row, "count", count );
	GET_ATTR_DOUBLE( row, "WAP", WAP );
	GET_ATTR_BOOL( row, "hasGaps", hasGaps );
	
	return row;
}


PacketHistData::PacketHistData() :
		rows(*(new std::vector<Row>()))
{
	error = ERR_NONE;
	reqId = -1;
	request = NULL;
}

PacketHistData::~PacketHistData()
{
	delete &rows;
	if( request != NULL ) {
		delete request;
	}
}

PacketHistData * PacketHistData::fromXml( xmlNodePtr root )
{
	PacketHistData *phd = new PacketHistData();
	
	for( xmlNodePtr p = root->children; p!= NULL; p=p->next) {
		if( p->type == XML_ELEMENT_NODE ) {
			if( strcmp((char*)p->name, "query") == 0 ) {
				phd->request = HistRequest::fromXml(p);
			}
			if( strcmp((char*)p->name, "response") == 0 ) {
				for( xmlNodePtr q = p->children; q!= NULL; q=q->next) {
					if( q->type != XML_ELEMENT_NODE ) {
						continue;
					}
					if( strcmp((char*)q->name, "row") == 0 ) {
						Row *row = Row::fromXml( q );
						phd->rows.push_back(*row);
						delete row;
					} else if( strcmp((char*)q->name, "fin") == 0 ) {
						Row *fin = Row::fromXml( q );
						phd->finishRow = *fin;
						phd->mode = CLOSED;
						delete fin;
					}
				}
			}
		}
	}
	
	return phd;
}


#define ADD_ATTR_STRING( _ne_, _struct_, _attr_ ) \
	if( !TwsXml::skip_defaults || _struct_._attr_ != dflt._attr_ ) { \
		xmlNewProp ( _ne_, (xmlChar*) #_attr_, \
			(xmlChar*) _struct_._attr_.c_str() ); \
	}

#define ADD_ATTR_INT( _ne_, _struct_, _attr_ ) \
	if( !TwsXml::skip_defaults || _struct_._attr_ != dflt._attr_ ) { \
		snprintf(tmp, sizeof(tmp), "%d",_struct_._attr_ ); \
		xmlNewProp ( _ne_, (xmlChar*) #_attr_, (xmlChar*) tmp ); \
	}

#define ADD_ATTR_DOUBLE( _ne_, _struct_, _attr_ ) \
	if( !TwsXml::skip_defaults || _struct_._attr_ != dflt._attr_ ) { \
		snprintf(tmp, sizeof(tmp), "%.10g", _struct_._attr_ ); \
		xmlNewProp ( _ne_, (xmlChar*) #_attr_, (xmlChar*) tmp ); \
	}

#define ADD_ATTR_BOOL( _ne_, _struct_, _attr_ ) \
	if( !TwsXml::skip_defaults || _struct_._attr_ != dflt._attr_ ) { \
		xmlNewProp ( _ne_, (xmlChar*) #_attr_, \
			(xmlChar*) (_struct_._attr_ ? "1" : "0") ); \
	}

void PacketHistData::dumpXml()
{
	char tmp[128];
	
	xmlNodePtr root = TwsXml::newDocRoot();
	xmlNodePtr nphd = xmlNewChild( root, NULL,
		(const xmlChar*)"request", NULL );
	xmlNewProp( nphd, (const xmlChar*)"type",
		(const xmlChar*)"historical_data" );
	
	{
		struct s_bla {
			const std::string endDateTime;
			const std::string durationStr;
			const std::string barSizeSetting;
			const std::string whatToShow;
			int useRTH;
			int formatDate;
		};
		static const s_bla dflt = {"", "", "", "", 0, 0 };
		const IB::Contract &c = request->ibContract();
		s_bla bla = { request->endDateTime(), request->durationStr(),
			request->barSizeSetting(), request->whatToShow(), request->useRTH(),
			request->formatDate() };
		
		xmlNodePtr nqry = xmlNewChild( nphd, NULL, (xmlChar*)"query", NULL);
		conv_ib2xml( nqry, "reqContract", c );
		ADD_ATTR_STRING( nqry, bla, endDateTime );
		ADD_ATTR_STRING( nqry, bla, durationStr );
		ADD_ATTR_STRING( nqry, bla, barSizeSetting );
		ADD_ATTR_STRING( nqry, bla, whatToShow );
		ADD_ATTR_INT( nqry, bla, useRTH );
		ADD_ATTR_INT( nqry, bla, formatDate );
	}
	
	if( mode == CLOSED ) {
	xmlNodePtr nrsp = xmlNewChild( nphd, NULL, (xmlChar*)"response", NULL);
	{
		static const Row dflt = {"", -1.0, -1.0, -1.0, -1.0, -1, -1, -1.0, 0 };
		for( size_t i=0; i<rows.size(); i++ ) {
			xmlNodePtr nrow = xmlNewChild( nrsp, NULL, (xmlChar*)"row", NULL);
			ADD_ATTR_STRING( nrow, rows[i], date );
			ADD_ATTR_DOUBLE( nrow, rows[i], open );
			ADD_ATTR_DOUBLE( nrow, rows[i], high );
			ADD_ATTR_DOUBLE( nrow, rows[i], low );
			ADD_ATTR_DOUBLE( nrow, rows[i], close );
			ADD_ATTR_INT( nrow, rows[i], volume );
			ADD_ATTR_INT( nrow, rows[i], count );
			ADD_ATTR_DOUBLE( nrow, rows[i], WAP );
			ADD_ATTR_BOOL( nrow, rows[i], hasGaps );
		}
		xmlNodePtr nrow = xmlNewChild( nrsp, NULL, (xmlChar*)"fin", NULL);
		ADD_ATTR_STRING( nrow, finishRow, date );
		ADD_ATTR_DOUBLE( nrow, finishRow, open );
		ADD_ATTR_DOUBLE( nrow, finishRow, high );
		ADD_ATTR_DOUBLE( nrow, finishRow, low );
		ADD_ATTR_DOUBLE( nrow, finishRow, close );
		ADD_ATTR_INT( nrow, finishRow, volume );
		ADD_ATTR_INT( nrow, finishRow, count );
		ADD_ATTR_DOUBLE( nrow, finishRow, WAP );
		ADD_ATTR_BOOL( nrow, finishRow, hasGaps );
	}
	}
	TwsXml::dumpAndFree( root );
}

const HistRequest& PacketHistData::getRequest() const
{
	return *request;
}


PacketHistData::Error PacketHistData::getError() const
{
	return error;
}


void PacketHistData::clear()
{
	mode = CLEAN;
	error = ERR_NONE;
	reqId = -1;
	if( request != NULL ) {
		delete request;
		request = NULL;
	}
	rows.clear();
	finishRow.clear();
}


void PacketHistData::record( int reqId, const HistRequest& hR )
{
	assert( mode == CLEAN && error == ERR_NONE && request == NULL );
	mode = RECORD;
	this->reqId = reqId;
	this->request = new HistRequest( hR );
}


void PacketHistData::append( int reqId, const std::string &date,
			double open, double high, double low, double close,
			int volume, int count, double WAP, bool hasGaps )
{
	assert( mode == RECORD && error == ERR_NONE );
	assert( this->reqId == reqId );
	
	Row row = { date, open, high, low, close,
		volume, count, WAP, hasGaps };
	
	if( strncmp(date.c_str(), "finished", 8) == 0) {
		mode = CLOSED;
		finishRow = row;
	} else {
		rows.push_back( row );
	}
}


void PacketHistData::closeError( Error e )
{
	assert( mode == RECORD);
	mode = CLOSED;
	error = e;
}


void PacketHistData::dump( bool printFormatDates )
{
	assert( mode == CLOSED && error == ERR_NONE );
	const IB::Contract &c = request->ibContract();
	const char *wts = short_wts( request->whatToShow().c_str() );
	const char *bss = short_bar_size( request->barSizeSetting().c_str());
	
	for( std::vector<Row>::const_iterator it = rows.begin();
		it != rows.end(); it++ ) {
		std::string expiry = c.expiry;
		std::string dateTime = it->date;
		if( printFormatDates ) {
			if( expiry.empty() ) {
				expiry = "0000-00-00";
			} else {
				expiry = ibDate2ISO( c.expiry );
			}
			dateTime = ibDate2ISO(it->date);
			assert( !expiry.empty() && !dateTime.empty() ); //TODO
		}
		
		char buf_c[512];
		snprintf( buf_c, sizeof(buf_c), "%s\t%s\t%s\t%s\t%s\t%g\t%s",
			c.symbol.c_str(),
			c.secType.c_str(),
			c.exchange.c_str(),
			c.currency.c_str(),
			expiry.c_str(),
			c.strike,
			c.right.c_str() );
		
		printf("%s\t%s\t%s\t%s\t%f\t%f\t%f\t%f\t%d\t%d\t%f\t%d\n",
		       wts,
		       bss,
		       buf_c,
		       dateTime.c_str(),
		       it->open, it->high, it->low, it->close,
		       it->volume, it->count, it->WAP, it->hasGaps);
		fflush(stdout);
	}
}








PacketAccStatus::PacketAccStatus() :
	list( new std::vector<RowAcc*>() )
{
}

PacketAccStatus::~PacketAccStatus()
{
	del_list_elements();
	delete list;
}

void PacketAccStatus::clear()
{
	assert( finished() );
	mode = CLEAN;
	del_list_elements();
	list->clear();
}

void PacketAccStatus::del_list_elements()
{
	std::vector<RowAcc*>::const_iterator it;
	for( it = list->begin(); it < list->end(); it++ ) {
		switch( (*it)->type ) {
		case RowAcc::t_AccVal:
			delete (RowAccVal*) (*it)->data;
			break;
		case RowAcc::t_Prtfl:
			delete (RowPrtfl*) (*it)->data;
			break;
		case RowAcc::t_stamp:
		case RowAcc::t_end:
			delete (std::string*) (*it)->data;
			break;
		}
		delete (*it);
	}
}

void PacketAccStatus::record( const std::string &acctCode )
{
	assert( empty() );
	mode = RECORD;
	this->accountName = acctCode;
}

void PacketAccStatus::append( const RowAccVal& row )
{
	RowAcc *arow = new RowAcc();
	arow->type = RowAcc::t_AccVal;
	arow->data = new RowAccVal(row);
	list->push_back( arow );
}

void PacketAccStatus::append( const RowPrtfl& row )
{
	RowAcc *arow = new RowAcc();
	arow->type = RowAcc::t_Prtfl;
	arow->data = new RowPrtfl(row);
	list->push_back( arow );
}

void PacketAccStatus::appendUpdateAccountTime( const std::string& timeStamp )
{
	RowAcc *arow = new RowAcc();
	arow->type =RowAcc::t_stamp;
	arow->data = new std::string(timeStamp);
	list->push_back( arow );
}

void PacketAccStatus::appendAccountDownloadEnd( const std::string& accountName )
{
	RowAcc *arow = new RowAcc();
	arow->type =RowAcc::t_end;
	arow->data = new std::string(accountName);
	list->push_back( arow );
	
	mode = CLOSED;
}


#define A_ADD_ATTR_STRING( _ne_, _struct_, _attr_ ) \
	xmlNewProp ( _ne_, (xmlChar*) #_attr_, \
		(const xmlChar*) _struct_._attr_.c_str() )

#define A_ADD_ATTR_INT( _ne_, _struct_, _attr_ ) \
	snprintf(tmp, sizeof(tmp), "%d",_struct_._attr_ ); \
	xmlNewProp ( _ne_, (xmlChar*) #_attr_, (xmlChar*) tmp )

#define A_ADD_ATTR_LONG( _ne_, _struct_, _attr_ ) \
	snprintf(tmp, sizeof(tmp), "%ld",_struct_._attr_ ); \
	xmlNewProp ( _ne_, (xmlChar*) #_attr_, (xmlChar*) tmp )

#define A_ADD_ATTR_DOUBLE( _ne_, _struct_, _attr_ ) \
	snprintf(tmp, sizeof(tmp), "%.10g", _struct_._attr_ ); \
	xmlNewProp ( _ne_, (xmlChar*) #_attr_, (xmlChar*) tmp )


static void conv2xml( xmlNodePtr parent, const RowAcc *row )
{
		char tmp[128];
		switch( row->type ) {
		case RowAcc::t_AccVal:
			{
				const RowAccVal &d = *(RowAccVal*)row->data;
				xmlNodePtr nrow = xmlNewChild( parent,
					NULL, (const xmlChar*)"AccVal", NULL);
				A_ADD_ATTR_STRING( nrow, d, key );
				A_ADD_ATTR_STRING( nrow, d, val );
				A_ADD_ATTR_STRING( nrow, d, currency );
				A_ADD_ATTR_STRING( nrow, d, accountName );
			}
			break;
		case RowAcc::t_Prtfl:
			{
				const RowPrtfl &d = *(RowPrtfl*)row->data;
				xmlNodePtr nrow = xmlNewChild( parent,
					NULL, (const xmlChar*)"Prtfl", NULL);
				conv_ib2xml( nrow, "contract", d.contract );
				A_ADD_ATTR_INT( nrow, d, position );
				A_ADD_ATTR_DOUBLE( nrow, d, marketPrice );
				A_ADD_ATTR_DOUBLE( nrow, d, marketValue );
				A_ADD_ATTR_DOUBLE( nrow, d, averageCost );
				A_ADD_ATTR_DOUBLE( nrow, d, unrealizedPNL );
				A_ADD_ATTR_DOUBLE( nrow, d, realizedPNL );
				A_ADD_ATTR_STRING( nrow, d, accountName );
			}
			break;
		case RowAcc::t_stamp:
			{
				const std::string &d = *(std::string*)row->data;
				xmlNodePtr nrow = xmlNewChild( parent,
					NULL, (const xmlChar*)"stamp", NULL);
				xmlNewProp ( nrow, (xmlChar*) "timeStamp",
					(const xmlChar*) d.c_str() );
			}
			break;
		case RowAcc::t_end:
			{
				const std::string &d = *(std::string*)row->data;
				xmlNodePtr nrow = xmlNewChild( parent,
					NULL, (const xmlChar*)"end", NULL);
				xmlNewProp ( nrow, (xmlChar*) "accountName",
					(const xmlChar*) d.c_str() );
			}
			break;
		}
}

void PacketAccStatus::dumpXml()
{
	xmlNodePtr root = TwsXml::newDocRoot();
	xmlNodePtr npcd = xmlNewChild( root, NULL,
		(const xmlChar*)"request", NULL );
	xmlNewProp( npcd, (const xmlChar*)"type",
		(const xmlChar*)"account" );
	
	xmlNodePtr nqry = xmlNewChild( npcd, NULL, (xmlChar*)"query", NULL);
	xmlNewProp ( nqry, (xmlChar*) "accountName",
		(const xmlChar*) accountName.c_str() );
	
	xmlNodePtr nrsp = xmlNewChild( npcd, NULL, (xmlChar*)"response", NULL);
	std::vector<RowAcc*>::const_iterator it;
	for( it = list->begin(); it < list->end(); it++ ) {
		conv2xml( nrsp, (*it) );
	}
	
	TwsXml::dumpAndFree( root );
}








PacketExecutions::PacketExecutions() :
	reqId(-1),
	executionFilter(NULL),
	list( new std::vector<RowExecution*>() )
{
}

PacketExecutions::~PacketExecutions()
{
	del_list_elements();
	delete list;
	if( executionFilter != NULL ) {
		delete executionFilter;
	}
}

void PacketExecutions::clear()
{
	assert( finished() );
	mode = CLEAN;
	del_list_elements();
	list->clear();
	if( executionFilter != NULL ) {
		delete executionFilter;
	}
	executionFilter = NULL;
}

void PacketExecutions::del_list_elements()
{
	std::vector<RowExecution*>::const_iterator it;
	for( it = list->begin(); it < list->end(); it++ ) {
		delete (*it);
	}
}

void PacketExecutions::record(  const int reqId, const IB::ExecutionFilter &eF )
{
	assert( empty() );
	mode = RECORD;
	this->reqId = reqId;
	this->executionFilter = new IB::ExecutionFilter( eF );
}

void PacketExecutions::append( int reqId,
	const IB::Contract& c, const IB::Execution& e)
{
	RowExecution *arow = new RowExecution();
	arow->contract = c;
	arow->execution = e;
	list->push_back( arow );
}

void PacketExecutions::appendExecutionsEnd( int reqId )
{
	mode = CLOSED;
}

static void conv2xml( xmlNodePtr parent, const RowExecution *row )
{
		xmlNodePtr nrow = xmlNewChild( parent,
			NULL, (const xmlChar*)"ExecDetails", NULL);
		conv_ib2xml( nrow, "contract", row->contract );
		conv_ib2xml( nrow, "execution", row->execution );
}

void PacketExecutions::dumpXml()
{
	xmlNodePtr root = TwsXml::newDocRoot();
	xmlNodePtr npcd = xmlNewChild( root, NULL,
		(const xmlChar*)"request", NULL );
	xmlNewProp( npcd, (const xmlChar*)"type",
		(const xmlChar*)"executions" );
	
	xmlNodePtr nqry = xmlNewChild( npcd, NULL, (xmlChar*)"query", NULL);
	conv_ib2xml( nqry, "executionFilter", *executionFilter );
	
	xmlNodePtr nrsp = xmlNewChild( npcd, NULL, (xmlChar*)"response", NULL);
	std::vector<RowExecution*>::const_iterator it;
	for( it = list->begin(); it < list->end(); it++ ) {
		conv2xml( nrsp, (*it) );
	}
	
	TwsXml::dumpAndFree( root );
}







PacketOrders::PacketOrders() :
	list( new std::vector<RowOrd*>() )
{
}

PacketOrders::~PacketOrders()
{
	del_list_elements();
	delete list;
}

void PacketOrders::clear()
{
	assert( finished() );
	mode = CLEAN;
	del_list_elements();
	list->clear();
}

void PacketOrders::del_list_elements()
{
	std::vector<RowOrd*>::const_iterator it;
	for( it = list->begin(); it < list->end(); it++ ) {
		switch( (*it)->type ) {
		case RowOrd::t_OrderStatus:
			delete (RowOrderStatus*) (*it)->data;
			break;
		case RowOrd::t_OpenOrder:
			delete (RowOpenOrder*) (*it)->data;
			break;
		}
		delete (*it);
	}
}

void PacketOrders::record()
{
	assert( empty() );
	mode = RECORD;
}

void PacketOrders::append( const RowOrderStatus& row )
{
	RowOrd *arow = new RowOrd();
	arow->type = RowOrd::t_OrderStatus;
	arow->data = new RowOrderStatus(row);
	list->push_back( arow );
}

void PacketOrders::append( const RowOpenOrder& row )
{
	RowOrd *arow = new RowOrd();
	arow->type = RowOrd::t_OpenOrder;
	arow->data = new RowOpenOrder(row);
	list->push_back( arow );
}

void PacketOrders::appendOpenOrderEnd()
{
	mode = CLOSED;
}


static void conv2xml( xmlNodePtr parent, const RowOrd *row )
{
	char tmp[128];
	switch( row->type ) {
	case RowOrd::t_OrderStatus:
		{
			const RowOrderStatus &d = *(RowOrderStatus*)row->data;
			xmlNodePtr nrow = xmlNewChild( parent,
				NULL, (const xmlChar*)"OrderStatus", NULL);
			A_ADD_ATTR_LONG(nrow, d, id);
			A_ADD_ATTR_STRING( nrow, d, status );
			A_ADD_ATTR_INT( nrow, d, filled );
			A_ADD_ATTR_INT( nrow, d, remaining );
			A_ADD_ATTR_DOUBLE( nrow, d, avgFillPrice );
			A_ADD_ATTR_INT( nrow, d, permId );
			A_ADD_ATTR_INT( nrow, d, parentId );
			A_ADD_ATTR_DOUBLE( nrow, d, lastFillPrice );
			A_ADD_ATTR_INT( nrow, d, clientId );
			A_ADD_ATTR_STRING( nrow, d, whyHeld );
		}
		break;
	case RowOrd::t_OpenOrder:
		{
			const RowOpenOrder &d = *(RowOpenOrder*)row->data;
			xmlNodePtr nrow = xmlNewChild( parent,
				NULL, (const xmlChar*)"OpenOrder", NULL);
			A_ADD_ATTR_LONG(nrow, d, orderId);
			conv_ib2xml( nrow, "contract", d.contract );
			conv_ib2xml( nrow, "order", d.order );
			conv_ib2xml( nrow, "orderState", d.orderState );
		}
		break;
	}
}

void PacketOrders::dumpXml()
{
	xmlNodePtr root = TwsXml::newDocRoot();
	xmlNodePtr npcd = xmlNewChild( root, NULL,
		(const xmlChar*)"request", NULL );
	xmlNewProp( npcd, (const xmlChar*)"type",
		(const xmlChar*)"open_orders" );
	
	/*xmlNodePtr nqry = */xmlNewChild( npcd, NULL, (xmlChar*)"query", NULL);
	
	xmlNodePtr nrsp = xmlNewChild( npcd, NULL, (xmlChar*)"response", NULL);
	std::vector<RowOrd*>::const_iterator it;
	for( it = list->begin(); it < list->end(); it++ ) {
		conv2xml( nrsp, (*it) );
	}
	
	TwsXml::dumpAndFree( root );
}








PacingControl::PacingControl( int r, int i, int m, int v ) :
	dateTimes(*(new std::vector<int64_t>())),
	violations(*(new std::vector<bool>())),
	maxRequests( r ),
	checkInterval( i ),
	minPacingTime( m ),
	violationPause( v )
{
}

PacingControl::~PacingControl()
{
	delete &violations;
	delete &dateTimes;
}

void PacingControl::setPacingTime( int r, int i, int m )
{
	maxRequests = r;
	checkInterval = i;
	minPacingTime = m;
}


void PacingControl::setViolationPause( int vP )
{
	violationPause = vP;
}


bool PacingControl::isEmpty() const
{
	return dateTimes.empty();
}


void PacingControl::clear()
{
	if( !dateTimes.empty() ) {
		int64_t now = nowInMsecs();
		if( now - dateTimes.back() < 5000  ) {
			// HACK race condition might cause assert in notifyViolation(),
			// to avoid this we would need to ack each request
			DEBUG_PRINTF( "Warning, keep last pacing date time "
				"because it looks too new." );
			dateTimes.erase( dateTimes.begin(), --(dateTimes.end()) );
			violations.erase( violations.begin(), --(violations.end()) );
		} else {
			dateTimes.clear();
			violations.clear();
		}
	}
}


void PacingControl::addRequest()
{
	const int64_t now_t = nowInMsecs();
	dateTimes.push_back( now_t );
	violations.push_back( false );
}


void PacingControl::notifyViolation()
{
	assert( !violations.empty() );
	violations.back() = true;
}


#define SWAP_MAX( _waitX_, _dbg_ ) \
	if( retVal < _waitX_ ) { \
		retVal = _waitX_; \
		dbg = _dbg_ ; \
	}

int PacingControl::goodTime(const char** ddd) const
{
	const int64_t now = nowInMsecs();
	const char* dbg = "don't wait";
	int retVal = INT_MIN;
	
	if( dateTimes.empty() ) {
		*ddd = dbg;
		return retVal;
	}
	
	
	int waitMin = dateTimes.back() + minPacingTime - now;
	SWAP_MAX( waitMin, "wait min" );
	
// 	int waitAvg =  dateTimes.last() + avgPacingTime - now;
// 	SWAP_MAX( waitAvg, "wait avg" );
	
	int waitViol = violations.back() ?
		(dateTimes.back() + violationPause - now) : INT_MIN;
	SWAP_MAX( waitViol, "wait violation" );
	
	int waitBurst = INT_MIN;
	int p_index = dateTimes.size() - maxRequests;
	if( p_index >= 0 ) {
		int64_t p_time = dateTimes.at( p_index );
		waitBurst = p_time + checkInterval - now;
	}
	SWAP_MAX( waitBurst, "wait burst" );
	
	*ddd = dbg;
	return retVal;
}

#undef SWAP


int PacingControl::countLeft() const
{
	const int64_t now = nowInMsecs();
	
	if( (dateTimes.size() > 0) && violations.back() ) {
		int waitViol = dateTimes.back() + violationPause - now;
		if( waitViol > 0 ) {
			return 0;
		}
	}
	
	int retVal = maxRequests;
	std::vector<int64_t>::const_iterator it = dateTimes.end();
	while( it != dateTimes.begin() ) {
		it--;
		int waitBurst = *it + checkInterval - now;
		if( waitBurst > 0 ) {
			retVal--;
		} else {
			break;
		}
	}
	return retVal;
}


void PacingControl::merge( const PacingControl& other )
{
// 	qDebug() << dateTimes;
// 	qDebug() << other.dateTimes;
	std::vector<int64_t>::iterator t_d = dateTimes.begin();
	std::vector<bool>::iterator t_v = violations.begin();
	std::vector<int64_t>::const_iterator o_d = other.dateTimes.begin();
	std::vector<bool>::const_iterator o_v = other.violations.begin();
	
	while( t_d != dateTimes.end() && o_d != other.dateTimes.end() ) {
		if( *o_d < *t_d ) {
			t_d = dateTimes.insert( t_d, *o_d );
			t_v = violations.insert( t_v, *o_v );
			o_d++;
			o_v++;
		} else {
			t_d++;
			t_v++;
		}
	}
	while( o_d != other.dateTimes.end() ) {
		assert( t_d == dateTimes.end() );
		t_d = dateTimes.insert( t_d, *o_d );
		t_v = violations.insert( t_v, *o_v );
		t_d++;
		t_v++;
		o_d++;
		o_v++;
	}
// 	qDebug() << dateTimes;
}







/* In past we've used "exchange{TAB}secType", but exchange seems to be a unique
   for HMDS farms */
#define LAZY_CONTRACT_STR( _c_ ) \
	_c_.exchange


PacingGod::PacingGod( const DataFarmStates &dfs ) :
	dataFarms( dfs ),
	maxRequests( 60 ),
	checkInterval( 601000 ),
	minPacingTime( 1500 ),
	violationPause( 60000 ),
	controlGlobal( *(new PacingControl(
		maxRequests, checkInterval, minPacingTime, violationPause)) ),
	controlHmds(*(new std::map<const std::string, PacingControl*>()) ),
	controlLazy(*(new std::map<const std::string, PacingControl*>()) )
{
}


PacingGod::~PacingGod()
{
	delete &controlGlobal;
	
	std::map<const std::string, PacingControl*>::iterator it;
	for( it = controlHmds.begin(); it != controlHmds.end(); it++ ) {
		delete it->second;
	}
	for( it = controlLazy.begin(); it != controlLazy.end(); it++ ) {
		delete it->second;
	}
	delete &controlHmds;
	delete &controlLazy;
}


void PacingGod::setPacingTime( int r, int i, int m )
{
	maxRequests = r;
	checkInterval = i;
	minPacingTime = m;
	controlGlobal.setPacingTime( r, i, m );
	std::map<const std::string, PacingControl*>::iterator it;
	
	for( it = controlHmds.begin(); it != controlHmds.end(); it++ ) {
		it->second->setPacingTime( r, i, m );
	}
	for( it = controlLazy.begin(); it != controlLazy.end(); it++ ) {
		it->second->setPacingTime( r, i, m  );
	}
}


void PacingGod::setViolationPause( int vP )
{
	violationPause = vP;
	controlGlobal.setViolationPause( vP );
	std::map<const std::string, PacingControl*>::iterator it;
	
	for( it = controlHmds.begin(); it != controlHmds.end(); it++ ) {
		it->second->setViolationPause( vP );
	}
	for( it = controlLazy.begin(); it != controlLazy.end(); it++ ) {
		it->second->setViolationPause( vP );
	}
}


void PacingGod::clear()
{
	if( dataFarms.getActives().empty() ) {
		// clear all PacingControls
		DEBUG_PRINTF( "clear all pacing controls" );
		controlGlobal.clear();
		std::map<const std::string, PacingControl*>::iterator it;
		
		for( it = controlHmds.begin(); it != controlHmds.end(); it++ ) {
			it->second->clear();
		}
		for( it = controlLazy.begin(); it != controlLazy.end(); it++ ) {
			it->second->clear();
		}
	} else {
		// clear only PacingControls of inactive farms
		const std::vector<std::string> inactives = dataFarms.getInactives();
		for( std::vector<std::string>::const_iterator it = inactives.begin();
			    it != inactives.end(); it++ ) {
			if( controlHmds.find(*it) != controlHmds.end() ) {
				DEBUG_PRINTF( "clear pacing control of inactive farm %s",
					it->c_str() );
				controlHmds.find(*it)->second->clear();
			}
		}
	}
}


void PacingGod::addRequest( const IB::Contract& c )
{
	std::string farm;
	std::string lazyC;
	checkAdd( c, &lazyC, &farm );
	
	controlGlobal.addRequest();
	
	if( farm.empty() ) {
		DEBUG_PRINTF( "add request lazy" );
		assert( controlLazy.find(lazyC) != controlLazy.end()
			&& controlHmds.find(farm) == controlHmds.end() );
		controlLazy[lazyC]->addRequest();
	} else {
		DEBUG_PRINTF( "add request farm %s", farm.c_str() );
		assert( controlHmds.find(farm) != controlHmds.end()
			&& controlLazy.find(lazyC) == controlLazy.end() );
		controlHmds[farm]->addRequest();
	}
}


void PacingGod::notifyViolation( const IB::Contract& c )
{
	std::string farm;
	std::string lazyC;
	checkAdd( c, &lazyC, &farm );
	
	controlGlobal.notifyViolation();
	
	if( farm.empty() ) {
		DEBUG_PRINTF( "set violation lazy" );
		assert( controlLazy.find(lazyC) != controlLazy.end()
			&& controlHmds.find(farm) == controlHmds.end() );
		controlLazy[lazyC]->notifyViolation();
	} else {
		DEBUG_PRINTF( "set violation farm %s", farm.c_str() );
		assert( controlHmds.find(farm) != controlHmds.end()
			&& controlLazy.find(lazyC) == controlLazy.end() );
		controlHmds[farm]->notifyViolation();
	}
}


int PacingGod::goodTime( const IB::Contract& c )
{
	const char* dbg;
	std::string farm;
	std::string lazyC;
	checkAdd( c, &lazyC, &farm );
	bool laziesCleared = laziesAreCleared();
	
	if( farm.empty() || !laziesCleared ) {
		// we have to use controlGlobal if any contract's farm is ambiguous
		assert( (controlLazy.find(lazyC) != controlLazy.end()
			&& controlHmds.find(farm) == controlHmds.end() )
			|| !laziesCleared );
		int t = controlGlobal.goodTime(&dbg);
		DEBUG_PRINTF( "get good time global %s %d", dbg, t );
		return t;
	} else {
		assert( (controlHmds.find(farm) != controlHmds.end()
			&& controlLazy.empty()) || laziesCleared );
		int t = controlHmds.find(farm)->second->goodTime(&dbg);
		DEBUG_PRINTF( "get good time farm %s %s %d", farm.c_str(), dbg, t );
		return t;
	}
}


int PacingGod::countLeft( const IB::Contract& c )
{
	std::string farm;
	std::string lazyC;
	checkAdd( c, &lazyC, &farm );
	bool laziesCleared = laziesAreCleared();
	
	if( farm.empty() || !laziesCleared ) {
		// we have to use controlGlobal if any contract's farm is ambiguous
		assert( (controlLazy.find(lazyC) != controlLazy.end()
			&& controlHmds.find(farm) == controlHmds.end())
			|| !laziesCleared );
		int left = controlGlobal.countLeft();
		DEBUG_PRINTF( "get count left global %d", left );
		return left;
	} else {
		assert( (controlHmds.find(farm) != controlHmds.end()
			&& controlLazy.empty()) || laziesCleared );
		int left = controlHmds.find(farm)->second->countLeft();
		DEBUG_PRINTF( "get count left farm %s %d", farm.c_str(), left );
		return controlHmds.find(farm)->second->countLeft();
	}
}



void PacingGod::checkAdd( const IB::Contract& c,
	std::string *lazyC_, std::string *farm_ )
{
	*lazyC_ = LAZY_CONTRACT_STR(c);
	*farm_ = dataFarms.getHmdsFarm(c);
	
	std::vector<std::string> lazies;
	bool append_lazyC = true;
	std::map<const std::string, PacingControl*>::const_iterator it =
		controlLazy.begin();
	while( it != controlLazy.end() ) {
		lazies.push_back( it->first );
		if( it->first == *lazyC_ ) {
			append_lazyC = false;
		}
		it++;
	}
	if( append_lazyC ) {
		lazies.push_back(*lazyC_);
	}
	
	for( std::vector<std::string>::const_iterator it = lazies.begin();
		    it != lazies.end(); it++ ) {
	const std::string &lazyC = *it;
	std::string farm = dataFarms.getHmdsFarm(lazyC);
	if( !farm.empty() ) {
		if( controlHmds.find(farm) == controlHmds.end() ) {
			PacingControl *pC;
			if( controlLazy.find(lazyC) != controlLazy.end() ) {
				DEBUG_PRINTF( "move pacing control lazy to farm %s, %s",
					lazyC.c_str(), farm.c_str() );
				
				
				std::map<const std::string, PacingControl*>::iterator it
					= controlLazy.find(lazyC);
				pC = it->second;
				controlLazy.erase(it);
			} else {
				DEBUG_PRINTF( "create pacing control for farm %s",
					farm.c_str() );
				pC = new PacingControl(
					maxRequests, checkInterval, minPacingTime, violationPause);
			}
			controlHmds[farm] = pC;
		} else {
			if( controlLazy.find(lazyC) == controlLazy.end() ) {
				// fine - no history about that
			} else {
				DEBUG_PRINTF( "merge pacing control lazy into farm %s %s",
					lazyC.c_str(), farm.c_str() );
				
				std::map<const std::string, PacingControl*>::iterator it
					= controlLazy.find(lazyC);
				PacingControl *pC = it->second;
				controlLazy.erase(it);
				controlHmds.find(farm)->second->merge(*pC);
				delete pC;
			}
		}
		assert( controlHmds.find(farm) != controlHmds.end() );
		assert( controlLazy.find(lazyC) == controlLazy.end() );
		
	} else if( controlLazy.find(lazyC) == controlLazy.end() ) {
			DEBUG_PRINTF( "create pacing control for lazy %s", lazyC.c_str() );
			PacingControl *pC = new PacingControl(
				maxRequests, checkInterval, minPacingTime, violationPause);
			controlLazy[lazyC] = pC;
			
			assert( controlHmds.find(farm) == controlHmds.end() );
			assert( controlLazy.find(lazyC) != controlLazy.end() );
	}
	}
}


bool PacingGod::laziesAreCleared() const
{
	
	bool retVal = true;
	std::map<const std::string, PacingControl*>::iterator it;
	for( it = controlLazy.begin(); it != controlLazy.end(); it++ ) {
		retVal &=  it->second->isEmpty();
	}
	return retVal;
}








DataFarmStates::DataFarmStates() :
	mStates( *(new std::map<const std::string, State>()) ),
	hStates( *(new std::map<const std::string, State>()) ),
	mLearn( *(new std::map<const std::string, std::string>()) ),
	hLearn( *(new std::map<const std::string, std::string>()) ),
	lastMsgNumber(INT_MIN),
	edemo_checked(false)
{
	initHardCodedFarms();
}

DataFarmStates::~DataFarmStates()
{
	delete &hLearn;
	delete &mLearn;
	delete &hStates;
	delete &mStates;
}

void DataFarmStates::initHardCodedFarms()
{
	hLearn["BELFOX"] = "euhmds2";
	hLearn["BM"] = "euhmds2";
	hLearn["BVME"] = "euhmds2";
	hLearn["CBOT"] = "euhmds2";
	hLearn["DTB"] = "euhmds2";
	hLearn["EBS"] = "euhmds2";
	hLearn["EDXNO"] = "euhmds2";
	hLearn["FTA"] = "euhmds2";
	hLearn["IBIS"] = "euhmds2";
	hLearn["IDEM"] = "euhmds2";
	hLearn["IPE"] = "euhmds2";
	hLearn["LIFFE_NF"] = "euhmds2";
	hLearn["LIFFE"] = "euhmds2";
	hLearn["LSE"] = "euhmds2";
	hLearn["LSSF"] = "euhmds2";
	hLearn["MATIF"] = "euhmds2";
	hLearn["MEFFRV"] = "euhmds2";
	hLearn["MONEP"] = "euhmds2";
	hLearn["OMS"] = "euhmds2";
	hLearn["SFB"] = "euhmds2";
	hLearn["SOFFEX"] = "euhmds2";
	
	hLearn["AMEX"] = "ushmds2a";
	hLearn["AQS"] = "ushmds2a";
	hLearn["ARCA"] = "ushmds2a";
	hLearn["CBOE"] = "ushmds2a";
	hLearn["CDE"] = "ushmds2a";
	hLearn["CFE"] = "ushmds2a";
	hLearn["CME"] = "ushmds2a";
	hLearn["ECBOT"] = "ushmds2a";
	hLearn["GLOBEX"] = "ushmds2a";
	hLearn["IDEALPRO"] = "ushmds2a";
	hLearn["IPE"] = "ushmds2a";
	hLearn["ISE"] = "ushmds2a";
	hLearn["NASDAQ"] = "ushmds2a";
	hLearn["NYBOT"] = "ushmds2a";
	hLearn["NYMEX"] = "ushmds2a";
	hLearn["NYSELIFFE"] = "ushmds2a";
	hLearn["NYSE"] = "ushmds2a";
	hLearn["ONE"] = "ushmds2a";
	hLearn["PHLX"] = "ushmds2a";
	hLearn["PSE"] = "ushmds2a";
	hLearn["TSE"] = "ushmds2a";
	
	hLearn["ASX"] = "hkhmds2";
	hLearn["HKFE"] = "hkhmds2";
	hLearn["KSE"] = "hkhmds2";
	hLearn["NSE"] = "hkhmds2";
	hLearn["OSE.JPN"] = "hkhmds2";
	hLearn["SGX"] = "hkhmds2";
	hLearn["SNFE"] = "hkhmds2";
	hLearn["TSE.JPN"] = "hkhmds2";
}


void DataFarmStates::check_edemo_hack( const std::string &farm )
{
	if( edemo_checked ) {
		return;
	}
	
	/* This is a dirty HACK. We would still crash if we are changing to edemo
	   after reconnect. To fix it we would need to handle the general case that
	   we've learned a wrong farm somehow which should be handled anyway. */
	if( farm == "ibdemo" || farm == "demohmds" ) {
		DEBUG_PRINTF( "Dropping hardcoded data farms because edemo TWS." );
		hLearn.clear();
	}
	edemo_checked = true;
}


void DataFarmStates::setAllBroken()
{
	std::map<const std::string, State>::iterator it;
	
	it = mStates.begin();
	while( it != mStates.end() ) {
		it->second = BROKEN;
		it++;
	}
	
	it = hStates.begin();
	while( it != hStates.end() ) {
		it->second = BROKEN;
		it++;
	}
	
	last_learned_lazy_contract.clear();
}


void DataFarmStates::notify(int msgNumber, int errorCode,
	const std::string &msg)
{
	lastMsgNumber = msgNumber;
	std::string farm;
	State state;
	std::map<const std::string, State> *pHash = NULL;
	
	// prefixes used for getFarm() are taken from past logs
	switch( errorCode ) {
	case 2103:
		//API docu: "A market data farm is disconnected."
		pHash = &mStates;
		state = BROKEN;
		farm = getFarm("Market data farm connection is broken:", msg);
		break;
	case 2104:
		//API docu: "A market data farm is connected."
		pHash = &mStates;
		state = OK;
		farm = getFarm("Market data farm connection is OK:", msg);
		break;
	case 2105:
		//API docu: "A historical data farm is disconnected."
		pHash = &hStates;
		state = BROKEN;
		farm = getFarm("HMDS data farm connection is broken:", msg);
		break;
	case 2106:
		//API docu: "A historical data farm is connected."
		pHash = &hStates;
		state = OK;
		farm = getFarm("HMDS data farm connection is OK:", msg);
		break;
	case 2107:
		//API docu: "A historical data farm connection has become inactive but should be available upon demand."
		pHash = &hStates;
		state = INACTIVE;
		farm = getFarm("HMDS data farm connection is inactive but should be available upon demand.", msg);
		break;
	case 2108:
		//API docu: "A market data farm connection has become inactive but should be available upon demand."
		pHash = &mStates;
		state = INACTIVE;
		farm = getFarm("Market data farm connection is inactive but should be available upon demand.", msg);
		break;
	default:
		assert(false);
		return;
	}
	
	check_edemo_hack(farm);
	
	lastChanged = farm;
	(*pHash)[farm] = state;
	last_learned_lazy_contract.clear();
// 	qDebug() << *pHash; // TODO print farms with states
}


/// static member
std::string DataFarmStates::getFarm( const std::string &prefix,
	const std::string& msg )
{
	assert( prefix == msg.substr(0, prefix.size()) );
	
	return msg.substr( prefix.size() );
}


void DataFarmStates::learnMarket( const IB::Contract& )
{
	assert( false ); //not implemented
}


void DataFarmStates::learnHmds( const IB::Contract& c )
{
	std::string lazyC = LAZY_CONTRACT_STR(c);
	
	if( last_learned_lazy_contract == lazyC ) {
		return;
	}
	last_learned_lazy_contract = lazyC;
	
	std::vector<std::string> sl;
	std::map<const std::string, State>::const_iterator it = hStates.begin();
	while( it != hStates.end() ) {
		if( it->second == OK ) {
			sl.push_back( it->first );
		}
		it++;
	}
	if( sl.size() <= 0 ) {
		assert(false); // assuming at least one farm must be active
	} else if( sl.size() == 1 ) {
		if( hLearn.find(lazyC) != hLearn.end() ) {
			assert( hLearn.find( lazyC )->second == sl.front() );
		} else {
			hLearn[lazyC] = sl.front();
			DEBUG_PRINTF( "learn HMDS farm (unique): %s %s",
				lazyC.c_str(), sl.front().c_str() );
		}
	} else {
		if( hLearn.find(lazyC) != hLearn.end() ) {
			// here we just validate that the known farm is active
			const std::string &known_farm =  hLearn.find(lazyC)->second;
			bool sl_contains_lazyC = false;
			for( std::vector<std::string>::const_iterator it = sl.begin();
				    it != sl.end(); it++ ) {
				if( *it == known_farm ) {
					sl_contains_lazyC = true;
					break;
				}
			}
			assert( sl_contains_lazyC );
		} else {
			//but doing nothing
			std::string dbg_active_farms;
			for( std::vector<std::string>::const_iterator it = sl.begin();
				    it != sl.end(); it++ ) {
				dbg_active_farms.append(",").append(*it);
			}
			
			DEBUG_PRINTF( "learn HMDS farm (ambiguous): %s (%s)",
				lazyC.c_str(),
				(dbg_active_farms.c_str()+1) );
		}
	}
}


void DataFarmStates::learnHmdsLastOk(int msgNumber, const IB::Contract& c )
{
	if( msgNumber != (lastMsgNumber + 1) ) {
		return;
	}
	/* Last tws message notified about a farm state change ...*/
	
	std::map<const std::string, State>::const_iterator it =
		hStates.find(lastChanged);
	
	if( it != hStates.end() && it->second == OK ) {
		/* ... and it was a hist farm which became OK. So this contract should
		   belong to that farm.*/
		std::string lazyC = LAZY_CONTRACT_STR(c);
		if( hLearn.find(lazyC) != hLearn.end() ) {
			assert( hLearn.find(lazyC)->second == lastChanged );
		} else {
			hLearn[lazyC] = lastChanged;
			DEBUG_PRINTF( "learn HMDS farm (last ok): %s %s",
				lazyC.c_str(), lastChanged.c_str());
		}
	}
}


std::vector<std::string> DataFarmStates::getInactives() const
{
	std::vector<std::string> sl;
	std::map<const std::string, State>::const_iterator it = hStates.begin();
	while( it != hStates.end() ) {
		if( it->second == INACTIVE || it->second == BROKEN ) {
			sl.push_back( it->first );
		}
		it++;
	}
	return sl;
}


std::vector<std::string> DataFarmStates::getActives() const
{
	std::vector<std::string> sl;
	std::map<const std::string, State>::const_iterator it = hStates.begin();
	while( it != hStates.end() ) {
		if( it->second == OK ) {
			sl.push_back( it->first );
		}
		it++;
	}
	return sl;
}


std::string DataFarmStates::getMarketFarm( const IB::Contract& c ) const
{
	std::string lazyC = LAZY_CONTRACT_STR(c);
	std::map<const std::string, std::string>::const_iterator it = mLearn.find(lazyC);
	return (it != mLearn.end()) ? it->second : "";
}


std::string DataFarmStates::getHmdsFarm( const IB::Contract& c ) const
{
	std::string lazyC = LAZY_CONTRACT_STR(c);
	std::map<const std::string, std::string>::const_iterator it = hLearn.find(lazyC);
	return (it != hLearn.end()) ? it->second : "";
}


std::string DataFarmStates::getHmdsFarm( const std::string& lazyC ) const
{
	std::map<const std::string, std::string>::const_iterator it = hLearn.find(lazyC);
	return (it != hLearn.end()) ? it->second : "";
}


#undef LAZY_CONTRACT_STR




