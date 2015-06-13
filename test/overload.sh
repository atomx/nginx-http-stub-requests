#!/bin/sh

$NGINX -p $PWD -c server.conf &
sleep 1

for i in `seq 1 10`; do
  curl -s -S -H "Host: example.com" "http://127.0.0.1:9090/requests" > /dev/null &
done

curl -H "Host: example.com" "http://127.0.0.1:9090/requests" 2>&1 | grep '<td>'

kill `cat test.pid`
sleep 1
