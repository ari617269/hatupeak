# Hatupeak
A lightweight, file-based single-producer/single-consumer messaging system designed for telemetry, logging, IPC, and event streaming.
Hatupeak uses a binary file buffer with atomic file locking to coordinate safe read/write operations without external dependencies.

## 📌 Features

- Binary on-disk message queue
- Single Producer → Single Consumer
- Atomic file locking (flock)
- Minimal memory footprint
- No external services, perfect for local agents and embedded workflows

### 📁 Project Structure

```bash
records.txt   # shared binary log queue
data/         # directory containing message payload files
producer.cpp
consumer.cpp
build.sh
```

### 📥 Clone

```bash
git clone https://github.com/ari617269/hatupeak
cd hatupeak
```

### Build

```bash
chmod +x ./build.sh
./build.sh
```
- This compiles both producer and consumer.

### ▶ Run

- Producer

```bash
./producer
```
-- Example input session:
```bash
Enter text (or 'exit' to quit): hello world
Enter text (or 'exit' to quit): bye bye birdie
Enter text (or 'exit' to quit): silence
Enter text (or 'exit' to quit): exit
```

- Consumer

```bash
./consumer
```
-- Example output:
```bash
1738352958123.txt : 1 kib
1738352959123.txt : 3 kib
1764503601648.txt : 0 kib
1764503608056.txt : 0 kib
1764503615631.txt : 0 kib
 : 0 kib
```

## 📚 How It Works
Binary File Layout (records.txt)

Header (16 bytes):
```bash
uint64 readAddress
uint64 writeAddress
```

Records (variable):
```bash
uint32 length
char[length] payload   # filename
```

### Producer:

Writes {length, filename} at writeAddress

Updates writeAddress

Periodically compacts file to remove processed data

### Consumer:

Reads records sequentially from readAddress

Loads each payload file from /data

Updates readAddress