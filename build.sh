#!/bin/sh

cmake -B./out -S./ && \
cd ./out && make && cd .. && \
exec out/KfIDE