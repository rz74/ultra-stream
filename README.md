# High-Performance Data Processing Engine – Project Overview

This project implements a **low-latency UDP-to-TCP data processing engine** designed for real-time streaming data applications. The service consumes UDP multicast data streams, maintains multi-level data books per subject, calculates composite scores using configurable algorithms, and forwards processed results via TCP. Network byte order is adopted system-wide for optimal performance.

---

## Key Components

### CompositeScoreCalculator
The `CompositeScoreCalculator` is a configurable computational engine that processes multi-level data updates. It performs weighted calculations over demand/supply levels to generate composite scores. The algorithm is designed to be:
- **Fast**: Sub-millisecond processing per message
- **Scalable**: Handles multiple data subjects simultaneously  
- **Generic**: Adaptable to various data processing scenarios

**Note**: The current implementation uses a simplified scoring algorithm for demonstration purposes. In production use cases, this would be replaced with domain-specific calculations tailored to your application requirements.

### DataBook Management
The `DataBook` system maintains real-time state for each data subject:
- **Multi-level tracking**: Up to 10 levels of demand/supply data
- **Efficient updates**: Delta-based processing for minimal overhead
- **Memory optimized**: Compact data structures for cache efficiency

---

## Project Structure

```
ultra-stream/
├── CMakeLists.txt                    # CMake build configuration for the entire project
├── LICENSE                          # Project license
├── README.md                        # This file - comprehensive project documentation
├── run_full_pipeline.sh             # Master script to build, generate test vectors, and run full performance test
├── build_distribution.sh            # Script to create distribution package
├── test_distribution.sh             # Test script for distribution package
├── test_full_integration.sh         # Full integration test script
├── data_processing_engine_linux.tar.gz # Pre-built distribution package
├── DISTRIBUTION_README.md           # Distribution-specific documentation
├── .vscode/                         # VSCode project settings (optional)
├── build/                           # Generated during build (excluded from repository)
│   ├── bin/                         # Compiled executables
│   │   ├── data_processing_service  # Main executable
│   │   ├── tcp_receiver             # Test TCP server
│   │   ├── test_all                 # Integration test runner
│   │   └── udp_packet_generator     # Test data generator
│   └── ...                          # CMake build artifacts
├── include/
│   ├── composite_score_calculator.hpp # CompositeScoreCalculator for data aggregation algorithms
│   ├── data_book.hpp                # DataBook classes to manage multi-level data state per subject
│   ├── logger.hpp                   # High-performance logging with microsecond timestamps
│   ├── parser_utils.hpp             # Binary protocol parsing and endianness utilities
│   ├── process_packet_core.hpp      # Core pipeline: parse message → update data → calculate → transmit
│   ├── tcp_sender.hpp               # TcpSender class for reliable result transmission
│   ├── types.hpp                    # Shared type definitions: DataLevel, ProcessedMessage, CompositeScoreMessage
│   └── udp_receiver.hpp             # UdpReceiver for high-efficiency multicast data ingestion
├── src/
│   ├── composite_score_calculator.cpp # Implementation of configurable scoring algorithms
│   ├── data_book.cpp                # Implements DataBook update logic and demand/supply state management
│   ├── logger.cpp                   # Implementation of microsecond-precision logging
│   ├── main.cpp                     # Application entry: sets up multicast ingestion, data processing, TCP output
│   ├── parser_utils.cpp             # Binary message parsing with network byte order handling
│   ├── tcp_sender.cpp               # TCP socket management and message transmission
│   └── udp_receiver.cpp             # Multicast receive loop with user-provided callback dispatch
├── test/
│   ├── evaluate_results.py          # Python analytics engine: generates performance dashboard from timing data
│   ├── tcp_receiver.cpp             # TCP test server to capture and validate processed outputs
│   ├── test_all.cpp                 # Comprehensive integration test with latency measurement
│   └── udp_packet_generator.cpp     # Load test generator: produces realistic UDP data streams
├── test_data/
│   ├── input_packets.bin            # Binary test data stream for system validation
│   ├── input_packets.csv            # Human-readable representation of test data
│   └── input_summary.csv            # Test case summary with expected processing behavior
├── test_results/
│   ├── latency_report.html          # Interactive performance dashboard with histograms and statistics
│   ├── latency_trace.csv            # Microsecond-precision timing data for each processed message
│   ├── tcp_sent.csv                 # Actual output messages transmitted via TCP (validation data)
│   └── test_all.log                 # Complete test execution log with performance metrics
└── tools/
    ├── generate_test_vectors.cpp    # Test data generator: creates realistic data patterns for validation
    └── udp_generator.cpp            # Alternative UDP injection utility for custom test scenarios
```

**Note**: The `build/` and `logs/` directories are generated during compilation and testing. They are excluded from the repository via `.gitignore` to prevent exposing local system paths.

---

