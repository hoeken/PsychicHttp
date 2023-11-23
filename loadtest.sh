#install the tester
npm install -i loadtest

#load test on /
loadtest -t 3600 --timeout 5000 http://192.168.2.129/

#load test on /endpoint/api
loadtest -t 60 --timeout 5000 -P '{"foo":"bar"}' http://192.168.2.129/api
loadtest -t 60 --timeout 5000 'http://192.168.2.129/api?foo=bar'

#load test on /ws
loadtest -t 60 --timeout 5000 'ws://192.168.2.129/ws'

#some basic test commands
curl http://192.168.2.129/
curl 'http://192.168.2.129/api?foo=bar'
curl -d '{"foo":"bar"}' http://192.168.2.129/api

#loadtest -t 3600 --timeout 5000 http://192.168.2.129/
Target URL:          http://192.168.2.129/
Max time (s):        3600
Concurrent clients:  20
Running on cores:    2
Agent:               none

Completed requests:  157834
Total errors:        9
Total time:          3600.038 s
Mean latency:        455.3 ms
Effective rps:       44

Percentage of requests served within a certain time
  50%      150 ms
  90%      1153 ms
  95%      1212 ms
  99%      3255 ms
 100%      131129 ms (longest request)

   -1:   9 errors