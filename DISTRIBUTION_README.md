# Data Processing Engine - Distribution Package

This package contains pre-built executables for Linux x86_64 systems.

## Contents
- `dist/data_processing_service` - Main processing engine executable
- `dist/test_all` - Integration test and performance validator
- `dist/tcp_receiver` - TCP test server
- `dist/udp_packet_generator` - Test data generator
- `test_data/` - Sample test vectors
- `run_full_pipeline.sh` - Complete test script

## Quick Start

1. Extract the package:
   ```bash
   tar -xzf data_processing_engine_linux.tar.gz
   cd data_processing_engine_linux/
   ```

2. Make executables accessible:
   ```bash
   chmod +x dist/*
   export PATH=$PWD/dist:$PATH
   ```

3. Run the complete test:
   ```bash
   ./run_full_pipeline.sh
   # Choose 'n' when asked to rebuild (binaries are pre-built)
   ```

## System Requirements
- Linux x86_64 (Ubuntu 18.04+ or equivalent)
- Python 3.6+ with pandas and matplotlib (for analytics)
- Network access for UDP multicast and TCP

## Manual Usage

Run the main service:
```bash
./dist/data_processing_service
```

Run performance tests:
```bash
./dist/test_all
```

Generate test data:
```bash
./dist/udp_packet_generator
```

## Note
These are statically-linked executables that should run on most modern Linux distributions without additional dependencies.