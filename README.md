# nanomq.io

NanoMQ WebSite

## Navigator

```
Logo                                                 Github
```

## Slogan

Title: Nano MQTT Broker

Headline: A light-weight and Blazing-fast MQTT 5.0 Broker for IoT Edge platform.

Buttion: Get the Code [link to github.com/nanomq/nanomq/]

TODO: @feng Background Image

## Features

1. Cost-effective on embedded platform.
2. Base on native POSIX Linux, high portability with embedded platform.
3. Full asynchronous I/O, good support for SMP.
4. Low latency & High handling capacity .

## QuickStart

1. Compile & Install

NanoMQ is base on NNG's asynchronous I/O threading model. We rewriting the TCP/SP part with self-added protocol: nano_tcp.

Basically you just need to simply compile and install nanomq by:

$PROJECT_PATH/nanomq$ mkdir build & cd build

$PROJECT_PATH/nanomq/build$ cmake -G Ninja .. 

$PROJECT_PATH/nanomq/build$ sudo ninja install

or you can limit threads by
cmake -G Ninja -DNNG_RESOLV_CONCURRENCY=1 -DNNG_NUM_TASKQ_THREADS=5 -DNNG_MAX_TASKQ_THREADS=5  ..

Please be awared that nanomq depends on nanolib & nng (MQTT ver)

both dependencies can be complied independently

$PROJECT_PATH/nanomq/nng/build$ cmake -G Ninja .. 
$PROJECT_PATH/nanomq/nng/build$ ninja install

compile nanolib independently:
$PROJECT_PATH/nanolib/build$ cmake -G Ninja ..
$PROJECT_PATH/nanolib/build$ ninja install

In the future, We will  implement a way to let nanomq support MQTT without damaging NNG's SP support.
Also rewrite CMake and MakeFile so that user can easily choose which ver of nng to base on.

TODO:

more features coming

===============================================

2. Usage:

#ongoing MQTT Broker
sudo ./nanomq broker start 'tcp://localhost:1883'

#test POSIX message Queue
sudo ./nanomq broker mq start/stop  

## Communties

You can Join us on Slack channel:

#nanomq-dev : for MQTT lover & developer

#nanomq-nng : for user & nng project.



More communities on github, slack, reddit, twitter, gitter, discord are coming soon.

## Subscribe News

Subscribe: [       ]

## Footer

TODO: @crazywisdom
