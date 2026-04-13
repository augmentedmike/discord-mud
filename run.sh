#!/bin/bash

trap 'kill 0; exit' SIGINT SIGTERM

# kill anything on our ports and wait for release
for port in 4200 4201 4202; do
    pid=$(lsof -ti :$port 2>/dev/null)
    if [ -n "$pid" ]; then
        kill -9 $pid 2>/dev/null
    fi
done
sleep 2

# build server
make clean && make
if [ $? -ne 0 ]; then
    echo "Server build failed"
    exit 1
fi

# install client deps if needed
if [ ! -d client/node_modules ]; then
    (cd client && bun install)
fi

# start all three: server=4200, proxy=4201, client=4202
./world &
sleep 1
(cd client && bun mud-proxy.js) &
sleep 1
(cd client && bun run dev) &

echo ""
echo "MUD server:  localhost:4200"
echo "WS proxy:    localhost:4201"
echo "Web client:  http://localhost:4202"
echo ""

wait
