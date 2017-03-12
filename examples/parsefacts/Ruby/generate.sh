#!/bin/sh
protoc -I ../ --proto_path=../ --beefcake_out . ../facts.proto