### Testing and Performance Validation
The system includes comprehensive performance testing:
- **Latency measurement**: Microsecond-precision timing throughout the pipeline
- **Load testing**: Configurable packet generation with realistic data patterns
- **Performance reporting**: Automated HTML dashboard generation
- **Stress testing**: High-frequency data stream simulation

### Environment Requirements
All tests were run under WSL with cross-platform compatibility.

> If you encounter errors such as `bad interpreter: No such file or directory`, it's likely due to Windows-style line endings (`^M`). To fix this, you can run:
>
> ```bash
> dos2unix run_full_pipeline.sh
> ```

This conversion solves the issue of scripts not executing properly in WSL environments.

### Performance Analytics
A comprehensive **performance report** is generated using the Python analytics script `evaluate_results.py` in the `test/` directory. The resulting analysis includes:
- **Latency distributions** per message complexity
- **Throughput metrics** across different load patterns  
- **Performance histograms** and statistical summaries
- **Interactive dashboard** saved as `latency_report.html`

---

## Architecture Highlights

### Network Processing
- **UDP Multicast Input**: High-efficiency packet reception with minimal overhead
- **TCP Output Streaming**: Reliable delivery of processed results
- **Zero-copy optimizations**: Direct memory access patterns where possible

### Data Pipeline
1. **UDP Reception**: Raw packet capture and validation
2. **Message Parsing**: Binary protocol decoding with endianness handling
3. **Data Book Updates**: Real-time state management per subject
4. **Composite Calculation**: Configurable scoring algorithms
5. **TCP Transmission**: Formatted result delivery

### Performance Characteristics
- **Latency**: Sub-millisecond end-to-end processing
- **Throughput**: 1000+ messages/second sustained
- **Memory**: Efficient data structures with minimal allocation
- **CPU**: Optimized algorithms with cache-friendly access patterns

### Performance Benchmarks & Context

**Test Environment:**
- **Hardware**: Microsoft Surface Book 2
- **CPU**: Intel Core i7-8650U @ 1.90GHz (4 cores, 8 threads, up to 2.11GHz boost)
- **RAM**: 16GB DDR4-1867 (2x8GB dual channel)
- **OS**: Windows 11 with WSL2 (Ubuntu 22.04)
- **WSL Kernel**: 6.6.87.2-microsoft-standard-WSL2

**Performance Notes:**
- Results are hardware-dependent and will vary on different systems
- The composite score calculator uses simplified algorithms as a demonstration
- Actual throughput depends on network conditions, system load, and hardware configuration
- Sub-millisecond latency achieved under optimal conditions with minimal system load

**Typical Results on Test System:**
- **Message Processing**: ~99.8% success rate (998/1000 messages)
- **End-to-End Latency**: Sub-millisecond for simple calculations
- **Sustained Throughput**: 1000+ messages/second

---

## Build and Execution

### Quick Start

