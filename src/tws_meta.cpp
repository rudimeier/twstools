#include "tws_meta.h"
#include "tws_xml.h"

#include "twsUtil.h"
#include "debug.h"

#include <QtCore/QDateTime>
#include <QtCore/QStringList>
#include <QtCore/QFile>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <limits.h>








qint64 nowInMsecs()
{
	const QDateTime now = QDateTime::currentDateTime();
	const qint64 now_s = now.toTime_t();
	const qint64 now_ms = now_s * 1000 + now.time().msec();
	return now_ms;
}


/// stupid static helper
QString ibDate2ISO( const QString &ibDate )
{
	QDateTime dt;
	
	dt = QDateTime::fromString( ibDate, "yyyyMMdd  hh:mm:ss");
	if( dt.isValid() ) {
		return dt.toString("yyyy-MM-dd hh:mm:ss");
	}
	
	dt.setDate( QDate::fromString( ibDate, "yyyyMMdd") );
	if( dt.isValid() ) {
		return dt.toString("yyyy-MM-dd");
	}
	
	bool ok = false;
	uint t = ibDate.toUInt( &ok );
	if( ok ) {
		dt.setTime_t( t );
		return dt.toString("yyyy-MM-dd hh:mm:ss");
	}
	
	return QString();
}


QHash<QString, const char*> init_short_wts()
{
	QHash<QString, const char*> ht;
	ht.insert("TRADES", "T");
	ht.insert("MIDPOINT", "M");
	ht.insert("BID", "B");
	ht.insert("ASK", "A");
	ht.insert("BID_ASK", "BA");
	ht.insert("HISTORICAL_VOLATILITY", "HV");
	ht.insert("OPTION_IMPLIED_VOLATILITY", "OIV");
	ht.insert("OPTION_VOLUME", "OV");
	return ht;
}

const QHash<QString, const char*> short_wts = init_short_wts();


QHash<QString, const char*> init_short_bar_size()
{
	QHash<QString, const char*> ht;
	ht.insert("1 secs",   "s01");
	ht.insert("5 secs",   "s05");
	ht.insert("15 secs",  "s15");
	ht.insert("30 secs",  "s30");
	ht.insert("1 min",    "m01");
	ht.insert("2 mins",   "m02");
	ht.insert("3 mins",   "m03");
	ht.insert("5 mins",   "m05");
	ht.insert("15 mins",  "m15");
	ht.insert("30 mins",  "m30");
	ht.insert("1 hour",   "h01");
	ht.insert("1 day",    "eod");
	ht.insert("1 week",   "w01");
	ht.insert("1 month",  "x01");
	ht.insert("3 months", "x03");
	ht.insert("1 year",   "y01");
	return ht;
}

const QHash<QString, const char*> short_bar_size = init_short_bar_size();








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


bool ContractDetailsRequest::fromStringList( const QList<QString>& sl,
	bool includeExpired )
{
	_ibContract.symbol = toIBString( sl[0] );
	_ibContract.secType = toIBString( sl[1]);
	// optional filter for exchange
	_ibContract.exchange= toIBString( sl.size() > 2 ? sl[2] : "" );
	// optional filter for a single expiry
	_ibContract.expiry = toIBString( sl.size() > 3 ? sl[3] : "" );
	_ibContract.includeExpired = includeExpired;
	return true;
}








bool HistRequest::initialize( const IB::Contract& c, const QString &e,
	const QString &d, const QString &b,
	const QString &w, int u, int f )
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


bool HistRequest::fromString( const QString& s, bool includeExpired )
{
	bool ok = false;
	QStringList sl = s.split('\t');
	
	if( sl.size() < 13 ) {
		return ok;
	}
	
	int i = 0;
	_endDateTime = sl.at(i++);
	_durationStr = sl.at(i++);
	_barSizeSetting = sl.at(i++);
	_whatToShow = sl.at(i++);
	_useRTH = sl.at(i++).toInt( &ok );
	if( !ok ) {
		return false;
	}
	_formatDate = sl.at(i++).toInt( &ok );
	if( !ok ) {
		return false;
	}
	
	_ibContract.symbol = toIBString(sl.at(i++));
	_ibContract.secType = toIBString(sl.at(i++));
	_ibContract.exchange = toIBString(sl.at(i++));
	_ibContract.currency = toIBString(sl.at(i++));
	_ibContract.expiry = toIBString(sl.at(i++));
	_ibContract.strike = sl.at(i++).toDouble( &ok );
	if( !ok ) {
		return false;
	}
	_ibContract.right = toIBString(sl.at(i++));
	_ibContract.includeExpired = includeExpired;
	
	return ok;
}


