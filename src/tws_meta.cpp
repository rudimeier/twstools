#include "tws_meta.h"

#include "twsapi/twsUtil.h"
#include "utilities/debug.h"

#include <QtCore/QDateTime>
#include <QtCore/QStringList>
#include <QtCore/QFile>

#include <limits.h>




namespace Test {




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







bool ContractDetailsRequest::initialize( const IB::Contract& c )
{
	ibContract = c;
	return true;
}


bool ContractDetailsRequest::fromStringList( const QList<QString>& sl )
{
	ibContract.symbol = toIBString( sl[0] );
	ibContract.secType = toIBString( sl[1]);
	ibContract.exchange= toIBString( sl[2] );
	// optional filter for a single expiry
	QString e = sl.size() > 3 ? sl[3] : "";
	ibContract.expiry = toIBString( e );
	return true;
}








bool HistRequest::initialize( const IB::Contract& c, const QString &e,
	const QString &d, const QString &b,
	const QString &w, int u, int f )
{
	ibContract = c;
	endDateTime = e;
	durationStr = d;
	barSizeSetting = b;
	whatToShow = w;
	useRTH = u;
	formatDate = f;
	return true;
}


bool HistRequest::fromString( const QString& s )
{
	bool ok = false;
	QStringList sl = s.split('\t');
	
	if( sl.size() < 13 ) {
		return ok;
	}
	
	int i = 0;
	endDateTime = sl.at(i++);
	durationStr = sl.at(i++);
	barSizeSetting = sl.at(i++);
	whatToShow = sl.at(i++);
	useRTH = sl.at(i++).toInt( &ok );
	if( !ok ) {
		return false;
	}
	formatDate = sl.at(i++).toInt( &ok );
	if( !ok ) {
		return false;
	}
	
	ibContract.symbol = toIBString(sl.at(i++));
	ibContract.secType = toIBString(sl.at(i++));
	ibContract.exchange = toIBString(sl.at(i++));
	ibContract.currency = toIBString(sl.at(i++));
	ibContract.expiry = toIBString(sl.at(i++));
	ibContract.strike = sl.at(i++).toDouble( &ok );
	if( !ok ) {
		return false;
	}
	ibContract.right = toIBString(sl.at(i++));
	
	return ok;
}


QString HistRequest::toString() const
{
	QString c_str = QString("%1\t%2\t%3\t%4\t%5\t%6\t%7")
		.arg(toQString(ibContract.symbol))
		.arg(toQString(ibContract.secType))
		.arg(toQString(ibContract.exchange))
		.arg(toQString(ibContract.currency))
		.arg(toQString(ibContract.expiry))
		.arg(ibContract.strike)
		.arg(toQString(ibContract.right));
	
	QString retVal = QString("%1\t%2\t%3\t%4\t%5\t%6\t%7")
		.arg(endDateTime)
		.arg(durationStr)
		.arg(barSizeSetting)
		.arg(whatToShow)
		.arg(useRTH)
		.arg(formatDate)
		.arg(c_str);
	
	return retVal;
}


void HistRequest::clear()
{
	ibContract = IB::Contract();
	whatToShow.clear();
}








GenericRequest::GenericRequest() :
	reqType(NONE),
	reqId(0)
{
}


void GenericRequest::nextRequest( ReqType t )
{
	reqType = t;
	reqId++;
}


void GenericRequest::close()
{
	reqType = NONE;
}










HistTodo::HistTodo() :
	curIndexTodoHistData(0)
{
}


HistTodo::~HistTodo()
{
	foreach( HistRequest *hR, histRequests ) {
		delete hR;
	}
}


int HistTodo::fromFile( const QString & fileName )
{
	histRequests.clear();
	
	QList<QByteArray> rows;
	int retVal = read_file( fileName, &rows );
	if( retVal == -1) {
		return retVal;
	}
	foreach( QByteArray row, rows ) {
		if( row.startsWith('[') ) {
			int firstTab = row.indexOf('\t');
			Q_ASSERT( row.size() > firstTab );
			Q_ASSERT( firstTab >= 0 ); //TODO
			row.remove(0, firstTab+1 );
		}
		HistRequest hR;
		bool ok = hR.fromString( row );
		Q_ASSERT(ok); //TODO
		add( hR );
	}
	return retVal;
}


int HistTodo::read_file( const QString & fileName, QList<QByteArray> *list ) const
{
	int retVal = -1;
	QFile f( fileName );
	if (f.open(QFile::ReadOnly)) {
		retVal = 0;
		while (!f.atEnd()) {
			QByteArray line = f.readLine();
			line.chop(1); //remove line feed
			if( line.startsWith('#') || line.isEmpty() ) {
				continue;
			}
			list->append(line);
			retVal++;
		}
	} else {
// 		_lastError = QString("can't read file '%1'").arg(fileName);
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


int HistTodo::currentIndex() const
{
	Q_ASSERT( curIndexTodoHistData > -1);
	return curIndexTodoHistData;
}


const HistRequest& HistTodo::current() const
{
	Q_ASSERT( curIndexTodoHistData > -1);
	Q_ASSERT( curIndexTodoHistData == leftRequests.first() );
	return *histRequests[curIndexTodoHistData];
}


void HistTodo::tellDone()
{
	Q_ASSERT( curIndexTodoHistData > -1);
	Q_ASSERT( curIndexTodoHistData == leftRequests.first() );
	leftRequests.removeFirst();
	doneRequests.append(curIndexTodoHistData);
	if( leftRequests.size() > 0 ) {
		curIndexTodoHistData = leftRequests.first();
	} else {
		curIndexTodoHistData = -1;
	}
}


void HistTodo::add( const HistRequest& hR )
{
	HistRequest *p = new HistRequest(hR);
	histRequests.append(p);
	leftRequests.append(histRequests.size() - 1);
}


void HistTodo::optimize( PacingGod *pG, const DataFarmStates *dfs)
{
	Q_ASSERT( curIndexTodoHistData > -1);
	QList<int> tmp;
	QHash< QString, QList<int> > h;
	foreach( int i ,leftRequests ) {
		QString farm = dfs->getHmdsFarm(histRequests.at(i)->ibContract);
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
		const IB::Contract& c = histRequests.at(l.first())->ibContract;
		int count = qMin( pG->countLeft( c ), l.size() );
		while( i < count) {
			qDebug() << "add farm" << i << farm;
			tmp.append(l.takeFirst());
			i++;
		}
	}
	foreach( QString farm, farms ) {
		int i = 0;
		Q_ASSERT( h.contains(farm) );
		QList<int> &l = h[farm];
		while( i < l.size() ) {
			qDebug() << "add rest" << i << farm;
			tmp.append(l.at(i));
			i++;
		}
	}
	
	
	qDebug() << tmp.size() << leftRequests.size();
	Q_ASSERT( tmp.size() == leftRequests.size() );
	leftRequests = tmp;
	curIndexTodoHistData = leftRequests.first();
}








int ContractDetailsTodo::fromConfig( const QList< QList<QString> > &contractSpecs )
{
	int retVal = contractSpecs.size();
	foreach( const QList<QString> &sl, contractSpecs ) {
		ContractDetailsRequest cdR;
		bool ok = cdR.fromStringList( sl );
		Q_ASSERT(ok); //TODO
		contractDetailsRequests.append( cdR );
	}
	return retVal;
}








PacketContractDetails::PacketContractDetails()
{
	complete = false;
	reqId = -1;
}


const QList<IB::ContractDetails>& PacketContractDetails::constList() const
{
	return cdList;
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
	reqId = -1;
	repeat = false;
}


bool PacketHistData::isFinished() const
{
	return (mode == CLOSED_SUCC) || (mode == CLOSED_ERR) ;
}


bool PacketHistData::needRepeat() const
{
	return repeat;
}


void PacketHistData::clear()
{
	mode = CLEAN;
	reqId = -1;
	repeat = false;
	rows.clear();
	finishRow.clear();
}


void PacketHistData::record( int reqId )
{
	Q_ASSERT( mode == CLEAN );
	mode = RECORD;
	this->reqId = reqId;
}


void PacketHistData::append( int reqId, const QString &date,
			double open, double high, double low, double close,
			int volume, int count, double WAP, bool hasGaps )
{
	Q_ASSERT( mode == RECORD);
	Q_ASSERT( this->reqId == reqId );
	
	Row row = { date, open, high, low, close,
		volume, count, WAP, hasGaps };
	
	if( date.startsWith("finished") ) {
		mode = CLOSED_SUCC;
		finishRow = row;
	} else {
		rows.append( row );
	}
}


void PacketHistData::closeError( bool repeat )
{
	Q_ASSERT( mode == RECORD);
	mode = CLOSED_ERR;
	this->repeat = repeat;
}


void PacketHistData::dump( const HistRequest& hR, bool printFormatDates )
{
	Q_ASSERT( mode == CLOSED_SUCC || ( mode == CLOSED_ERR && !repeat ) );
	const IB::Contract &c = hR.ibContract;
	const QString &wts = hR.whatToShow;
	const QString &barSizeSetting = hR.barSizeSetting;
	
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








quint64 PacingControl::nowInMsecs()
{
	const QDateTime now = QDateTime::currentDateTime();
	const quint64 now_t = (now.toTime_t() * 1000) + now.time().msec();
	return now_t;
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
		quint64 now = nowInMsecs();
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
	const quint64 now_t = nowInMsecs();
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

int PacingControl::goodTime() const
{
	const quint64 now = nowInMsecs();
	const char* dbg = "don't wait";
	int retVal = INT_MIN;
	
	if( dateTimes.isEmpty() ) {
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
		quint64 p_time = dateTimes.at( p_index );
		waitBurst = p_time + checkInterval - now;
	}
	SWAP_MAX( waitBurst, "wait burst" );
	
	qDebug() << dbg << retVal;
	return retVal;
}

#undef SWAP


int PacingControl::countLeft() const
{
	const quint64 now = nowInMsecs();
	
	if( (dateTimes.size() > 0) && violations.last() ) {
		int waitViol = dateTimes.last() + violationPause - now;
		if( waitViol > 0 ) {
			return 0;
		}
	}
	
	int retVal = maxRequests;
	QList<quint64>::const_iterator it = dateTimes.constEnd();
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
	QList<quint64>::iterator t_d = dateTimes.begin();
	QList<bool>::iterator t_v = violations.begin();
	QList<quint64>::const_iterator o_d = other.dateTimes.constBegin();
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
	QString farm;
	QString lazyC;
	checkAdd( c, &lazyC, &farm );
	bool laziesCleared = laziesAreCleared();
	
	if( farm.isEmpty() || !laziesCleared ) {
		// we have to use controlGlobal if any contract's farm is ambiguous
		qDebug() << "get good time global";
		Q_ASSERT( (controlLazy.contains(lazyC) && !controlHmds.contains(farm))
			|| !laziesCleared );
		return controlGlobal.goodTime();
	} else {
		qDebug() << "get good time farm" << farm ;
		Q_ASSERT( (controlHmds.contains(farm) && controlLazy.isEmpty())
			|| laziesCleared );
		return controlHmds.value(farm)->goodTime();
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
	QString *lazyC, QString *farm )
{
	*lazyC = LAZY_CONTRACT_STR(c);
	*farm = dataFarms.getHmdsFarm(c);
	
	if( !farm->isEmpty() ) {
		if( !controlHmds.contains(*farm) ) {
			PacingControl *pC;
			if( controlLazy.contains(*lazyC) ) {
				pC = controlLazy.take(*lazyC);
			} else {
				pC = new PacingControl(
					maxRequests, checkInterval, minPacingTime, violationPause);
			}
			controlHmds.insert( *farm, pC );
		} else {
			if( !controlLazy.contains(*lazyC) ) {
				// fine - no history about that
			} else {
				controlHmds.value(*farm)->merge(*controlLazy.take(*lazyC));
			}
		}
		Q_ASSERT( controlHmds.contains(*farm) );
		Q_ASSERT( !controlLazy.contains(*lazyC) );
		return;
	}
	
	if( !controlLazy.contains(*lazyC) ) {
		PacingControl *pC = new PacingControl(
			maxRequests, checkInterval, minPacingTime, violationPause);
		controlLazy.insert( *lazyC, pC );
	}
	
	Q_ASSERT( !controlHmds.contains(*farm) );
	Q_ASSERT( controlLazy.contains(*lazyC) );
}


bool PacingGod::laziesAreCleared() const
{
	
	bool retVal = true;
	foreach( PacingControl *pC, controlLazy ) {
		retVal &=  pC->isEmpty();
	}
	return retVal;
}








void DataFarmStates::notify(int errorCode, const QString &msg)
{
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
		farm = getFarm("never seen", msg); // will cause assert
		break;
	default:
		Q_ASSERT(false);
		return;
	}
	
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
			qDebug() << "HMDS farm unique:" << lazyC << sl.first();
		}
	} else {
		if( hLearn.contains(lazyC) ) {
			Q_ASSERT( sl.contains(hLearn.value(lazyC)) );
		} else {
			//but doing nothing
			qDebug() << "HMDS farm ambiguous:" << lazyC << sl;
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


#undef LAZY_CONTRACT_STR




} // namespace Test
