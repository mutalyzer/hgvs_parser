#!/bin/bash

MEM_CHECK=''
EXIT_CODE=0

while getopts 'fm' option; do
    case "${option}" in
        f) EXIT_CODE=1 ;;
        m) MEM_CHECK='valgrind -q' ;;
    esac
done

FAIL=0;

while IFS= read -r line; do
    echo "HGVS variant description: '$line'"
    ${MEM_CHECK} ./a.out "$line"
    ret=$?
    if [ ${ret} -ne ${EXIT_CODE} ]; then
        FAIL=1
    fi
done

exit ${FAIL}
