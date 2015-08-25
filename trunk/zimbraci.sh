#! /bin/bash

cd $(dirname $(readlink -f $0))
./zimbraci &>/dev/null &