**Two Options Available:**
1. **Pre-Built Distribution** (Recommended for quick evaluation): Skip to [Pre-Built Distribution](#pre-built-distribution) section
2. **Build from Source** (For development and customization): Continue below

**Prerequisites**: This project uses Linux-specific networking libraries and requires a Linux environment. On Windows, use WSL (Windows Subsystem for Linux).

#### On Linux or WSL:
```bash
# Ensure required tools are installed
sudo apt update
sudo apt install build-essential cmake python3 python3-pip

# Install Python dependencies
pip3 install pandas matplotlib

# Fix line endings (important for WSL users)
dos2unix run_full_pipeline.sh

# Make script executable
chmod +x run_full_pipeline.sh

# Clean any existing build cache (prevents CMake path conflicts)
rm -rf build/

# Build and run complete test suite
./run_full_pipeline.sh

# Note: The script will prompt if you want to rebuild the binaries
# - Choose 'y' for first run or after making code changes
# - Choose 'n' to skip rebuild and use existing binaries

# Manual build
mkdir -p build && cd build
cmake .. && make

# Run individual components  
./build/bin/data_processing_service
./build/bin/test_all
```

#### On Windows (WSL Setup):
```bash
# First install WSL and Ubuntu from Microsoft Store if not already installed
# Open WSL terminal and navigate to your project directory
cd /mnt/c/path/to/your/project

# Install dependencies
sudo apt update
sudo apt install build-essential cmake python3 python3-pip dos2unix
pip3 install pandas matplotlib

# Clean any existing build cache and continue with Linux commands above
```

### Configuration
The system supports runtime configuration for:
- **Network parameters**: Multicast addresses, TCP ports
- **Data subjects**: Number and ID ranges
- **Processing algorithms**: Scoring calculation methods
- **Performance tuning**: Buffer sizes, thread counts

---

## Pre-Built Distribution

For users who prefer ready-to-run executables without compilation, this project provides pre-built distribution packages.

### Distribution Package Contents

The distribution includes statically-linked executables for maximum portability:
- **`dist/data_processing_service`** - Main processing engine (1.7MB)
- **`dist/test_all`** - Integration test and performance validator (1.7MB)  
- **`dist/tcp_receiver`** - TCP test server (1.4MB)
- **`dist/udp_packet_generator`** - Test data generator (1.4MB)
- **`dist/generate_test_vectors`** - Test vector creation utility (90KB)
- **`test_data/`** - Sample test vectors and validation data
- **Test scripts** - Complete validation and testing infrastructure

### Using the Distribution Package

#### Quick Start with Pre-Built Binaries:
```bash
# Extract the distribution package
tar -xzf data_processing_engine_linux.tar.gz
cd data_processing_engine_linux/

# Make executables accessible
chmod +x dist/*

# Run complete validation test
./test_distribution.sh

# Manual execution
./dist/data_processing_service <mcast_ip> <mcast_port> <interface> <tcp_host> <tcp_port>
```

#### Distribution Testing:
```bash
# Full pipeline test with pre-built executables
./test_distribution.sh
# Generates the same comprehensive performance reports as building from source
# Expected: ~99% message processing success rate
# Output: HTML performance dashboard, latency analysis, timing traces

# Quick integration test
./test_full_integration.sh
# Validates basic functionality without full performance analysis

# Manual component testing
./dist/test_all 224.0.0.1 1234 lo 127.0.0.1 6010
./dist/tcp_receiver 6010
```

### Distribution vs Source Build Comparison

| Aspect | Source Build | Distribution Package |
|--------|-------------|---------------------|
| **Setup Time** | 5-10 minutes (install deps + compile) | 30 seconds (extract + chmod) |
| **Dependencies** | CMake, GCC, Python packages | Only basic system libraries |
| **Executable Size** | ~52KB (dynamically linked) | ~1.7MB (statically linked) |
| **Portability** | Requires development environment | Runs on most Linux x86_64 systems |
| **Performance** | Identical | Identical |
| **Customization** | Full source modification | Limited to runtime parameters |

### Creating Distribution Packages

For developers who want to create their own distribution:
```bash
# Build multiple distribution variants
./build_distribution.sh

# Options available:
# 1. Standard build (default linking)
# 2. Static executable (portable, no dependencies) 
# 3. Shared library + executable
# 4. Release optimized build
# 5. All variants

# Package for distribution
tar -czf my_custom_engine.tar.gz dist/ test_data/ *.md *.sh
```

### System Requirements for Distribution

- **Operating System**: Linux x86_64 (Ubuntu 18.04+ or equivalent)
- **Architecture**: x86_64 (Intel/AMD 64-bit)
- **Dependencies**: Only basic system libraries (glibc, libpthread)
- **Python**: 3.6+ with pandas and matplotlib (for performance analysis only)
- **Network**: UDP multicast and TCP capabilities

### Distribution Performance Validation

The distribution package includes comprehensive testing that replicates the full development pipeline:
- **Test Coverage**: Identical to source build testing
- **Performance Metrics**: Same latency analysis and throughput measurement
- **Report Generation**: Full HTML dashboard with performance histograms
- **Integration Testing**: Complete end-to-end system validation

Expected results on reference hardware (Intel i7-8650U):
- **Processing Rate**: 99%+ success rate (990-998/1000 messages)
- **Latency**: Sub-millisecond end-to-end processing
- **Report Size**: ~330KB comprehensive HTML performance analysis

---

### Development Notes
Before committing to version control, clean build artifacts to prevent exposing local paths:
```bash
rm -rf build/ test_results/ logs/
```

---

## Applications

This generic data processing engine is suitable for:
- **Real-time analytics**: Stream processing, data aggregation, metric calculation
- **IoT data streams**: Sensor aggregation, real-time analytics
- **Network monitoring**: Traffic analysis, performance metrics
- **Scientific computing**: Real-time data reduction, signal processing
- **Gaming/simulation**: Live event processing, state synchronization

---

## Final Notes

- The main executable `data_processing_service` is built via `CMakeLists.txt` and orchestrated using shell scripts
- `run_full_pipeline.sh` provides end-to-end testing: build, data generation, and performance analysis
- `test_distribution.sh` provides identical testing for pre-built executables without compilation
- Only unique composite score updates per subject are transmitted to optimize bandwidth
- All components are designed for minimal latency and maximum throughput

This architecture demonstrates modern C++ best practices for high-performance data processing, suitable for both academic research and industrial applications.

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

### Third-Party Dependencies

- **CMake**: Build system (BSD 3-Clause License)
- **Python Libraries**: pandas, matplotlib (BSD License)
- **C++ Standard Library**: System-provided
- **Linux System Libraries**: glibc, libpthread (LGPL/GPL)

All dependencies maintain compatibility with the MIT License.
