# Low Latency Trading Ecosystem

## Contents
* [Overiew](#overview)
* [Architecture](#architecture)
* [Data Exchnage Patterns](#data-exchange-patterns)
* [Demo Run](#demo-run)
* [Usage](#usage)
* [References](#references)

## Overview
Trading Ecosystem is a comprehensive low-latency, high-throughput trading ecosystem developed in C++. It includes key components such as a matching engine, market data handlers, order gateways, and trading algorithms. Built from the barebones, this project doesn't rely on any external C++ libraries except STL. The effort is centered around gaining insights into designing and building high-performance trading systems from the ground up, leveraging C++ functionalities for low-latency development.

### Key Building Blocks
- **Lock-Free Queue**: Utilizes Single Producer Single Consumer lock-free queues for non-blocking data transfer between threads, minimizing latency by avoiding synchronization overhead.
- **Logging**: Logging framework is used to monitor the functioning of the ecosystem. The task is done by a background thread, preventing disk I/O from impacting performance-critical operations in the trading pipeline.
- **Multi-threading**: To achieve parallel processing and optimal resource utilization, each core component of the trading system operates within its own thread.
- **Memory Pool**: Custom memory pool is used to manage dynamic memory allocation and deallocation, tailored to the usage patterns of the trading components. Memory pool reduces the overhead associated with frequent allocations and garbage collection.

## Architecture

<br>
<p align="center">
<img src="data/Trading_Diagram.png"/>
</p>
<br>

## Data Exchange Patterns

### Client-Server Pattern
- **TCP Sockets for Communication**: For handling order requests and responses, TCP sockets are utilized to establish reliable communication between the Trading Exchange and Market Participant. The `OrderServer` class encapsulates a `TCPServer` object, while the `OrderGateway` class owns a `TCPSocket` instance.

- **Connection Initiation**: Each `TCPSocket` in the `OrderGateway` initiates a connection to the `TCPServer`, which listens on a pre-assigned port, ready to accept incoming connections from multiple market participants.

- **Connection Handling**: Upon accepting a connection, the `TCPServer` dynamically assigns a dedicated `TCPSocket` to manage communication with each market participant. This socket handles all subsequent data exchanges, ensuring that the server can manage multiple concurrent connections efficiently.

- **Communication Mechanism**: The data exchange between `TCPSocket` instances is managed using a combination of polling and callback functions. The `TCPServer` polls for incoming data, and upon detection, triggers the appropriate callback functions to process the requests and send responses.

### Publisher-Subscriber
- **UDP Sockets for Multicast Communication**: The `MarketDataPublisher` class uses two `McastSocket` objects to broadcast market updates to a multicast address, with separate streams for incremental updates and periodic snapshots.

- **Market Participant Subscription**: Each `MarketDataConsumer` owns `McastSocket` objects configured to subscribe to both the incremental and snapshot multicast streams, ensuring real-time updates and periodic state synchronization.

## Demo Run

<br>
<p align="center">
<img src="data/trial.gif" width="800" height="400"/> 
</p>
<br>

## Usage

**Dependencies:**

- **GCC:** g++ (Ubuntu 11.3.0-1ubuntu1~22.04.1) 11.3.0
- **CMake:** cmake version 3.23.2
- **Ninja:** 1.10.2

**Run:**

```sh
bash scripts/run_exchange_and_clients.sh
```

## References

- Ghosha, S. *Low-Latency C++: Optimization Techniques for High-Performance Systems*.

- Beej. *Beej's Guide to Network Programming*.

- Meyers, S. *Effective Modern C++: 42 Specific Ways to Improve Your Use of C++11 and C++14*. 

