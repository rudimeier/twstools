/*** tws_meta.cpp -- helper structs for IB/API messages
 *
 * Copyright (C) 2010-2013 Ruediger Meier
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

#include "tws_meta.h"
#include "tws_xml.h"
#include "tws_query.h"
#include "tws_util.h"
#include "debug.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <limits.h>
#include <string.h>




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


void HistTodo::dumpLeft() const
{
	std::list<HistRequest*>::const_iterator it = leftRequests.begin();
	while( it != leftRequests.end() ) {
		fprintf( stderr, "[%p]\t%s\n",
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
		std::string farm = dfs->getHmdsFarm((*it)->ibContract);
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
		const IB::Contract& c = tmp_hR->ibContract;
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
	int wait = pG->goodTime( todo_hR->ibContract );
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

int HistTodo::skip_by_perm(const IB::Contract& con)
{
	int cnt_skipped = 0;
	std::list<HistRequest*>::iterator it = leftRequests.begin();

	if (con.symbol.empty() || con.secType.empty() || con.exchange.empty() ) {
		goto return_skip_by_con;
	}

	while( it != leftRequests.end() ) {
		HistRequest *hr = *it;
		const IB::Contract &ci = hr->ibContract;
		if (strcasecmp( ci.symbol.c_str(), con.symbol.c_str()) == 0 &&
				strcasecmp( ci.secType.c_str(), con.secType.c_str()) == 0 &&
				strcasecmp( ci.exchange.c_str(), con.exchange.c_str()) == 0) {
			cnt_skipped++;
			errorRequests.push_back(hr);
			it = leftRequests.erase(it);
		} else {
			++it;
		}
	}

return_skip_by_con:
	DEBUG_PRINTF("skipped %d requests for contracts like %s,%s,%s",
		cnt_skipped,
		con.symbol.c_str(), con.secType.c_str(), con.exchange.c_str());
	return cnt_skipped;
}

static inline bool is_quote_req(const HistRequest &hr)
{
	return strcasecmp(hr.whatToShow.c_str(), "BID") == 0
	       || strcasecmp(hr.whatToShow.c_str(), "ASK") == 0;
}

int HistTodo::skip_by_nodata(const HistRequest& hr)
{
	const IB::Contract &con = hr.ibContract;
	int cnt_skipped = 0;
	std::list<HistRequest*>::iterator it = leftRequests.begin();

	if (con.symbol.empty() || con.secType.empty() || con.exchange.empty() ) {
		goto return_skip_by_con;
	}

	while( it != leftRequests.end() ) {
		HistRequest *hi = *it;
		const IB::Contract &ci = hi->ibContract;
		if (strcasecmp( ci.symbol.c_str(), con.symbol.c_str()) == 0 &&
			  strcasecmp( ci.secType.c_str(), con.secType.c_str()) == 0 &&
			  strcasecmp( ci.exchange.c_str(), con.exchange.c_str()) == 0 &&
			  strcasecmp( ci.currency.c_str(), con.currency.c_str()) == 0 &&
			  strcasecmp( ci.multiplier.c_str(), con.multiplier.c_str()) == 0 &&
			  strcasecmp( ci.localSymbol.c_str(), con.localSymbol.c_str()) == 0 &&
			  ((is_quote_req(hr) && is_quote_req(*hi)) ||
			   strcasecmp( hr.whatToShow.c_str(), hi->whatToShow.c_str()) == 0)
			) {
			cnt_skipped++;
			errorRequests.push_back(hi);
			it = leftRequests.erase(it);
		} else {
			++it;
		}
	}

return_skip_by_con:
	DEBUG_PRINTF("skipped %d requests for contracts like %s,%s,%s",
		cnt_skipped,
		con.symbol.c_str(), con.secType.c_str(), con.exchange.c_str());
	return cnt_skipped;
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




PlaceOrderTodo::PlaceOrderTodo() :
	curIndex(-1),
	placeOrders(*(new std::vector<PlaceOrder>()))
{
}

PlaceOrderTodo::~PlaceOrderTodo()
{
	delete &placeOrders;
}

int PlaceOrderTodo::countLeft() const
{
	return placeOrders.size() - curIndex - 1;
}

void PlaceOrderTodo::checkout()
{
	assert( countLeft() > 0 );
	curIndex++;
}

const PlaceOrder& PlaceOrderTodo::current() const
{
	assert( curIndex >= 0 && curIndex < (int)placeOrders.size() );
	return placeOrders.at(curIndex);
}

void PlaceOrderTodo::add( const PlaceOrder& po )
{
	placeOrders.push_back(po);
}




MktDataTodo::MktDataTodo() :
	mktDataRequests(*(new std::vector<MktDataRequest>()))
{
}

MktDataTodo::~MktDataTodo()
{
	delete &mktDataRequests;
}

void MktDataTodo::add( const MktDataRequest& mkr )
{
	mktDataRequests.push_back(mkr);
}




WorkTodo::WorkTodo() :
	acc_status_todo(false),
	executions_todo(false),
	orders_todo(false),
	_contractDetailsTodo( new ContractDetailsTodo() ),
	_histTodo( new HistTodo() ),
	_place_order_todo( new PlaceOrderTodo() ),
	_market_data_todo( new MktDataTodo() )
{
}


WorkTodo::~WorkTodo()
{
	if( _market_data_todo != NULL ) {
		delete _market_data_todo;
	}
	if( _place_order_todo != NULL ) {
		delete _place_order_todo;
	}
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

PlaceOrderTodo* WorkTodo::placeOrderTodo() const
{
	return _place_order_todo;
}

const PlaceOrderTodo& WorkTodo::getPlaceOrderTodo() const
{
	return *_place_order_todo;
}

MktDataTodo* WorkTodo::mktDataTodo() const
{
	return _market_data_todo;
}

const MktDataTodo& WorkTodo::getMktDataTodo() const
{
	return *_market_data_todo;
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


int WorkTodo::read_req( const xmlNodePtr xn )
{
	int ret = 1;
	char* tmp = (char*) xmlGetProp( xn, (const xmlChar*) "type" );

	if( tmp == NULL ) {
		fprintf(stderr, "Warning, no request type specified.\n");
		ret = 0;
	} else if( strcmp( tmp, "contract_details") == 0 ) {
		PacketContractDetails *pcd = PacketContractDetails::fromXml(xn);
		_contractDetailsTodo->add(pcd->getRequest());
		delete pcd;
	} else if ( strcmp( tmp, "historical_data") == 0 ) {
		PacketHistData *phd = PacketHistData::fromXml(xn);
		_histTodo->add( phd->getRequest() );
		delete phd;
	} else if ( strcmp( tmp, "place_order") == 0 ) {
		PacketPlaceOrder *ppo = PacketPlaceOrder::fromXml(xn);
		_place_order_todo->add( ppo->getRequest() );
		delete ppo;
	} else if ( strcmp( tmp, "market_data") == 0 ) {
		PacketMktData *pmd = PacketMktData::fromXml(xn);
		_market_data_todo->add( pmd->getRequest() );
		delete pmd;
		// HACK always get contractDetails too
		PacketContractDetails *pcd = PacketContractDetails::fromXml(xn);
		_contractDetailsTodo->add(pcd->getRequest());
		delete pcd;
	} else if ( strcmp( tmp, "account") == 0 ) {
		addSimpleRequest(GenericRequest::ACC_STATUS_REQUEST);
	} else if ( strcmp( tmp, "executions") == 0 ) {
		addSimpleRequest(GenericRequest::EXECUTIONS_REQUEST);
	} else if ( strcmp( tmp, "open_orders") == 0 ) {
		addSimpleRequest(GenericRequest::ORDERS_REQUEST);
	} else {
		fprintf(stderr, "Warning, unknown request type '%s' ignored.\n", tmp );
		ret = 0;
	}

	free(tmp);
	return ret;
}

int WorkTodo::read_file( const char *fileName )
{
	int retVal = -1;

	TwsXml file;
	if( ! file.openFile(fileName) ) {
		return retVal;
	}
	retVal = 0;
	xmlNodePtr xn;
	while( (xn = file.nextXmlNode()) != NULL ) {
		assert( xn->type == XML_ELEMENT_NODE  );
		if( strcmp((char*)xn->name, "request") == 0 ) {
			retVal += read_req( xn );
		} else {
			fprintf(stderr, "Warning, unknown request tag '%s' ignored.\n",
				xn->name );
		}
	}

	return retVal;
}




static void del_tws_rows( std::vector<TwsRow>* v )
{
	std::vector<TwsRow>::const_iterator it;
	for( it = v->begin(); it < v->end(); it++ ) {
		switch( it->type ) {
		case t_error:
			delete (RowError*)it->data;
			break;
		case t_orderStatus:
			delete (RowOrderStatus*)it->data;
			break;
		case t_openOrder:
			delete (RowOpenOrder*)it->data;
			break;
		}
	}
}



Packet::Packet() :
	mode(CLEAN),
	error(REQ_ERR_NONE)
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

req_err Packet::getError() const
{
	return error;
}

void Packet::closeError( req_err e )
{
	if( mode != RECORD ) {
		DEBUG_PRINTF( "Warning, closeError closed packet.");
		assert( mode == CLOSED );
		return;
	}
	mode = CLOSED;
	error = e;
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
				pcd->request = new ContractDetailsRequest();
				from_xml(pcd->request, p);
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

	to_xml(npcd, *request );

	xmlNodePtr nrsp = xmlNewChild( npcd, NULL, (xmlChar*)"response", NULL);
	for( size_t i=0; i<cdList->size(); i++ ) {
		conv_ib2xml( nrsp, "ContractDetails", (*cdList)[i] );
	}

	TwsXml::dumpAndFree( root );
}








PacketHistData::PacketHistData() :
		rows(*(new std::vector<RowHist>()))
{
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
				phd->request = new HistRequest();
				from_xml(phd->request, p);
			}
			if( strcmp((char*)p->name, "response") == 0 ) {
				for( xmlNodePtr q = p->children; q!= NULL; q=q->next) {
					if( q->type != XML_ELEMENT_NODE ) {
						continue;
					}
					if( strcmp((char*)q->name, "row") == 0 ) {
						RowHist *row = new RowHist();
						from_xml( row, q );
						phd->rows.push_back(*row);
						delete row;
					} else if( strcmp((char*)q->name, "fin") == 0 ) {
						RowHist *fin = new RowHist();
						from_xml( fin, q );
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

void PacketHistData::dumpXml()
{
	xmlNodePtr root = TwsXml::newDocRoot();
	xmlNodePtr nphd = xmlNewChild( root, NULL,
		(const xmlChar*)"request", NULL );
	xmlNewProp( nphd, (const xmlChar*)"type",
		(const xmlChar*)"historical_data" );

	to_xml(nphd, *request);

	if( mode == CLOSED ) {
		xmlNodePtr nrsp = xmlNewChild( nphd, NULL, (xmlChar*)"response", NULL);
		for( size_t i=0; i<rows.size(); i++ ) {
			to_xml( nrsp, "row", rows[i] );
		}
		to_xml( nrsp, "fin", finishRow );
	}
	TwsXml::dumpAndFree( root );
}

const HistRequest& PacketHistData::getRequest() const
{
	return *request;
}


void PacketHistData::clear()
{
	mode = CLEAN;
	error = REQ_ERR_NONE;
	reqId = -1;
	if( request != NULL ) {
		delete request;
		request = NULL;
	}
	rows.clear();
	finishRow = dflt_RowHist;
}


void PacketHistData::record( int reqId, const HistRequest& hR )
{
	assert( mode == CLEAN && error == REQ_ERR_NONE && request == NULL );
	mode = RECORD;
	this->reqId = reqId;
	this->request = new HistRequest( hR );
}


void PacketHistData::append( int reqId,  const RowHist &row )
{
	assert( mode == RECORD && error == REQ_ERR_NONE );
	assert( this->reqId == reqId );

	if( strncmp( row.date.c_str(), "finished", 8) == 0) {
		mode = CLOSED;
		finishRow = row;
	} else {
		rows.push_back( row );
	}
}


void PacketHistData::dump( bool printFormatDates )
{
	const IB::Contract &c = request->ibContract;
	const char *wts = short_wts( request->whatToShow.c_str() );
	const char *bss = short_bar_size( request->barSizeSetting.c_str());

	for( std::vector<RowHist>::const_iterator it = rows.begin();
		it != rows.end(); it++ ) {
		std::string expiry = c.expiry;
		std::string dateTime = it->date;
		if( printFormatDates ) {
			if( expiry.empty() ) {
				expiry = "0000-00-00";
			} else {
				expiry = ib_date2iso( c.expiry );
			}
			dateTime = ib_date2iso(it->date);
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




PacketPlaceOrder::PacketPlaceOrder() :
	request(NULL),
	list( new std::vector<TwsRow>() )
{
}

PacketPlaceOrder::~PacketPlaceOrder()
{
	del_tws_rows(list);
	delete list;
	if( request != NULL ) {
		delete request;
	}
}

PacketPlaceOrder * PacketPlaceOrder::fromXml( xmlNodePtr root )
{
	PacketPlaceOrder *ppo = new PacketPlaceOrder();

	for( xmlNodePtr p = root->children; p!= NULL; p=p->next) {
		if( p->type == XML_ELEMENT_NODE ) {
			if( strcmp((char*)p->name, "query") == 0 ) {
				ppo->request = new PlaceOrder();
				from_xml(ppo->request, p);
			}
			if( strcmp((char*)p->name, "response") == 0 ) {
				for( xmlNodePtr q = p->children; q!= NULL; q=q->next) {
					if( q->type != XML_ELEMENT_NODE ) {
						continue;
					}
					/* not implemented yet */
					assert( false );
				}
			}
		}
	}
	return ppo;
}

