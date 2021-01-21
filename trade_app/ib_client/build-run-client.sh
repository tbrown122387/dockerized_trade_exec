#!/bin/bash

cd IBJts/samples/Cpp/TestCppClient/
make clean
make
echo "SUCCESSFULLY BUILT IB CLIENT EXECUTION APP\n"
echo "NOW RUNNING THE EXECUTION APP\n"
./emini_trader
