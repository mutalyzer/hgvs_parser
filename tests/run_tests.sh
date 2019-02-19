#!/bin/bash

while IFS= read -r line; do
    echo "$line"
    ./a.out "$line"
done
