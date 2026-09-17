# Peer-2-Peer (TCP Chat Application)

A multi-threaded C++ TCP chat system featuring direct messaging, room routing, binary file transfer, live network scanning, and JSON chat history logging.

---

## Features

* **Multi-Client Architecture**: Concurrent client handling using POSIX sockets and C++ `std::mutex` for thread-safe state management.
* **Direct Messaging (`/dm`)**: Send targeted private messages to specific connected clients.
* **Group Rooms (`/join` & `/group`)**: Create, join, and broadcast messages to custom group channels with dynamic room cleanup.
* **Binary File Transfer (`/file`)**: Transfer files up to 5MB encoded in Hex representation to prevent binary null-byte truncations (`\0`). Downloaded files are saved automatically in `bin/downloads/`.
* **Live Network Scanner (`/scan`)**: Ping all connected client IPs directly from the server to inspect online status.
* **Persistence & History**: Chat logs and session history are persistently recorded in JSON format under `bin/chat_history/`.
* **Dynamic Client Identification**: Unique UID assignment and automatic collision resolving (`User`, `User#1`, `User#2`).

---

## Technical Stack

* **Language**: C++17
* **Networking**: POSIX Sockets (`<sys/socket.h>`, `<netinet/in.h>`, `<arpa/inet.h>`)
* **Concurrency**: `std::mutex`, `std::lock_guard`, Multi-threading
* **Buffer Allocation**: 5MB Heap-allocated memory buffers for heavy data transactions
* **File System**: `std::filesystem` for dynamic directory and file generation