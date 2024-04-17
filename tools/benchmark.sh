#!/bin/bash


( ./benchmark 8080 100000) > "output_c-cpp.txt" 2>&1 &

( ./benchmark 8081 100000) > "output_z-cpp.txt" 2>&1 &

( ./benchmark 8082 100000) > "output_z-rust.txt" 2>&1 &


( ./benchmark 8083 100000) > "output_c-rust.txt" 2>&1 &

wait

echo "All benchmarks completed."

