./clean.sh

DURATION=$((12 * 60 * 60))
NUM=$(echo "10 * 1000 * 400" | bc)
MEMORY=$((128 * 1024 * 1024))

# DURATION=5
# NUM_PER_THREAD=$(echo "10.24 * 1000 * 100 / 8" | bc)

./stress_static $DURATION $NUM $MEMORY & pid=$!

# 平均每个对象132bytes，10.24million个大概522M，每次缓存写入128M

echo "Monitoring process with PID: $pid"
pidstat -p $pid -u -r -d 1 $DURATION >> pidstat.log
# /usr/lib/linux-tools/5.4.0-198-generic/perf record -p $pid -g # -g flag is used to capture call graph
