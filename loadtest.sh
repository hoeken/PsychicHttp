#!/usr/bin/env bash
#Command to install the testers:
# npm install -g loadtest
# npm install -g autocannon

TEST_IP="192.168.2.131"
TEST_TIME=10
LOG_FILE=psychic-http-loadtest.log
TIMEOUT=10000

if test -f "$LOG_FILE"; then
  rm $LOG_FILE
fi

for CONCURRENCY in 1 2 3 4 5 6 7 8 9 10 15 20
do
  echo "Testing $CONCURRENCY clients on http://$TEST_IP/"
  loadtest -c $CONCURRENCY --cores 1 -t $TEST_TIME --timeout $TIMEOUT "http://$TEST_IP/" --quiet >> $LOG_FILE
  #autocannon -c $CONCURRENCY -w 1 -d $TEST_TIME --renderStatusCodes "http://$TEST_IP/" >> $LOG_FILE 2>&1
  sleep 1

  # echo "Testing $CONCURRENCY clients on http://$TEST_IP/api"
  loadtest -c $CONCURRENCY --cores 1 -t $TEST_TIME --timeout $TIMEOUT "http://$TEST_IP/api?foo=bar" --quiet >> $LOG_FILE
  #autocannon -c $CONCURRENCY -w 1 -d $TEST_TIME --renderStatusCodes "http://$TEST_IP/api?foo=bar" >> $LOG_FILE 2>&1
  sleep 1

  # echo "Testing $CONCURRENCY clients on http://$TEST_IP/ws"
  loadtest -c $CONCURRENCY --cores 1 -t $TEST_TIME --timeout $TIMEOUT "ws://$TEST_IP/ws" --quiet 2> /dev/null >> $LOG_FILE
  sleep 1

  # echo "Testing $CONCURRENCY clients on http://$TEST_IP/alien.png single client"
  # loadtest -c $CONCURRENCY --cores 1 -t $TEST_TIME --timeout $TIMEOUT "http://$TEST_IP/alien.png" --quiet >> $LOG_FILE
  # autocannon -c $CONCURRENCY -w 1 -d $TEST_TIME --renderStatusCodes "http://$TEST_IP/alien.png" >> $LOG_FILE 2>&1
  # sleep 5
done

#some basic test commands:
# curl http://psychic.local/
# curl http://psychic.local/404
# curl http://psychic.local/alien.png
# curl 'http://psychic.local/api?foo=bar'
# curl -d '{"foo":"bar"}' http://psychic.local/api
# curl -i -X POST -T src/PsychicHttp.cpp 'http://psychic.local/upload'
# curl -i -X POST -T loadtest.sh 'http://psychic.local/upload?_filename=loadtest.sh'
# curl -i -X POST -T loadtest.sh 'http://psychic.local/upload/loadtest.sh'
# curl --data-urlencode "param1=Value (One)" http://psychic.local/post