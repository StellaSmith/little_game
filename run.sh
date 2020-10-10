#!/bin/sh

GAME_DIR=$(dirname $(readlink -f $0))

LIB_DIR=$GAME_DIR/lib
BIN_DIR=$GAME_DIR/bin

LD_LIBRARY_PATH=$LIB_DIR:$BIN_DIR:$LD_LIBRARY_PATH
PATH=$BIN_DIR:$PATH

cd $GAME_DIR
export LD_LIBRARY_PATH PATH

exec little_game
