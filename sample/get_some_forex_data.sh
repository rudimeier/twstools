#!/bin/sh

# change your host/port
TWSDO="twsdo -h localhost -p 7496 --maxRequests=30 --pacingInterval=305000"
WORK_CONTRACTS="sample_job_contracts_forex.xml"

# get some contract details
${TWSDO} ${WORK_CONTRACTS}  > out_forex_contracts.xml   \
	|| exit 1

# create historical data requests for these contracts
twsgen -H -b "15 mins" -w "BID_ASK" out_forex_contracts.xml  > out_forex_work_hist.xml  \
	|| exit 1

# process the hist data job
${TWSDO} out_forex_work_hist.xml  > out_forex_hist.xml  \
	|| exit 1


# convert to csv and sort traded and quotes into different files
twsgen -C  out_forex_hist.xml  |grep "^BA" | sort |uniq > out_forex_quotes.csv

