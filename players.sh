#! /bin/sh

MACHINE_NAME="vcm-181.vm.duke.edu"
PORT_NUM="4444"
PLAYER_NUM=$1

sleep 1 
for i in {1..2}
do
./player $MACHINE $PORT & 
done
wait