void PacketPlaceOrder::dumpXml()
{
	xmlNodePtr root = TwsXml::newDocRoot();
	xmlNodePtr nppo = xmlNewChild( root, NULL,
		(const xmlChar*)"request", NULL );
	xmlNewProp( nppo, (const xmlChar*)"type",
		(const xmlChar*)"place_order" );

	to_xml(nppo, *request);

	if( mode == CLOSED ) {
		xmlNodePtr nrsp = xmlNewChild( nppo, NULL, (xmlChar*)"response", NULL);
		std::vector<TwsRow>::const_iterator it;
		for( it = list->begin(); it < list->end(); it++ ) {
			to_xml( nrsp, *it );
		}
	}

	TwsXml::dumpAndFree( root );
}

const PlaceOrder& PacketPlaceOrder::getRequest() const
{
	return *request;
}

void PacketPlaceOrder::clear()
{
	mode = CLEAN;
	error = REQ_ERR_NONE;
	del_tws_rows(list);
	list->clear();
	if( request != NULL ) {
		delete request;
		request = NULL;
	}
}

void PacketPlaceOrder::record( long orderId, const PlaceOrder& oP )
{
	assert( mode == CLEAN && error == REQ_ERR_NONE && request == NULL );
	mode = RECORD;
	this->request = new PlaceOrder( oP );
	this->request->orderId = orderId;
	this->request->time_sent = nowInMsecs();
}

