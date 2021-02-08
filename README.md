# AgileOctopus
Get Octopus Agile and draw a bar chart
======================================

This package needs the following pre-requisites:

libmysql-dev (or MariaDB equivalent)

https://github.com/json-c/json-c

libcurl-dev

cmake

[Chart.js](https://www.chartjs.org/docs/latest/getting-started/installation.html)

[JQuery](https://jquery.com/download/)


Code changes
------------

To configure this for your Raspberry Pi and your Octopus tariff.

* MySQL/MaridDB credentials in both getTariffs.c &amp; agile.tariff.c
  * Change `const char* d_host ="192.168.3.14";` to your MySQL server
  * Change `const char* d_db = "EV";` to your MySQL database name
  * Change `const char* d_user = "MySQLUser";` to your MySQL username
  * Change `const char* d_pwd = "MySQLPwd";` to your MySQL password
* Octopus Region ID in getTariffs.c
  * Change E-1R-AGILE-18-02-21-H to end with your region ID letter - See 
  [Distributor ID](https://en.wikipedia.org/wiki/Meter_Point_Administration_Number#Distributor_ID) 
  or [Octopus Developer API](https://octopus.energy/dashboard/developer/)


Installation
------------

```
git clone https://github.com/DougieLawson/AgileOctopus
cd AgileOctopus
mkdir build
cd build
cmake ..
make
mysql -h 192.168.3.14 -u MySQLUser -p  < Agile_tariffs.sql
```

Run `getTariffs` daily after 16:00 (when the new tariffs are available) probably with a crontab entry (or more than one as the tariffs are often late). I run it at 16:30, 17:30 and 22:30.

```
30 16 * * * /home/pi/octopus/getTariffs
30 17 * * * /home/pi/octopus/getTariffs
30 22 * * * /home/pi/octopus/getTariffs
```

Move `tariffs.html` to `/var/www/html`

Move `agile.tariff` to `/usr/lib/cgi-bin` (Apache2) or `/var/www/html/cgi-bin` (Lighty or NGinx)

