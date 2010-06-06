#include "tws_meta.h"

#include "twsapi/twsUtil.h"

#include <QtCore/QDateTime>
#include <QtCore/QStringList>
#include <QtCore/QFile>




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
		.arg(whatToShow)
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
	reqState(FINISHED),
	reqId(0)
{
}


void GenericRequest::nextRequest( ReqType t )
{
	Q_ASSERT( reqState == FINISHED );
	reqType = t;
	reqState = PENDING;
	reqId++;
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
		histRequests.append( hR );
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
		         histRequests.at(i).toString().toUtf8().constData() );
	}
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
	reqId = -1;
}


bool PacketHistData::isFinished() const
{
	return !finishRow.date.isEmpty();
}


void PacketHistData::clear()
{
	reqId = -1;
	rows.clear();
	finishRow.clear();
}


void PacketHistData::append( int reqId, const QString &date,
			double open, double high, double low, double close,
			int volume, int count, double WAP, bool hasGaps )
{
	if( rows.isEmpty() ) {
		this->reqId = reqId;
	}
	Q_ASSERT( this->reqId == reqId );
	
	Row row = { date, open, high, low, close,
		volume, count, WAP, hasGaps };
	
	if( date.startsWith("finished") ) {
		finishRow = row;
	} else {
		rows.append( row );
	}
}


void PacketHistData::dump( const HistRequest& hR, bool printFormatDates )
{
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




} // namespace Test
