#!/bin/bash

g++ -std=c++11 -o $1 -pthread -I. $1.cpp Semaphore.cpp
