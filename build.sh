#!/bin/sh

cmake -DKF_DEBUG:BOOL=true -B./out -S./ && \
cd ./out && make && cd .. && \
exec out/KfIDE