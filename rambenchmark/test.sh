output=$(./rambench | grep "MEMTESTRESULT")

MEMTESTRESULT=$(echo $output | cut -d '=' -f 2)
export MEMTESTRESULT

echo $MEMTESTRESULT