QString HistRequest::toString() const
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
		.arg(_endDateTime)
		.arg(_durationStr)
		.arg(_barSizeSetting)
		.arg(_whatToShow)
		.arg(_useRTH)
		.arg(_formatDate)
		.arg(c_str);
	
	return retVal;
}


void HistRequest::clear()
{
	_ibContract = IB::Contract();
	_whatToShow.clear();
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










HistTodo::HistTodo()
{
}


HistTodo::~HistTodo()
{
	foreach( HistRequest *hR, histRequests ) {
		delete hR;
	}
}


int HistTodo::fromFile( const QList<QByteArray> &rows, bool includeExpired )
{
	histRequests.clear();
	
	int retVal = rows.size();
	
	foreach( QByteArray row, rows ) {
		if( row.startsWith('[') ) {
			int firstTab = row.indexOf('\t');
			Q_ASSERT( row.size() > firstTab );
			Q_ASSERT( firstTab >= 0 ); //TODO
			row.remove(0, firstTab+1 );
		}
		HistRequest hR;
		bool ok = hR.fromString( row, includeExpired );
		Q_ASSERT(ok); //TODO
		add( hR );
	}
	return retVal;
}


void HistTodo::dump( FILE *stream ) const
{
	for(int i=0; i < histRequests.size(); i++ ) {
		fprintf( stream, "[%d]\t%s\n",
		         i,
		         histRequests.at(i)->toString().toUtf8().constData() );
	}
}


void HistTodo::dumpLeft( FILE *stream ) const
{
	for(int i=0; i < leftRequests.size(); i++ ) {
		fprintf( stream, "[%d]\t%s\n",
		         leftRequests[i],
		         histRequests.at(leftRequests[i])->toString().toUtf8().constData() );
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








int ContractDetailsTodo::fromFile( const QList<QByteArray> &rows,
	bool includeExpired )
{
	int retVal = 0;
	QRegExp regExp("^REQ_CD:?");
	
	foreach( QByteArray row, rows ) {
		QString s(row);
		Q_ASSERT( s.contains( regExp ) );
		s.remove( regExp );
		QList<QString> sl = s.trimmed().split(QRegExp("[ \t\r\n]*,[ \t\r\n]*"));
		Q_ASSERT( sl.size() >= 2 && sl.size() <= 4 ); // TODO handle that
		
		ContractDetailsRequest cdR;
		bool ok = cdR.fromStringList( sl, includeExpired );
		Q_ASSERT(ok); //TODO
		contractDetailsRequests.append( cdR );
		retVal++;
	}
	return retVal;
}








WorkTodo::WorkTodo() :
	reqType(GenericRequest::NONE),
	rows(new QList<QByteArray>())
{
}


WorkTodo::~WorkTodo()
{
	delete rows;
}


GenericRequest::ReqType WorkTodo::getType() const
{
	return reqType;
}


const QList<QByteArray>& WorkTodo::getRows() const
{
	return *rows;
}


int WorkTodo::read_file( const QString & fileName )
{
	int retVal = -1;
	QFile f( fileName );
	
	reqType = GenericRequest::NONE;
	rows->clear();


	TwsXml file;
	if( ! file.openFile(fileName.toAscii().constData()) ) {
		return retVal;
	}
	retVal = 0;
	xmlNodePtr xn;
	while( (xn = file.nextXmlNode()) != NULL ) {
		QByteArray line;
		if( xn->type == XML_ELEMENT_NODE
			&& strcmp((char*)xn->name, "PacketContractDetails") == 0 ) {
			reqType = GenericRequest::CONTRACT_DETAILS_REQUEST;
			PacketContractDetails *pcd = PacketContractDetails::fromXml(xn);
			const IB::Contract &c = pcd->getRequest().ibContract();
			line = QByteArray("REQ_CD: ")
				+ c.symbol.c_str() + ", "
				+ c.secType.c_str() + ", "
				+ c.exchange.c_str();
		} else if ( xn->type == XML_ELEMENT_NODE
			&& strcmp((char*)xn->name, "PacketHistData") == 0 ) {
			reqType = GenericRequest::HIST_REQUEST;
			Q_ASSERT(false);
		}
		rows->append(line);
		retVal++;
	}
	
	return retVal;
}








PacketContractDetails::PacketContractDetails()
{
	complete = false;
	reqId = -1;
	request = NULL;
}

PacketContractDetails::~PacketContractDetails()
{
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
	xmlDocPtr doc = xmlNewDoc( (const xmlChar*) "1.0");
	xmlNodePtr root = xmlNewDocNode( doc, NULL,
		(const xmlChar*)"PacketContractDetails", NULL );
	xmlDocSetRootElement( doc, root );
	
	xmlNodePtr nqry = xmlNewChild( root, NULL, (xmlChar*)"query", NULL);
	conv_ib2xml( nqry, "reqContract", request->ibContract(), TwsXml::skip_defaults );
	
	
	xmlNodePtr nrsp = xmlNewChild( root, NULL, (xmlChar*)"response", NULL);
	for( int i=0; i<cdList.size(); i++ ) {
		conv_ib2xml( nrsp, "ContractDetails", cdList[i], TwsXml::skip_defaults );
	}
	
	xmlDocFormatDump(stdout, doc, 1);
	//HACK print form feed as xml file separator
	printf("\f");
	
	xmlFreeDoc(doc);
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


PacketHistData::PacketHistData()
{
	mode = CLEAN;
	error = ERR_NONE;
	reqId = -1;
}


PacketHistData * PacketHistData::fromXml( xmlNodePtr )
{
	PacketHistData *phd = new PacketHistData();
	Q_ASSERT(false); // not implemented yet
	return phd;
}

#define ADD_ATTR_QSTRING( _ne_, _struct_, _attr_ ) \
	if( !TwsXml::skip_defaults || _struct_._attr_ != dflt._attr_ ) { \
		xmlNewProp ( _ne_, (xmlChar*) #_attr_, \
			(xmlChar*) toIBString(_struct_._attr_).c_str() ); \
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

void PacketHistData::dumpXml( const HistRequest& hR )
{
	char tmp[128];
	
	Q_ASSERT( mode == CLOSED && error == ERR_NONE );
	
	xmlDocPtr doc = xmlNewDoc( (const xmlChar*) "1.0");
	xmlNodePtr root = xmlNewDocNode( doc, NULL,
		(const xmlChar*)"PacketHistData", NULL );
	xmlDocSetRootElement( doc, root );
	{
		struct s_bla {
			const QString &endDateTime;
			const QString &durationStr;
			const QString &barSizeSetting;
			const QString &whatToShow;
			int useRTH;
			int formatDate;
		};
		static const s_bla dflt = {"", "", "", "", 0, 0 };
		const IB::Contract &c = hR.ibContract();
		s_bla bla = { hR.endDateTime(), hR.durationStr(), hR.barSizeSetting(),
			hR.whatToShow(), hR.useRTH(), hR.formatDate() };
		
		xmlNodePtr nqry = xmlNewChild( root, NULL, (xmlChar*)"query", NULL);
		conv_ib2xml( nqry, "reqContract", c, TwsXml::skip_defaults );
		ADD_ATTR_QSTRING( nqry, bla, endDateTime );
		ADD_ATTR_QSTRING( nqry, bla, durationStr );
		ADD_ATTR_QSTRING( nqry, bla, barSizeSetting );
		ADD_ATTR_QSTRING( nqry, bla, whatToShow );
		ADD_ATTR_INT( nqry, bla, useRTH );
		ADD_ATTR_INT( nqry, bla, formatDate );
	}
	
	xmlNodePtr nrsp = xmlNewChild( root, NULL, (xmlChar*)"response", NULL);
	{
		static const Row dflt = {"", -1.0, -1.0, -1.0, -1.0, -1, -1, -1.0, 0 };
		for( int i=0; i<rows.size(); i++ ) {
			xmlNodePtr nrow = xmlNewChild( nrsp, NULL, (xmlChar*)"row", NULL);
			ADD_ATTR_QSTRING( nrow, rows[i], date );
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
		ADD_ATTR_QSTRING( nrow, finishRow, date );
		ADD_ATTR_DOUBLE( nrow, finishRow, open );
		ADD_ATTR_DOUBLE( nrow, finishRow, high );
		ADD_ATTR_DOUBLE( nrow, finishRow, low );
		ADD_ATTR_DOUBLE( nrow, finishRow, close );
		ADD_ATTR_INT( nrow, finishRow, volume );
		ADD_ATTR_INT( nrow, finishRow, count );
		ADD_ATTR_DOUBLE( nrow, finishRow, WAP );
		ADD_ATTR_BOOL( nrow, finishRow, hasGaps );
	}
	
	xmlDocFormatDump(stdout, doc, 1);
	//HACK print form feed as xml file separator
	printf("\f");
	
	xmlFreeDoc(doc);
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
	rows.clear();
	finishRow.clear();
}


void PacketHistData::record( int reqId )
{
	Q_ASSERT( mode == CLEAN && error == ERR_NONE );
	mode = RECORD;
	this->reqId = reqId;
}


void PacketHistData::append( int reqId, const QString &date,
			double open, double high, double low, double close,
			int volume, int count, double WAP, bool hasGaps )
{
	Q_ASSERT( mode == RECORD && error == ERR_NONE );
	Q_ASSERT( this->reqId == reqId );
	
	Row row = { date, open, high, low, close,
		volume, count, WAP, hasGaps };
	
	if( date.startsWith("finished") ) {
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


void PacketHistData::dump( const HistRequest& hR, bool printFormatDates )
{
	Q_ASSERT( mode == CLOSED && error == ERR_NONE );
	const IB::Contract &c = hR.ibContract();
	const QString &wts = hR.whatToShow();
	const QString &barSizeSetting = hR.barSizeSetting();
	
	foreach( Row r, rows ) {
		QString expiry = toQString(c.expiry);
		QString dateTime = r.date;
		if( printFormatDates ) {
			if( expiry.isEmpty() ) {
				expiry = "0000-00-00";
			} else {
				expiry = ibDate2ISO( toQString(c.expiry) );
			}
			dateTime = ibDate2ISO(r.date);
			Q_ASSERT( !expiry.isEmpty() && !dateTime.isEmpty() ); //TODO
		}
		QString c_str = QString("%1\t%2\t%3\t%4\t%5\t%6\t%7")
			.arg(toQString(c.symbol))
			.arg(toQString(c.secType))
			.arg(toQString(c.exchange))
			.arg(toQString(c.currency))
			.arg(expiry)
			.arg(c.strike)
			.arg(toQString(c.right));
		printf("%s\t%s\t%s\t%s\t%f\t%f\t%f\t%f\t%d\t%d\t%f\t%d\n",
		       short_wts.value( wts, "NNN" ),
		       short_bar_size.value( barSizeSetting, "00N" ),
		       c_str.toUtf8().constData(),
		       dateTime.toUtf8().constData(),
		       r.open, r.high, r.low, r.close,
		       r.volume, r.count, r.WAP, r.hasGaps);
		fflush(stdout);
	}
}








PacingControl::PacingControl( int r, int i, int m, int v ) :
	maxRequests( r ),
	checkInterval( i ),
	minPacingTime( m ),
	violationPause( v )
{
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
		qint64 now = nowInMsecs();
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
	const qint64 now_t = nowInMsecs();
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
	const qint64 now = nowInMsecs();
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
		qint64 p_time = dateTimes.at( p_index );
		waitBurst = p_time + checkInterval - now;
	}
	SWAP_MAX( waitBurst, "wait burst" );
	
	*ddd = dbg;
	return retVal;
}

#undef SWAP


int PacingControl::countLeft() const
{
	const qint64 now = nowInMsecs();
	
	if( (dateTimes.size() > 0) && violations.last() ) {
		int waitViol = dateTimes.last() + violationPause - now;
		if( waitViol > 0 ) {
			return 0;
		}
	}
	
	int retVal = maxRequests;
	QList<qint64>::const_iterator it = dateTimes.constEnd();
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
	QList<qint64>::iterator t_d = dateTimes.begin();
	QList<bool>::iterator t_v = violations.begin();
	QList<qint64>::const_iterator o_d = other.dateTimes.constBegin();
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
		maxRequests, checkInterval, minPacingTime, violationPause)) )
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
	lastChanged(INT_MIN)
{
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


void DataFarmStates::notify(int msgNumber, int errorCode, const QString &msg)
{
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
	return mLearn[lazyC];
}


QString DataFarmStates::getHmdsFarm( const IB::Contract& c ) const
{
	QString lazyC = LAZY_CONTRACT_STR(c);
	return hLearn[lazyC];
}


QString DataFarmStates::getHmdsFarm( const QString& lazyC ) const
{
	return hLearn[lazyC];
}


#undef LAZY_CONTRACT_STR




