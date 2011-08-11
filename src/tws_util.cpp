/*** tws_util.cpp -- common utils
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

#include "tws_util.h"
#include "debug.h"

// from global installed twsapi
#include "twsapi/EWrapper.h" //IB::TickType
#include "twsapi/Execution.h"
#include "twsapi/Contract.h"

#include <string.h>
#include <sys/time.h>




int64_t nowInMsecs()
{
	timeval tv;
	int err = gettimeofday( &tv, NULL );
	assert( err == 0 );
	
	const int64_t now_ms = (int64_t)(tv.tv_sec*1000ULL) + (int64_t)(tv.tv_usec/1000ULL);
	return now_ms;
}

std::string msecs_to_string( int64_t msecs )
{
	const time_t s = msecs / 1000;
	const uint ms = msecs % 1000;
	char buf[ 19 + 4 + 1 ]; // "yyyy-mm-dd hh:mm:ss.zzz"
	const struct tm *tmp_tm = localtime( &s );
	size_t tmp_sz;
	
	tmp_sz = strftime(buf, 19+1 , "%F %T", tmp_tm);
	assert( tmp_sz == 19);
	tmp_sz = snprintf( buf+19, 4+1, ".%03u", ms );
	assert( tmp_sz == 4);
	
	return std::string(buf);
}


/**
 * Convert IB style date or date time string to struct tm.
 * Return 0 on success or -1 on error;
 */
int ib_strptime( struct tm *tm, const std::string &ib_datetime )
{
	char *tmp;
	
	memset(tm, 0, sizeof(struct tm));
	tmp = strptime( ib_datetime.c_str(), "%Y%m%d", tm);
	if( tmp != NULL && *tmp == '\0' ) {
		return 0;
	}
	
	memset(tm, 0, sizeof(struct tm));
	tmp = strptime( ib_datetime.c_str(), "%Y%m%d%t%H:%M:%S", tm);
	if(  tmp != NULL && *tmp == '\0' ) {
		return 0;
	}
	return -1;
}


std::string ibToString( int tickType) {
	switch ( tickType) {
		case IB::BID_SIZE:                    return "bidSize";
		case IB::BID:                         return "bidPrice";
		case IB::ASK:                         return "askPrice";
		case IB::ASK_SIZE:                    return "askSize";
		case IB::LAST:                        return "lastPrice";
		case IB::LAST_SIZE:                   return "lastSize";
		case IB::HIGH:                        return "high";
		case IB::LOW:                         return "low";
		case IB::VOLUME:                      return "volume";
		case IB::CLOSE:                       return "close";
		case IB::BID_OPTION_COMPUTATION:      return "bidOptComp";
		case IB::ASK_OPTION_COMPUTATION:      return "askOptComp";
		case IB::LAST_OPTION_COMPUTATION:     return "lastOptComp";
		case IB::MODEL_OPTION:                return "modelOptComp";
		case IB::OPEN:                        return "open";
		case IB::LOW_13_WEEK:                 return "13WeekLow";
		case IB::HIGH_13_WEEK:                return "13WeekHigh";
		case IB::LOW_26_WEEK:                 return "26WeekLow";
		case IB::HIGH_26_WEEK:                return "26WeekHigh";
		case IB::LOW_52_WEEK:                 return "52WeekLow";
		case IB::HIGH_52_WEEK:                return "52WeekHigh";
		case IB::AVG_VOLUME:                  return "AvgVolume";
		case IB::OPEN_INTEREST:               return "OpenInterest";
		case IB::OPTION_HISTORICAL_VOL:       return "OptionHistoricalVolatility";
		case IB::OPTION_IMPLIED_VOL:          return "OptionImpliedVolatility";
		case IB::OPTION_BID_EXCH:             return "OptionBidExchStr";
		case IB::OPTION_ASK_EXCH:             return "OptionAskExchStr";
		case IB::OPTION_CALL_OPEN_INTEREST:   return "OptionCallOpenInterest";
		case IB::OPTION_PUT_OPEN_INTEREST:    return "OptionPutOpenInterest";
		case IB::OPTION_CALL_VOLUME:          return "OptionCallVolume";
		case IB::OPTION_PUT_VOLUME:           return "OptionPutVolume";
		case IB::INDEX_FUTURE_PREMIUM:        return "IndexFuturePremium";
		case IB::BID_EXCH:                    return "bidExch";
		case IB::ASK_EXCH:                    return "askExch";
		case IB::AUCTION_VOLUME:              return "auctionVolume";
		case IB::AUCTION_PRICE:               return "auctionPrice";
		case IB::AUCTION_IMBALANCE:           return "auctionImbalance";
		case IB::MARK_PRICE:                  return "markPrice";
		case IB::BID_EFP_COMPUTATION:         return "bidEFP";
		case IB::ASK_EFP_COMPUTATION:         return "askEFP";
		case IB::LAST_EFP_COMPUTATION:        return "lastEFP";
		case IB::OPEN_EFP_COMPUTATION:        return "openEFP";
		case IB::HIGH_EFP_COMPUTATION:        return "highEFP";
		case IB::LOW_EFP_COMPUTATION:         return "lowEFP";
		case IB::CLOSE_EFP_COMPUTATION:       return "closeEFP";
		case IB::LAST_TIMESTAMP:              return "lastTimestamp";
		case IB::SHORTABLE:                   return "shortable";
		case IB::FUNDAMENTAL_RATIOS:          return "fundamentals";
		case IB::RT_VOLUME:                   return "rtVolume";
		case IB::HALTED:                      return "halted";
		default:                          return "unknown";
	}
}


std::string ibToString( const IB::Execution& ex )
{
	char buf[1024];
	snprintf( buf, sizeof(buf), "orderId:%ld: clientId:%ld, execId:%s, "
		"time:%s, acctNumber:%s, exchange:%s, side:%s, shares:%d, price:%g, "
		"permId:%d, liquidation:%d, cumQty:%d, avgPrice:%g",
		ex.orderId, ex.clientId, ex.execId.c_str(), ex.time.c_str(),
		ex.acctNumber.c_str(), ex.exchange.c_str(), ex.side.c_str(),
		ex.shares, ex.price, ex.permId, ex.liquidation, ex.cumQty, ex.avgPrice);
	return std::string(buf);
}


std::string ibToString( const IB::Contract &c, bool showFields )
{
	char buf[1024];
	const char *fmt;
	
	if( showFields ) {
		fmt = "conId:%ld, symbol:%s, secType:%s, expiry:%s, strike:%g, "
		"right:%s, multiplier:%s, exchange:%s, currency:%s, localSymbol:%s";
	} else {
		fmt = "%ld,%s,%s,%s,%g,%s,%s,%s,%s,%s";
	}
	snprintf( buf, sizeof(buf), fmt,
		c.conId, c.symbol.c_str(), c.secType.c_str(),
		c.expiry.c_str(), c.strike, c.right.c_str(),
		c.multiplier.c_str(), c.exchange.c_str(), c.currency.c_str(),
		c.localSymbol.c_str() );
	
	return std::string(buf);
}

