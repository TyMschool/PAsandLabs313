#!/bin/bash

# Simple Performance Test Script
echo "Creating test files..."

# Create test files of different sizes
truncate -s 1K BIMDC/test1k.bin
truncate -s 10K BIMDC/test10k.bin  
truncate -s 100K BIMDC/test100k.bin
truncate -s 1M BIMDC/test1m.bin
truncate -s 10M BIMDC/test10m.bin
truncate -s 100M BIMDC/test100m.bin

echo "File Size | Time (seconds)"
echo "----------|---------------"

# Function to run test and extract time
run_test() {
    local file=$1
    local size=$2
    echo -n "$size:       "
    { time ./client -f $file > /dev/null 2>&1; } 2>&1 | grep real | awk '{print $2}'
}

# Run tests
run_test "test1k.bin" "1KB"
run_test "test10k.bin" "10KB"
run_test "test100k.bin" "100KB"
run_test "test1m.bin" "1MB"
run_test "test10m.bin" "10MB"
run_test "test100m.bin" "100MB"
