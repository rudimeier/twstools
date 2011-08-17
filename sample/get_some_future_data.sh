#!/bin/sh

# change your host/port
TWSDO="twsdo -h localhost -p 7496"
WORK_CONTRACTS="sample_job_contracts_future.xml"

# get some contract details
${TWSDO} ${WORK_CONTRACTS}  > out_fut_contracts.xml   \
	|| exit 1

# create historical data requests for these contracts
twsgen -H -b "30 mins" -w "TRADES,BID_ASK" out_fut_contracts.xml  > out_fut_work_hist.xml  \
	|| exit 1

# process the hist data job
${TWSDO} out_fut_work_hist.xml  > out_fut_hist.xml  \
	|| exit 1


# convert to csv and sort traded and quotes into different files
twsgen -C  out_fut_hist.xml  |grep "^T" | sort |uniq > out_fut_trades.csv
twsgen -C  out_fut_hist.xml  |grep "^BA" | sort |uniq > out_fut_quotes.csv

