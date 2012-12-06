mqtt-http-server
================
Toby Jaffey <toby@sensemote.com>

An MQTT to HTTP bridge, allowing web clients access to MQTT.
Supporting long polling, JSON, Javascript API, SSL.

More documentation is needed. For now, see the examples in htdocs.

Building
--------

Install dependencies

    sudo apt-get install libgnutls-dev ragel liburiparser-dev libmosquitto0-dev libev-dev

Build it

    make

Run it

    ./mqtthttpd -v -r htdocs -l 8080 -n foo -s test.mosquitto.org -p 1883

Point a web browser at:

    http://localhost:8080


HTTPS
-----

To run with HTTPS, provide a certificate and key, eg.

    ./mqtthttpd -v -r htdocs -l 443 -n foo -s test.mosquitto.org -p 1883 -c ./libebb/examples/ca-cert.pem -k ./libebb/examples/ca-key.pem


Using ports 80 and 443
----------------------

mqtthttpd should not be run as root. Install authbind (apt-get install authbind) to allow users to run services on low numbered ports.

    sudo apt-get install authbind
    sudo touch /etc/authbind/byport/80
    sudo chmod 755 /etc/authbind/byport/80
    sudo chown username.username /etc/authbind/byport/80
    ./mqtthttpd -v -r htdocs -l 80 -n foo -s test.mosquitto.org -p 1883


