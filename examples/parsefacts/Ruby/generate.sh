#!/bin/sh
protoc -I ../C++ --proto_path=../ --beefcake_out . ../C++/facts.proto
