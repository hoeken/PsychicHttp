#!/usr/bin/env bash
#Command to install the tester:
# npm install -i loadtest

TEST_IP="192.168.2.131"
TEST_TIME=21600
LOG_FILE=psychic-http-loadtest.log

if test -f "$LOG_FILE"; then
  rm $LOG_FILE
fi

echo "Testing http://$TEST_IP/"
loadtest -t $TEST_TIME --timeout 5000 http://$TEST_IP/ --quiet >> $LOG_FILE

echo "Testing http://$TEST_IP/api"
loadtest -t $TEST_TIME --timeout 5000 -P '{"foo":"bar"}' http://$TEST_IP/api --quiet >> $LOG_FILE

echo "Testing http://$TEST_IP/ws"
loadtest -t $TEST_TIME --timeout 5000 "ws://$TEST_IP/ws" --quiet 2> /dev/null >> $LOG_FILE

echo "Testing http://$TEST_IP/alien.png"
loadtest -t $TEST_TIME --timeout 5000 http://$TEST_IP/alien.png --quiet >> $LOG_FILE

#some basic test commands
# curl http://192.168.2.129/
# curl 'http://192.168.2.129/api?foo=bar'
# curl -d '{"foo":"bar"}' http://192.168.2.129/api