#!/bin/bash

apt-get clean -y 
apt-get update -y
apt-get upgrade -y
apt-get install -y build-essential
apt-get install -y libboost-all-dev
apt-get install -y libeigen3-dev
apt-get install -y make

