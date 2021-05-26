#!/bin/bash 
# 
# Copyright © Dougie Lawson, 2020-2021, All rights reserved 
#
NOW=$(date --iso-8601)
END_DATE=$(date --iso-8601 --date "+2 days")

/usr/local/bin/http --output /tmp/htariff.json GET  https://api.octopus.energy/v1/products/AGILE-18-02-21/electricity-tariffs/E-1R-AGILE-18-02-21-H/standard-unit-rates/ period_from=="${NOW}T00:00Z" period_to=="${END_DATE}T23:30Z" 

jq -r '.results[] | [ 
	(.valid_from|fromdate|strftime("%Y-%m-%d %H:%M")|tostring),
	(.valid_to|fromdate|strftime("%Y-%m-%d %H:%M")|tostring),
       	(.value_inc_vat|tostring) ] | join (",")' /tmp/htariff.json \
> /tmp/tariff.csv

sqlite3 -batch /home/pi/octopus/tariffs.db <<"SQLEND"
delete from agile_tariffs;
.mode csv
.import /tmp/tariff.csv agile_tariffs
.quit
SQLEND

rm /tmp/tariff.csv
rm /tmp/htariff.json
