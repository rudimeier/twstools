#!/bin/sh

# change your host/port
TWSDO="twsdo -h localhost -p 7496"

# get some contract details
${TWSDO} --get-account > out_account.xml
${TWSDO} --get-exec > out_exec.xml


