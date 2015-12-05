#! /bin/sh
./client -d tv -c $1
usleep 100000
./client -d tv -c $2
usleep 100000
