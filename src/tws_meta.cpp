#include "tws_meta.h"
#include "tws_xml.h"

#include "twsUtil.h"
#include "debug.h"

#include <QtCore/QDateTime>
#include <QtCore/QStringList>
#include <QtCore/QHash>


#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <limits.h>








int64_t nowInMsecs()
{
	const QDateTime now = QDateTime::currentDateTime();
	const int64_t now_s = now.toTime_t();
	const int64_t now_ms = now_s * 1000 + now.time().msec();
	return now_ms;
}


/// stupid static helper
std::string ibDate2ISO( const std::string &ibDate )
{
	QDateTime dt;
	
	dt = QDateTime::fromString( toQString(ibDate), "yyyyMMdd  hh:mm:ss");
	if( dt.isValid() ) {
		return toIBString(dt.toString("yyyy-MM-dd hh:mm:ss"));
	}
	
	dt.setDate( QDate::fromString( toQString(ibDate), "yyyyMMdd") );
	if( dt.isValid() ) {
		return toIBString(dt.toString("yyyy-MM-dd"));
	}
	
	bool ok = false;
	uint t = toQString(ibDate).toUInt( &ok );
	if( ok ) {
		dt.setTime_t( t );
		return toIBString(dt.toString("yyyy-MM-dd hh:mm:ss"));
	}
	
	return std::string();
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
	QString c_str = QString("%1\t%2\t%3\t%4\t%5\t%6\t%7")
		.arg(toQString(_ibContract.symbol))
		.arg(toQString(_ibContract.secType))
		.arg(toQString(_ibContract.exchange))
		.arg(toQString(_ibContract.currency))
		.arg(toQString(_ibContract.expiry))
		.arg(_ibContract.strike)
		.arg(toQString(_ibContract.right));
	
	QString retVal = QString("%1\t%2\t%3\t%4\t%5\t%6\t%7")
		.arg(toQString(_endDateTime))
		.arg(toQString(_durationStr))
		.arg(toQString(_barSizeSetting))
		.arg(toQString(_whatToShow))
		.arg(_useRTH)
		.arg(_formatDate)
		.arg(c_str);
	
	return retVal.toStdString();
}


void HistRequest::clear()
{
	_ibContract = IB::Contract();
	_whatToShow.clear();
}



#define GET_ATTR_STRING( _struct_, _name_, _attr_ ) \
	tmp = (char*) xmlGetProp( node, (xmlChar*) _name_ ); \
	_struct_->_attr_ = tmp ? std::string(tmp) \
		: dflt._attr_; \
	free(tmp)

#define GET_ATTR_INT( _struct_, _name_, _attr_ ) \
	tmp = (char*) xmlGetProp( node, (xmlChar*) _name_ ); \
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
	histRequests(*(new QList<HistRequest*>())),
	doneRequests(*(new QList<int>())),
	leftRequests(*(new QList<int>())),
	errorRequests(*(new QList<int>())),
	checkedOutRequests(*(new QList<int>()))
{
}


HistTodo::~HistTodo()
{
	foreach( HistRequest *hR, histRequests ) {
		delete hR;
	}
	delete &histRequests;
	delete &doneRequests;
	delete &leftRequests;
	delete &errorRequests;
	delete &checkedOutRequests;
}


void HistTodo::dump( FILE *stream ) const
{
	for(int i=0; i < histRequests.size(); i++ ) {
		fprintf( stream, "[%d]\t%s\n",
		         i,
		         histRequests.at(i)->toString().c_str() );
	}
}


