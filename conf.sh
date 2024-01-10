#!/bin/bash
SYSTEM="leonardo"

case $SYSTEM in
  leonardo)
    source conf/leonardo.sh
    ;;

  *)
    echo -n "Unknown SYSTEM "$SYSTEM
    ;;
esac