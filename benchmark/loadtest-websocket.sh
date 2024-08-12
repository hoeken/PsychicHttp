#!/usr/bin/env bash
#Command to install the testers:
# npm install

TEST_IP="psychic.local"
TEST_TIME=60
LOG_FILE=psychic-websocket-loadtest.json
RESULTS_FILE=websocket-loadtest-results.csv
PROTOCOL=ws
#PROTOCOL=wss

if test -f "$LOG_FILE"; then
  rm $LOG_FILE
fi

echo "url,clients,rps,latency,errors" > $RESULTS_FILE

for CONCURRENCY in 1 2 3 4 5 6 7 8 9 10 15 20
do
  echo "Testing $CONCURRENCY clients on $PROTOCOL://$TEST_IP/ws"
  loadtest -c $CONCURRENCY --cores 1 -t $TEST_TIME --insecure $PROTOCOL://$TEST_IP/ws --quiet  2> /dev/null >> $LOG_FILE
  node parse-websocket-test.js $LOG_FILE $RESULTS_FILE
  sleep 5
done

#for CONNECTIONS in 8 10 16 20
#for CONNECTIONS in 20
#do
#  CONCURRENCY=$((CONNECTIONS / 2))
#  echo "Testing $CONNECTIONS clients on $PROTOCOL://$TEST_IP/ws"
#  loadtest -c $CONCURRENCY --cores 1 -t $TEST_TIME --insecure $PROTOCOL://$TEST_IP/ws --quiet 2> /dev/null >> $LOG_FILE
#  node parse-websocket-test.js $LOG_FILE $RESULTS_FILE
#  sleep 5
#done