void HistTodo::dumpLeft( FILE *stream ) const
{
	for(int i=0; i < leftRequests.size(); i++ ) {
		fprintf( stream, "[%d]\t%s\n",
		         leftRequests[i],
		         histRequests.at(leftRequests[i])->toString().c_str() );
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
	Q_ASSERT( checkedOutRequests.isEmpty() );
	int id = leftRequests.takeFirst();
	checkedOutRequests.append(id);
}


int HistTodo::checkoutOpt( PacingGod *pG, const DataFarmStates *dfs )
{
	Q_ASSERT( checkedOutRequests.isEmpty() );
	
	QHash< QString, int > hashByFarm;
	QHash<QString, int> countByFarm;
	foreach( int i ,leftRequests ) {
		QString farm = dfs->getHmdsFarm(histRequests.at(i)->ibContract());
		if( !hashByFarm.contains(farm) ) {
			hashByFarm.insert(farm, i);
			countByFarm.insert(farm, 1);
		} else {
			countByFarm[farm]++;
		}
	}
	
	QStringList farms = hashByFarm.keys();
	
	int todoId = leftRequests.first();
	int countTodo = 0;
	foreach( QString farm, farms ) {
		Q_ASSERT( hashByFarm.contains(farm) );
		int tmpid = hashByFarm[farm];
		Q_ASSERT( histRequests.size() > tmpid );
		const IB::Contract& c = histRequests.at(tmpid)->ibContract();
		if( pG->countLeft( c ) > 0 ) {
			if( farm.isEmpty() ) {
				// 1. the unknown ones to learn farm quickly
				todoId = tmpid;
				break;
			} else if( countTodo < countByFarm[farm] ) {
				// 2. get from them biggest list
				todoId = tmpid;
				countTodo = countByFarm[farm];
			}
		}
	}
	
	int i = leftRequests.indexOf( todoId );
	Q_ASSERT( i>=0 );
	int wait = pG->goodTime( histRequests[todoId]->ibContract() );
	if( wait <= 0 ) {
		leftRequests.removeAt( i );
		checkedOutRequests.append(todoId);
	}
	
	return wait;
}


void HistTodo::cancelForRepeat( int priority )
{
	Q_ASSERT( checkedOutRequests.size() == 1 );
	int id = checkedOutRequests.takeFirst();
	if( priority <= 0 ) {
		leftRequests.prepend(id);
	} else if( priority <=1 ) {
		leftRequests.append(id);
	} else {
		errorRequests.append(id);
	}
}


int HistTodo::currentIndex() const
{
	Q_ASSERT( !checkedOutRequests.isEmpty() );
	return checkedOutRequests.first();
}


const HistRequest& HistTodo::current() const
{
	Q_ASSERT( !checkedOutRequests.isEmpty() );
	return *histRequests[checkedOutRequests.first()];
}


void HistTodo::tellDone()
{
	Q_ASSERT( !checkedOutRequests.isEmpty() );
	int doneId = checkedOutRequests.takeFirst();
	doneRequests.append(doneId);
}


void HistTodo::add( const HistRequest& hR )
{
	HistRequest *p = new HistRequest(hR);
	histRequests.append(p);
	leftRequests.append(histRequests.size() - 1);
}


void HistTodo::optimize( PacingGod *pG, const DataFarmStates *dfs)
{
	Q_ASSERT( checkedOutRequests.isEmpty() );
	
	QList<int> tmp;
	QHash< QString, QList<int> > h;
	foreach( int i ,leftRequests ) {
		QString farm = dfs->getHmdsFarm(histRequests.at(i)->ibContract());
		if( !h.contains(farm) ) {
			h.insert(farm, QList<int>());
		}
		h[farm].append(i);
	}
	
	QStringList farms = h.keys();
	if( farms.removeOne("") ) {
		farms.prepend("");
	}
	
	foreach( QString farm, farms ) {
		int i = 0;
		Q_ASSERT( h.contains(farm) );
		QList<int> &l = h[farm];
		Q_ASSERT( l.size() > 0 );
		Q_ASSERT( histRequests.size() > l.first() );
		const IB::Contract& c = histRequests.at(l.first())->ibContract();
		int count = qMin( pG->countLeft( c ), l.size() );
		while( i < count) {
			tmp.append(l.takeFirst());
			i++;
		}
	}
	foreach( QString farm, farms ) {
		int i = 0;
		Q_ASSERT( h.contains(farm) );
		QList<int> &l = h[farm];
		while( i < l.size() ) {
			tmp.append(l.at(i));
			i++;
		}
	}
	
	
	qDebug() << tmp.size() << leftRequests.size();
	Q_ASSERT( tmp.size() == leftRequests.size() );
	leftRequests = tmp;
}








ContractDetailsTodo::ContractDetailsTodo() :
	contractDetailsRequests(*(new QList<ContractDetailsRequest>()))
{
}

ContractDetailsTodo::~ContractDetailsTodo()
{
	delete &contractDetailsRequests;
}








WorkTodo::WorkTodo() :
	reqType(GenericRequest::NONE),
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


GenericRequest::ReqType WorkTodo::getType() const
{
	return reqType;
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


int WorkTodo::read_file( const std::string & fileName )
{
	int retVal = -1;
	reqType = GenericRequest::NONE;
	
	TwsXml file;
	if( ! file.openFile(fileName.c_str()) ) {
		return retVal;
	}
	retVal = 0;
	xmlNodePtr xn;
	while( (xn = file.nextXmlNode()) != NULL ) {
		QByteArray line;
		Q_ASSERT( xn->type == XML_ELEMENT_NODE  );
		if( strcmp((char*)xn->name, "PacketContractDetails") == 0 ) {
			reqType = GenericRequest::CONTRACT_DETAILS_REQUEST;
			PacketContractDetails *pcd = PacketContractDetails::fromXml(xn);
			_contractDetailsTodo->contractDetailsRequests.append(pcd->getRequest());
		} else if ( strcmp((char*)xn->name, "PacketHistData") == 0 ) {
			reqType = GenericRequest::HIST_REQUEST;
			PacketHistData *phd = PacketHistData::fromXml(xn);
			_histTodo->add( phd->getRequest() );
		} else {
			fprintf(stderr, "Warning, unknown request tag '%s' ignored.\n",
				xn->name );
			continue;
		}
		retVal++;
	}
	
	return retVal;
}








PacketContractDetails::PacketContractDetails() :
	cdList(*(new QList<IB::ContractDetails>()))
{
	complete = false;
	reqId = -1;
	request = NULL;
}

PacketContractDetails::~PacketContractDetails()
{
	delete &cdList;
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
						pcd->cdList.append(cd);
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

const QList<IB::ContractDetails>& PacketContractDetails::constList() const
{
	return cdList;
}

void PacketContractDetails::record( int reqId,
	const ContractDetailsRequest& cdr )
{
	Q_ASSERT( !complete && this->reqId == -1 && request == NULL
		&& cdList.isEmpty() );
	this->reqId = reqId;
	this->request = new ContractDetailsRequest( cdr );
}

void PacketContractDetails::setFinished()
{
	Q_ASSERT( !complete );
	complete = true;
}


bool PacketContractDetails::isFinished() const
{
	return complete;
}


void PacketContractDetails::clear()
{
	complete = false;
	reqId = -1;
	cdList.clear();
	if( request != NULL ) {
		delete request;
		request = NULL;
	}
}


void PacketContractDetails::append( int reqId, const IB::ContractDetails& c )
{
	if( cdList.isEmpty() ) {
		this->reqId = reqId;
	}
	Q_ASSERT( this->reqId == reqId );
	Q_ASSERT( !complete );
	
	cdList.append(c);
}


void PacketContractDetails::dumpXml()
{
	xmlNodePtr root = TwsXml::newDocRoot();
	xmlNodePtr npcd = xmlNewChild( root, NULL,
		(const xmlChar*)"PacketContractDetails", NULL );
	
	xmlNodePtr nqry = xmlNewChild( npcd, NULL, (xmlChar*)"query", NULL);
	conv_ib2xml( nqry, "reqContract", request->ibContract() );
	
	xmlNodePtr nrsp = xmlNewChild( npcd, NULL, (xmlChar*)"response", NULL);
	for( int i=0; i<cdList.size(); i++ ) {
		conv_ib2xml( nrsp, "ContractDetails", cdList[i] );
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


PacketHistData::PacketHistData() :
		rows(*(new QList<Row>()))
{
	mode = CLEAN;
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
					if( q->type == XML_ELEMENT_NODE
						&& strcmp((char*)q->name, "row???????") == 0 )  {
						// TODO
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
		(const xmlChar*)"PacketHistData", NULL );
	
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
		for( int i=0; i<rows.size(); i++ ) {
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

bool PacketHistData::isFinished() const
{
	return (mode == CLOSED);
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
	Q_ASSERT( mode == CLEAN && error == ERR_NONE && request == NULL );
	mode = RECORD;
	this->reqId = reqId;
	this->request = new HistRequest( hR );
}


void PacketHistData::append( int reqId, const std::string &date,
			double open, double high, double low, double close,
			int volume, int count, double WAP, bool hasGaps )
{
	Q_ASSERT( mode == RECORD && error == ERR_NONE );
	Q_ASSERT( this->reqId == reqId );
	
	Row row = { date, open, high, low, close,
		volume, count, WAP, hasGaps };
	
	if( strncmp(date.c_str(), "finished", 8) == 0) {
		mode = CLOSED;
		finishRow = row;
	} else {
		rows.append( row );
	}
}


void PacketHistData::closeError( Error e )
{
	Q_ASSERT( mode == RECORD);
	mode = CLOSED;
	error = e;
}


void PacketHistData::dump( bool printFormatDates )
{
	Q_ASSERT( mode == CLOSED && error == ERR_NONE );
	const IB::Contract &c = request->ibContract();
	const char *wts = short_wts( request->whatToShow().c_str() );
	const char *bss = short_bar_size( request->barSizeSetting().c_str());
	
	foreach( Row r, rows ) {
		std::string expiry = c.expiry;
		std::string dateTime = r.date;
		if( printFormatDates ) {
			if( expiry.empty() ) {
				expiry = "0000-00-00";
			} else {
				expiry = ibDate2ISO( c.expiry );
			}
			dateTime = ibDate2ISO(r.date);
			Q_ASSERT( !expiry.empty() && !dateTime.empty() ); //TODO
		}
		QString c_str = QString("%1\t%2\t%3\t%4\t%5\t%6\t%7")
			.arg(toQString(c.symbol))
			.arg(toQString(c.secType))
			.arg(toQString(c.exchange))
			.arg(toQString(c.currency))
			.arg(toQString(expiry))
			.arg(c.strike)
			.arg(toQString(c.right));
		printf("%s\t%s\t%s\t%s\t%f\t%f\t%f\t%f\t%d\t%d\t%f\t%d\n",
		       wts,
		       bss,
		       c_str.toUtf8().constData(),
		       dateTime.c_str(),
		       r.open, r.high, r.low, r.close,
		       r.volume, r.count, r.WAP, r.hasGaps);
		fflush(stdout);
	}
}








PacingControl::PacingControl( int r, int i, int m, int v ) :
	dateTimes(*(new QList<int64_t>())),
	violations(*(new QList<bool>())),
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
	return dateTimes.isEmpty();
}


void PacingControl::clear()
{
	if( !dateTimes.isEmpty() ) {
		int64_t now = nowInMsecs();
		if( now - dateTimes.last() < 5000  ) {
			// HACK race condition might cause assert in notifyViolation(),
			// to avoid this we would need to ack each request
			qDebug() << "Warning, keep last pacing date time "
				"because it looks too new.";
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
	dateTimes.append( now_t );
	violations.append( false );
}


void PacingControl::notifyViolation()
{
	Q_ASSERT( !violations.isEmpty() );
	violations.last() = true;
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
	
	if( dateTimes.isEmpty() ) {
		*ddd = dbg;
		return retVal;
	}
	
	
	int waitMin = dateTimes.last() + minPacingTime - now;
	SWAP_MAX( waitMin, "wait min" );
	
// 	int waitAvg =  dateTimes.last() + avgPacingTime - now;
// 	SWAP_MAX( waitAvg, "wait avg" );
	
	int waitViol = violations.last() ?
		(dateTimes.last() + violationPause - now) : INT_MIN;
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
	
	if( (dateTimes.size() > 0) && violations.last() ) {
		int waitViol = dateTimes.last() + violationPause - now;
		if( waitViol > 0 ) {
			return 0;
		}
	}
	
	int retVal = maxRequests;
	QList<int64_t>::const_iterator it = dateTimes.constEnd();
	while( it != dateTimes.constBegin() ) {
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
	qDebug() << dateTimes;
	qDebug() << other.dateTimes;
	QList<int64_t>::iterator t_d = dateTimes.begin();
	QList<bool>::iterator t_v = violations.begin();
	QList<int64_t>::const_iterator o_d = other.dateTimes.constBegin();
	QList<bool>::const_iterator o_v = other.violations.constBegin();
	
	while( t_d != dateTimes.end() && o_d != other.dateTimes.constEnd() ) {
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
	while( o_d != other.dateTimes.constEnd() ) {
		Q_ASSERT( t_d == dateTimes.end() );
		t_d = dateTimes.insert( t_d, *o_d );
		t_v = violations.insert( t_v, *o_v );
		t_d++;
		t_v++;
		o_d++;
		o_v++;
	}
	qDebug() << dateTimes;
}








#define LAZY_CONTRACT_STR( _c_ ) \
	toQString(_c_.exchange) + QString("\t") + toQString(_c_.secType);


PacingGod::PacingGod( const DataFarmStates &dfs ) :
	dataFarms( dfs ),
	maxRequests( 60 ),
	checkInterval( 601000 ),
	minPacingTime( 1500 ),
	violationPause( 60000 ),
	controlGlobal( *(new PacingControl(
		maxRequests, checkInterval, minPacingTime, violationPause)) ),
	controlHmds(*(new QHash<const QString, PacingControl*>()) ),
	controlLazy(*(new QHash<const QString, PacingControl*>()) )
{
}


PacingGod::~PacingGod()
{
	delete &controlGlobal;
	
	QHash<const QString, PacingControl*>::iterator it;
	it = controlHmds.begin();
	while( it != controlHmds.end() ) {
		delete *it;
		it = controlHmds.erase(it);
	}
	it = controlLazy.begin();
	while( it != controlLazy.end() ) {
		delete *it;
		it = controlLazy.erase(it);
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
	foreach( PacingControl *pC, controlHmds ) {
		pC->setPacingTime( r, i, m );
	}
	foreach( PacingControl *pC, controlLazy ) {
		pC->setPacingTime( r, i, m  );
	}
}


void PacingGod::setViolationPause( int vP )
{
	violationPause = vP;
	controlGlobal.setViolationPause( vP );
	foreach( PacingControl *pC, controlHmds ) {
		pC->setViolationPause( vP );
	}
	foreach( PacingControl *pC, controlLazy ) {
		pC->setViolationPause( vP );
	}
}


void PacingGod::clear()
{
	if( dataFarms.getActives().isEmpty() ) {
		// clear all PacingControls
		qDebug() << "clear all pacing controls";
		controlGlobal.clear();
		foreach( PacingControl *pC, controlHmds ) {
			pC->clear();
		}
		foreach( PacingControl *pC, controlLazy ) {
			pC->clear();
		}
	} else {
		// clear only PacingControls of inactive farms
		foreach( QString farm, dataFarms.getInactives() ) {
			if( controlHmds.contains(farm) ) {
				qDebug() << "clear pacing control of inactive farm" << farm;
				controlHmds.value(farm)->clear();
			}
		}
	}
}


void PacingGod::addRequest( const IB::Contract& c )
{
	QString farm;
	QString lazyC;
	checkAdd( c, &lazyC, &farm );
	
	controlGlobal.addRequest();
	
	if( farm.isEmpty() ) {
		qDebug() << "add request lazy";
		Q_ASSERT( controlLazy.contains(lazyC) && !controlHmds.contains(farm) );
		controlLazy[lazyC]->addRequest();
	} else {
		qDebug() << "add request farm" << farm;
		Q_ASSERT( controlHmds.contains(farm) && !controlLazy.contains(lazyC) );
		controlHmds[farm]->addRequest();
	}
}


void PacingGod::notifyViolation( const IB::Contract& c )
{
	QString farm;
	QString lazyC;
	checkAdd( c, &lazyC, &farm );
	
	controlGlobal.notifyViolation();
	
	if( farm.isEmpty() ) {
		qDebug() << "set violation lazy";
		Q_ASSERT( controlLazy.contains(lazyC) && !controlHmds.contains(farm) );
		controlLazy[lazyC]->notifyViolation();
	} else {
		qDebug() << "set violation farm" << farm;
		Q_ASSERT( controlHmds.contains(farm) && !controlLazy.contains(lazyC) );
		controlHmds[farm]->notifyViolation();
	}
}


int PacingGod::goodTime( const IB::Contract& c )
{
	const char* dbg;
	QString farm;
	QString lazyC;
	checkAdd( c, &lazyC, &farm );
	bool laziesCleared = laziesAreCleared();
	
	if( farm.isEmpty() || !laziesCleared ) {
		// we have to use controlGlobal if any contract's farm is ambiguous
		Q_ASSERT( (controlLazy.contains(lazyC) && !controlHmds.contains(farm))
			|| !laziesCleared );
		int t = controlGlobal.goodTime(&dbg);
		qDebug() << "get good time global" << dbg << t;
		return t;
	} else {
		Q_ASSERT( (controlHmds.contains(farm) && controlLazy.isEmpty())
			|| laziesCleared );
		int t = controlHmds.value(farm)->goodTime(&dbg);
		qDebug() << "get good time farm" << farm << dbg << t;
		return t;
	}
}


int PacingGod::countLeft( const IB::Contract& c )
{
	QString farm;
	QString lazyC;
	checkAdd( c, &lazyC, &farm );
	bool laziesCleared = laziesAreCleared();
	
	if( farm.isEmpty() || !laziesCleared ) {
		// we have to use controlGlobal if any contract's farm is ambiguous
		Q_ASSERT( (controlLazy.contains(lazyC) && !controlHmds.contains(farm))
			|| !laziesCleared );
		int left = controlGlobal.countLeft();
		qDebug() << "get count left global" << left;
		return left;
	} else {
		Q_ASSERT( (controlHmds.contains(farm) && controlLazy.isEmpty())
			|| laziesCleared );
		int left = controlHmds.value(farm)->countLeft();
		qDebug() << "get count left farm" << farm << left;
		return controlHmds.value(farm)->countLeft();
	}
}



void PacingGod::checkAdd( const IB::Contract& c,
	QString *lazyC_, QString *farm_ )
{
	*lazyC_ = LAZY_CONTRACT_STR(c);
	*farm_ = dataFarms.getHmdsFarm(c);
	
	// controlLazy.keys() does not work for QHash<const QString, PacingControl*>
	QStringList lazies;
	QHash<const QString, PacingControl*>::const_iterator it =
		controlLazy.constBegin();
	while( it != controlLazy.constEnd() ) {
		lazies.append( it.key() );
		it++;
	}
	if( !lazies.contains(*lazyC_) ) {
		lazies.append(*lazyC_);
	}
	
	foreach( QString lazyC, lazies ) {
	QString farm = dataFarms.getHmdsFarm(lazyC);
	if( !farm.isEmpty() ) {
		if( !controlHmds.contains(farm) ) {
			PacingControl *pC;
			if( controlLazy.contains(lazyC) ) {
				qDebug() << "move pacing control lazy to farm"
					<< lazyC << farm;
				pC = controlLazy.take(lazyC);
			} else {
				qDebug() << "create pacing control for farm" << farm;
				pC = new PacingControl(
					maxRequests, checkInterval, minPacingTime, violationPause);
			}
			controlHmds.insert( farm, pC );
		} else {
			if( !controlLazy.contains(lazyC) ) {
				// fine - no history about that
			} else {
				qDebug() << "merge pacing control lazy into farm"
					<< lazyC << farm;
				PacingControl *pC = controlLazy.take(lazyC);
				controlHmds.value(farm)->merge(*pC);
				delete pC;
			}
		}
		Q_ASSERT( controlHmds.contains(farm) );
		Q_ASSERT( !controlLazy.contains(lazyC) );
		
	} else if( !controlLazy.contains(lazyC) ) {
			qDebug() << "create pacing control for lazy" << lazyC;
			PacingControl *pC = new PacingControl(
				maxRequests, checkInterval, minPacingTime, violationPause);
			controlLazy.insert( lazyC, pC );
			
			Q_ASSERT( !controlHmds.contains(farm) );
			Q_ASSERT( controlLazy.contains(lazyC) );
	}
	}
}


bool PacingGod::laziesAreCleared() const
{
	
	bool retVal = true;
	foreach( PacingControl *pC, controlLazy ) {
		retVal &=  pC->isEmpty();
	}
	return retVal;
}








DataFarmStates::DataFarmStates() :
	mStates( *(new QHash<const QString, State>()) ),
	hStates( *(new QHash<const QString, State>()) ),
	mLearn( *(new QHash<const QString, QString>()) ),
	hLearn( *(new QHash<const QString, QString>()) ),
	lastMsgNumber(INT_MIN)
{
}

DataFarmStates::~DataFarmStates()
{
	delete &hLearn;
	delete &mLearn;
	delete &hStates;
	delete &mStates;
}

void DataFarmStates::setAllBroken()
{
	QHash<const QString, State>::iterator it;
	
	it = mStates.begin();
	while( it != mStates.end() ) {
		*it = BROKEN;
		it++;
	}
	
	it = hStates.begin();
	while( it != hStates.end() ) {
		*it = BROKEN;
		it++;
	}
}


void DataFarmStates::notify(int msgNumber, int errorCode,
	const std::string &_msg)
{
	QString msg = toQString(_msg); // just convert to QString
	lastMsgNumber = msgNumber;
	QString farm;
	State state;
	QHash<const QString, State> *pHash = NULL;
	
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
		Q_ASSERT(false);
		return;
	}
	
	lastChanged = farm;
	pHash->insert( farm, state );
	qDebug() << *pHash;
}


/// static member
QString DataFarmStates::getFarm( const QString prefix, const QString& msg )
{
	Q_ASSERT( msg.startsWith(prefix, Qt::CaseInsensitive) );
	
	return msg.right( msg.size() - prefix.size() );
}


void DataFarmStates::learnMarket( const IB::Contract& )
{
	Q_ASSERT( false ); //not implemented
}


void DataFarmStates::learnHmds( const IB::Contract& c )
{
	QString lazyC = LAZY_CONTRACT_STR(c);
	
	QStringList sl;
	QHash<const QString, State>::const_iterator it = hStates.constBegin();
	while( it != hStates.constEnd() ) {
		if( *it == OK ) {
			sl.append( it.key() );
		}
		it++;
	}
	if( sl.size() <= 0 ) {
		Q_ASSERT(false); // assuming at least one farm must be active
	} else if( sl.size() == 1 ) {
		if( hLearn.contains(lazyC) ) {
			Q_ASSERT( hLearn.value( lazyC ) == sl.first() );
		} else {
			hLearn.insert( lazyC, sl.first() );
			qDebug() << "learn HMDS farm (unique):" << lazyC << sl.first();
		}
	} else {
		if( hLearn.contains(lazyC) ) {
			Q_ASSERT( sl.contains(hLearn.value(lazyC)) );
		} else {
			//but doing nothing
			qDebug() << "learn HMDS farm (ambiguous):" << lazyC << sl;
		}
	}
}


void DataFarmStates::learnHmdsLastOk(int msgNumber, const IB::Contract& c )
{
	Q_ASSERT( !lastChanged.isEmpty() && hStates.contains(lastChanged) );
	if( (msgNumber == (lastMsgNumber + 1)) && (hStates[lastChanged] == OK) ) {
		QString lazyC = LAZY_CONTRACT_STR(c);
		if( hLearn.contains(lazyC) ) {
			Q_ASSERT( hLearn.value( lazyC ) == lastChanged );
		} else {
			hLearn.insert( lazyC, lastChanged );
			qDebug() << "learn HMDS farm (last ok):" << lazyC << lastChanged;
		}
	}
}


QStringList DataFarmStates::getInactives() const
{
	QStringList sl;
	QHash<const QString, State>::const_iterator it = hStates.constBegin();
	while( it != hStates.constEnd() ) {
		if( *it == INACTIVE || *it == BROKEN ) {
			sl.append( it.key() );
		}
		it++;
	}
	return sl;
}


QStringList DataFarmStates::getActives() const
{
	QStringList sl;
	QHash<const QString, State>::const_iterator it = hStates.constBegin();
	while( it != hStates.constEnd() ) {
		if( *it == OK ) {
			sl.append( it.key() );
		}
		it++;
	}
	return sl;
}


QString DataFarmStates::getMarketFarm( const IB::Contract& c ) const
{
	QString lazyC = LAZY_CONTRACT_STR(c);
	return mLearn.value(lazyC);
}


QString DataFarmStates::getHmdsFarm( const IB::Contract& c ) const
{
	QString lazyC = LAZY_CONTRACT_STR(c);
	return hLearn.value(lazyC);
}


QString DataFarmStates::getHmdsFarm( const QString& lazyC ) const
{
	return hLearn.value(lazyC);
}


#undef LAZY_CONTRACT_STR




