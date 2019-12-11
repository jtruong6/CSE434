#!/bin/sh

sudo apt-get install build-essential -y
g++ udp_server.cpp -o udp_server