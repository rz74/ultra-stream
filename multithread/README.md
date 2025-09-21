# Multithreaded Data Processing Service

This folder contains a multithreaded implementation of the data processing service. This version uses **Boost.Asio** for asynchronous networking and **multi-threading** with worker thread pools to achieve high-performance, low-latency data processing.

## Key Technologies
- **Boost.Asio**: Asynchronous UDP/TCP networking
- **Multi-threading**: Worker thread pools for parallel processing
- **Thread-safe data structures**: Mutex-protected data books
- **C++20**: Modern C++ features for performance optimization

## Folder Structure

```
multithread/
├── DataProcessingService.cpp              # Main multithreaded implementation
├── DataProcessingService.h                # Header file
├── DataProcessingService                  # Compiled executable
├── DataProcessingService_Portable.cpp     # Alternative implementation
├── MicroPriceService.cpp                  # Original service code
├── CMakeLists.txt                         # Build configuration
├── CMakePresets.json                      # CMake presets
├── CMakeSettings.json                     # Build settings
├── build_multithreaded_distribution.sh    # Distribution package builder
├── test_multithreaded_distribution.sh     # Comprehensive test suite
├── test_data_processing.sh               # Focused test script
├── multithreaded_data_processing_engine_linux.tar.gz     # Distribution package
├── multithreaded_data_processing_engine_linux.tar.gz.sha256  # Package checksum
├── test_run.log                          # Test execution log
└── .vs/                                  # Visual Studio files
```

## Usage

To run the service:
```bash
./DataProcessingService <multicast_address> <port>
```

Example:
```bash
./DataProcessingService 239.255.0.1 12345
```

## Testing

Run the comprehensive test suite:
```bash
./test_multithreaded_distribution.sh
```

Or run the focused data processing test:
```bash
./test_data_processing.sh
```