#!/bin/bash

# start server
./server_v3 &
sleep 0.1

# start clients
for i in {1..1000}
do
    if (( $i > 1 && $i < 1000 )); then
        ./client $((1234+$i)) 1 > /dev/null &
    else
        ./client $((1234+$i)) &
    fi
done

sleep 15

# shutdown
killall -9 server_v3
