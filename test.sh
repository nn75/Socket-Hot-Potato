#! /bin/bash                                                                                                                  
pkill -9 ringmaster
pkill -9 player

make clean
git pull
make

RINGMASTER_HOSTNAME=vcm-8252.vm.duke.edu
RINGMASTER_PORT=40000
NUM_PLAYERS=200
NUM_HOPS=512

./ringmaster $RINGMASTER_PORT $NUM_PLAYERS $NUM_HOPS &

sleep 2

for (( i=0; i<$NUM_PLAYERS; i++ ))
do
    ./player $RINGMASTER_HOSTNAME $RINGMASTER_PORT &
done

wait

