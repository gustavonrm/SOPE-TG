#!/bin/bash
./user 0 "123456789" 100 0 "2 25 top_secret" &
./user 0 "123456789" 100 0 "1 25 top_secret" &
./user 2 "top_secret" 100 2 "1 10" & 
./user 1 "top_secret" 100 1 "" &
./user 0 "123456789" 100 0 "1 25 top_secret" &
./user 0 "123456789" 100 0 "3 25 top_secret" &
./user 3 "123456789" 100 0 "4 30 top_secret" &
./user 1 "top_secret" 100 0 "4 30 top_seret" &
./user 0 "123456789" 100 3 "" &