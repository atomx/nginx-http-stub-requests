#!/bin/bash

$NGINX -p $PWD -c server.conf &
sleep 1

send() {
  while true; do
    curl -s -S -H "Host: example.com" "http://127.0.0.1:9090/sleep" > /dev/null &
    sleep 0.01
  done
}

send &
send_pid=$!

control_c() {
  kill -9 $send_pid
  kill `cat test.pid`
  sleep 1
  exit 0
}

trap control_c SIGINT

while true; do
  read x;
  curl -H "Host: example.com" "http://127.0.0.1:9090/requests" 2>&1 | grep '<td>'
done

