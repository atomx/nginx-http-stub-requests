#!/bin/sh

$NGINX -p $PWD -c server.conf &
sleep 1

curl -s -S -H "Host: example.com" "http://127.0.0.1:9090/sleep" 2>&1 > /dev/null &
sleep 0.5
echo all
curl -v -H "Host: example.com" "http://127.0.0.1:9090/requests" 2>&1 | grep '<td>'
sleep 0.5
echo only me
curl -v -H "Host: example.com" "http://127.0.0.1:9090/requests" 2>&1 | grep '<td>'

kill `cat test.pid`
sleep 1