void PacketPlaceOrder::modify( const PlaceOrder& oP )
{
	assert( mode == RECORD );
	assert( this->request->orderId == oP.orderId );

	delete this->request;
	this->request = new PlaceOrder( oP );
	this->request->time_sent = nowInMsecs();
}

void PacketPlaceOrder::append( const RowError& err )
{
	TwsRow arow = { t_error, new RowError(err) };
	list->push_back( arow );
}

void PacketPlaceOrder::append( const RowOrderStatus& row )
{
	TwsRow arow = { t_orderStatus, new RowOrderStatus(row) };
	list->push_back( arow );

	if( row.remaining == 0 ) {
		mode = CLOSED;
	}
}

void PacketPlaceOrder::append( const RowOpenOrder& row )
{
	TwsRow arow = { t_openOrder, new RowOpenOrder(row) };
	list->push_back( arow );

	if( request->order.whatIf ) {
		/* for whatIf order we expect only one OpenOrder callback */
		mode = CLOSED;
	}
}








PacketAccStatus::PacketAccStatus() :
	request(NULL),
	list( new std::vector<RowAcc*>() )
{
}

PacketAccStatus::~PacketAccStatus()
{
	del_list_elements();
	delete list;
	if( request != NULL ) {
		delete request;
	}
}

void PacketAccStatus::clear()
{
	assert( finished() );
	mode = CLEAN;
	del_list_elements();
	list->clear();
	if( request != NULL ) {
		delete request;
	}
	request = NULL;
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

void PacketAccStatus::record( const AccStatusRequest& aR )
{
	assert( empty() );
	mode = RECORD;
	this->request = new AccStatusRequest(aR);
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

void PacketAccStatus::dumpXml()
{
	xmlNodePtr root = TwsXml::newDocRoot();
	xmlNodePtr npcd = xmlNewChild( root, NULL,
		(const xmlChar*)"request", NULL );
	xmlNewProp( npcd, (const xmlChar*)"type",
		(const xmlChar*)"account" );

	to_xml(npcd, *request);

	xmlNodePtr nrsp = xmlNewChild( npcd, NULL, (xmlChar*)"response", NULL);
	std::vector<RowAcc*>::const_iterator it;
	for( it = list->begin(); it < list->end(); it++ ) {
		to_xml( nrsp, **it );
	}

	TwsXml::dumpAndFree( root );
}








PacketExecutions::PacketExecutions() :
	reqId(-1),
	request(NULL),
	list( new std::vector<RowExecution*>() )
{
}

PacketExecutions::~PacketExecutions()
{
	del_list_elements();
	delete list;
	if( request != NULL ) {
		delete request;
	}
}

void PacketExecutions::clear()
{
	assert( finished() );
	mode = CLEAN;
	del_list_elements();
	list->clear();
	if( request != NULL ) {
		delete request;
	}
	request = NULL;
}

void PacketExecutions::del_list_elements()
{
	std::vector<RowExecution*>::const_iterator it;
	for( it = list->begin(); it < list->end(); it++ ) {
		delete (*it);
	}
}

void PacketExecutions::record(  const int reqId, const ExecutionsRequest &eR )
{
	assert( empty() );
	mode = RECORD;
	this->reqId = reqId;
	this->request = new ExecutionsRequest( eR );
}

void PacketExecutions::append( int reqId, const RowExecution &row )
{
	RowExecution *arow = new RowExecution(row);
	list->push_back( arow );
}

void PacketExecutions::appendExecutionsEnd( int reqId )
{
	mode = CLOSED;
}

void PacketExecutions::dumpXml()
{
	xmlNodePtr root = TwsXml::newDocRoot();
	xmlNodePtr npcd = xmlNewChild( root, NULL,
		(const xmlChar*)"request", NULL );
	xmlNewProp( npcd, (const xmlChar*)"type",
		(const xmlChar*)"executions" );

	to_xml(npcd, *request);

	xmlNodePtr nrsp = xmlNewChild( npcd, NULL, (xmlChar*)"response", NULL);
	std::vector<RowExecution*>::const_iterator it;
	for( it = list->begin(); it < list->end(); it++ ) {
		to_xml( nrsp, **it );
	}

	TwsXml::dumpAndFree( root );
}







PacketOrders::PacketOrders() :
	request(NULL),
	list( new std::vector<TwsRow>() )
{
}

PacketOrders::~PacketOrders()
{
	del_tws_rows(list);
	delete list;
	if( request != NULL ) {
		delete request;
	}
}

void PacketOrders::clear()
{
	assert( finished() );
	mode = CLEAN;
	del_tws_rows(list);
	list->clear();
	if( request != NULL ) {
		delete request;
	}
	request = NULL;
}

void PacketOrders::record( const OrdersRequest &oR )
{
	assert( empty() );
	mode = RECORD;
	this->request = new OrdersRequest( oR );
}

void PacketOrders::append( const RowOrderStatus& row )
{
	TwsRow arow = { t_orderStatus, new RowOrderStatus(row) };
	list->push_back( arow );
}

void PacketOrders::append( const RowOpenOrder& row )
{
	TwsRow arow = { t_openOrder, new RowOpenOrder(row) };
	list->push_back( arow );
}

void PacketOrders::appendOpenOrderEnd()
{
	mode = CLOSED;
}

void PacketOrders::dumpXml()
{
	xmlNodePtr root = TwsXml::newDocRoot();
	xmlNodePtr npcd = xmlNewChild( root, NULL,
		(const xmlChar*)"request", NULL );
	xmlNewProp( npcd, (const xmlChar*)"type",
		(const xmlChar*)"open_orders" );

	to_xml(npcd, *request);

	xmlNodePtr nrsp = xmlNewChild( npcd, NULL, (xmlChar*)"response", NULL);
	std::vector<TwsRow>::const_iterator it;
	for( it = list->begin(); it < list->end(); it++ ) {
		to_xml( nrsp, *it );
	}

	TwsXml::dumpAndFree( root );
}




PacketMktData::PacketMktData()/* :
		rows(*(new std::vector<RowHist>()))*/
{
	reqId = -1;
	request = NULL;
}

PacketMktData::~PacketMktData()
{
// 	delete &rows;
	if( request != NULL ) {
		delete request;
	}
}

PacketMktData * PacketMktData::fromXml( xmlNodePtr root )
{
	PacketMktData *pmd = new PacketMktData();

	for( xmlNodePtr p = root->children; p!= NULL; p=p->next) {
		if( p->type == XML_ELEMENT_NODE ) {
			if( strcmp((char*)p->name, "query") == 0 ) {
				pmd->request = new MktDataRequest();
				from_xml(pmd->request, p);
			}
			if( strcmp((char*)p->name, "response") == 0 ) {
				for( xmlNodePtr q = p->children; q!= NULL; q=q->next) {
					if( q->type != XML_ELEMENT_NODE ) {
						continue;
					}
// 					if( strcmp((char*)q->name, "row") == 0 ) {
// 						RowHist *row = new RowHist();
// 						from_xml( row, q );
// 						phd->rows.push_back(*row);
// 						delete row;
// 					} else if( strcmp((char*)q->name, "fin") == 0 ) {
// 						RowHist *fin = new RowHist();
// 						from_xml( fin, q );
// 						phd->finishRow = *fin;
// 						phd->mode = CLOSED;
// 						delete fin;
// 					}
				}
			}
		}
	}
	return pmd;
}

void PacketMktData::dumpXml()
{
}

const MktDataRequest& PacketMktData::getRequest() const
{
	return *request;
}


void PacketMktData::clear()
{
	mode = CLEAN;
	error = REQ_ERR_NONE;
	reqId = -1;
	if( request != NULL ) {
		delete request;
		request = NULL;
	}
// 	rows.clear();
}


void PacketMktData::record( int reqId, const MktDataRequest& mR )
{
	assert( mode == CLEAN && error == REQ_ERR_NONE && request == NULL );
	mode = RECORD;
	this->reqId = reqId;
	this->request = new MktDataRequest( mR );
}


// void PacketHistData::append( int reqId,  const RowHist &row )
// {
// 	assert( mode == RECORD && error == REQ_ERR_NONE );
// 	assert( this->reqId == reqId );
//
// 	if( strncmp( row.date.c_str(), "finished", 8) == 0) {
// 		mode = CLOSED;
// 		finishRow = row;
// 	} else {
// 		rows.push_back( row );
// 	}
// }




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

void PacingControl::remove_last_request()
{
	if( dateTimes.size() <= 0 ) {
		DEBUG_PRINTF( "Warning, assert remove_last_request");
		return;
	}
	dateTimes.pop_back();
	violations.pop_back();
}

void PacingControl::notifyViolation()
{
	if( violations.empty() ) {
		/* Either we have cleared violations for no good reason or TWS itself
		   made requests where we don't know about */
		DEBUG_PRINTF( "Warning, pacing violation occured while HMDS farm "
			"looks clear.");
		addRequest();
	}
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

void PacingGod::remove_last_request( const IB::Contract& c )
{
	std::string farm;
	std::string lazyC;
	checkAdd( c, &lazyC, &farm );

	controlGlobal.remove_last_request();

	if( farm.empty() ) {
		DEBUG_PRINTF( "remove request lazy" );
		assert( controlLazy.find(lazyC) != controlLazy.end()
			&& controlHmds.find(farm) == controlHmds.end() );
		controlLazy[lazyC]->remove_last_request();
	} else {
		DEBUG_PRINTF( "remove request farm %s", farm.c_str() );
		assert( controlHmds.find(farm) != controlHmds.end()
			&& controlLazy.find(lazyC) == controlLazy.end() );
		controlHmds[farm]->remove_last_request();
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
	hLearn["AEB"] = "euhmds";
	hLearn["BATECH"] = "euhmds";
	hLearn["BATEDE"] = "euhmds";
	hLearn["BATEEN"] = "euhmds";
	hLearn["BATEUK"] = "euhmds";
	hLearn["BELFOX"] = "euhmds";
	hLearn["BM"] = "euhmds";
	hLearn["BVL"] = "euhmds";
	hLearn["BVME"] = "euhmds";
	hLearn["CHIXCH"] = "euhmds";
	hLearn["CHIXDE"] = "euhmds";
	hLearn["CHIXEN"] = "euhmds";
	hLearn["CHIXUK"] = "euhmds";
	hLearn["DTB"] = "euhmds";
	hLearn["EBS"] = "euhmds";
	hLearn["EDXNO"] = "euhmds";
	hLearn["FTA"] = "euhmds";
	hLearn["FWB"] = "euhmds";
	hLearn["FWB2"] = "euhmds";
	hLearn["IBIS"] = "euhmds";
	hLearn["IBIS2"] = "euhmds";
	hLearn["IDEM"] = "euhmds";
	hLearn["IPE"] = "euhmds";
	hLearn["LIFFE_NF"] = "euhmds";
	hLearn["LIFFE"] = "euhmds";
	hLearn["LSE"] = "euhmds";
	hLearn["LSSF"] = "euhmds";
	hLearn["MATIF"] = "euhmds";
	hLearn["MEFFRV"] = "euhmds";
	hLearn["MONEP"] = "euhmds";
	hLearn["OMS"] = "euhmds";
	hLearn["SBF"] = "euhmds";
	hLearn["SBVM"] = "euhmds";
	hLearn["SFB"] = "euhmds";
	hLearn["SWB"] = "euhmds";
	hLearn["SWB2"] = "euhmds";
	hLearn["SOFFEX"] = "euhmds";
	hLearn["TGATE"] = "euhmds";
	hLearn["TRQXCH"] = "euhmds";
	hLearn["TRQXDE"] = "euhmds";
	hLearn["TRQXEN"] = "euhmds";
	hLearn["TRQXUK"] = "euhmds";
	hLearn["VIRTX"] = "euhmds";
	hLearn["VSE"] = "euhmds";

	hLearn["ECBOT"] = "ilhmds";
	hLearn["GLOBEX"] = "ilhmds";
	hLearn["NYMEX"] = "ilhmds";

	hLearn["BATS"] = "ushmds.us";
	hLearn["BYX"] = "ushmds.us";
	hLearn["CBOT"] = "ushmds.us";
	hLearn["CDE"] = "ushmds.us";
	hLearn["CFE"] = "ushmds.us";
	hLearn["CME"] = "ushmds.us";
	hLearn["DRCTEDGE"] = "ushmds.us";
	hLearn["EDGEA"] = "ushmds.us";
	hLearn["ELX"] = "ushmds.us";
	hLearn["IPE"] = "ushmds.us";
	hLearn["LAVA"] = "ushmds.us";
	hLearn["MEXDER"] = "ushmds.us";
	hLearn["NYBOT"] = "ushmds.us";
	hLearn["NYSELIFFE"] = "ushmds.us";
	hLearn["ONE"] = "ushmds.us";

	hLearn["ASX"] = "hkhmds";
	hLearn["CHIXJ"] = "hkhmds";
	hLearn["HKFE"] = "hkhmds";
	hLearn["HKMEX"] = "hkhmds";
	hLearn["KSE"] = "hkhmds";
	hLearn["NSE"] = "hkhmds";
	hLearn["OSE.JPN"] = "hkhmds";
	hLearn["SGX"] = "hkhmds";
	hLearn["SNFE"] = "hkhmds";
	hLearn["TSEJ"] = "hkhmds";
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
		/* FIXME this is a race, TWS tells us a farm is broken before sending
		   the last data. If this race happens in the other cases then we would
		   learn a wrong farm. The final fix would be to handle wrongly learned
		   farms when we notice it */
		DEBUG_PRINTF( "Warning, can't learn hmds while no farm is active.");
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




