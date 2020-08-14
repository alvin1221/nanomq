## NanoMQ

Nano MQTT Broker - A light-weight and Blazing-fast MQTT 5.0 Broker for IoT Edge platform.

## Features

1. cost-effective on embedded platform.
2. Base on native POSIX Linux, high portability.
3. Full asynchronous I/O, good support for SMP.
4. Low latency & High handling capacity .





## Compile & Install

NanoMQ base on NNG's AIO threading model. Rewriting the TCP/SP part with self-added protocol nano_tcp.

Basically you need to compile and install the nng first:

$PROJECT_PATH/nanomq/build$ cmake -G Ninja .. 
or you can limit threads by
cmake -G Ninja -DNNG_RESOLV_CONCURRENCY=1 -DNNG_NUM_TASKQ_THREADS=2 -DNNG_MAX_TASKQ_THREADS=2  ..

$PROJECT_PATH/nanomq/build$ ninja 
$PROJECT_PATH/nanomq/build$ ninja install

then compile nanomq independently:
$PROJECT_PATH/nanomq/emq/nanomq/build$ cmake -G Ninja ..
$PROJECT_PATH/nanomq/emq/nanomq/build$ ninja

In the future, We will  implement a way to let nanomq support MQTT without damaging NNG's SP support.
Also rewrite CMake and MakeFile so that user can compile nng-nanomq version with nanomq together.

===============================================

usage:

#ongoing MQTT Broker
sudo ./nanomq broker start 'tcp://localhost:1883'

#test POSIX message Queue
sudo ./nanomq broker mq start/stop  

# Author

EMQ X Team.
