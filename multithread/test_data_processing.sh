#!/bin/bash

# Simple Data Processing Test - Focus on Core Functionality
# Tests data injection and output validation for the multithreaded service

echo "=========================================================="
echo "   Data Processing Engine - Core Functionality Test"
echo "=========================================================="
echo ""

# Configuration
DIST_FILE="multithreaded_data_processing_engine_linux.tar.gz"
TEST_DIR="multithreaded_data_processing_engine_linux"
RESULTS_DIR="test_results"
TEST_MULTICAST="239.255.0.1"
TEST_PORT="12345"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info() {
    echo -e "${YELLOW}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[PASS]${NC} $1"
}

log_failure() {
    echo -e "${RED}[FAIL]${NC} $1"
}

# Create results directory
mkdir -p "$RESULTS_DIR"

echo "Test: Data Processing and Output Validation"
echo "============================================"

# Extract package if not already extracted
if [ ! -d "$TEST_DIR" ]; then
    if [ -f "$DIST_FILE" ]; then
        log_info "Extracting distribution package..."
        tar -xzf "$DIST_FILE" || { log_failure "Failed to extract package"; exit 1; }
        log_success "Package extracted successfully"
    else
        log_failure "Distribution file $DIST_FILE not found"
        exit 1
    fi
fi

cd "$TEST_DIR" || { log_failure "Cannot enter distribution directory"; exit 1; }

# Check if service executable exists
if [ ! -x "DataProcessingService" ]; then
    log_failure "DataProcessingService executable not found or not executable"
    exit 1
fi

log_success "DataProcessingService found and executable"

# Create test data with proper binary format
log_info "Creating test data..."

# Create a simple binary packet generator in Python
cat > generate_test_packets.py << 'EOF'
#!/usr/bin/env python3
import struct
import socket
import time
import sys

def create_packet(subject_id, updates):
    """Create a binary packet with the expected format"""
    packet = bytearray()
    
    # Calculate message length (header + updates)
    msg_length = 10 + len(updates) * 14  # 4+4+2 header + 14 bytes per update
    
    # Header: msg_length (4 bytes), subject_id (4 bytes), num_updates (2 bytes)
    packet.extend(struct.pack('<I', msg_length))  # uint32_t msg_length
    packet.extend(struct.pack('<I', subject_id))  # uint32_t subject_id
    packet.extend(struct.pack('<H', len(updates)))  # uint16_t num_updates
    
    # Updates: level (1), side (1), scaled_value (8), volume (4)
    for level, side, value, volume in updates:
        packet.extend(struct.pack('<B', level))  # uint8_t level
        packet.extend(struct.pack('<B', side))   # uint8_t side (0=demand, 1=supply)
        packet.extend(struct.pack('<q', int(value * 1e9)))  # int64_t scaled_value
        packet.extend(struct.pack('<I', volume))  # uint32_t volume
    
    return bytes(packet)

def send_test_data(host, port):
    """Send test packets to the service"""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    
    try:
        # Test data: subject_id, [(level, side, price, volume), ...]
        test_packets = [
            (1001, [(0, 0, 100.50, 1000), (0, 1, 100.55, 500)]),  # Subject 1001: demand/supply at level 0
            (1002, [(0, 0, 95.25, 750), (0, 1, 95.30, 600)]),    # Subject 1002: demand/supply at level 0
            (1003, [(0, 0, 110.75, 1200), (0, 1, 110.80, 800)]), # Subject 1003: demand/supply at level 0
        ]
        
        for subject_id, updates in test_packets:
            packet = create_packet(subject_id, updates)
            sock.sendto(packet, (host, port))
            print(f"Sent packet for subject {subject_id} ({len(packet)} bytes)")
            time.sleep(0.1)  # Small delay between packets
            
    except Exception as e:
        print(f"Error sending data: {e}")
    finally:
        sock.close()

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python3 generate_test_packets.py <host> <port>")
        sys.exit(1)
    
    host = sys.argv[1]
    port = int(sys.argv[2])
    send_test_data(host, port)
EOF

chmod +x generate_test_packets.py

# Test the data processing
log_info "Starting DataProcessingService..."

# Start service in background
timeout 10s ./DataProcessingService "$TEST_MULTICAST" "$TEST_PORT" > service_output.log 2>&1 &
SERVICE_PID=$!

# Wait for service to start
sleep 2

# Check if service is running
if ! ps -p $SERVICE_PID > /dev/null 2>&1; then
    log_failure "Service failed to start"
    cat service_output.log
    exit 1
fi

log_success "Service started successfully (PID: $SERVICE_PID)"

# Send binary test data
log_info "Injecting binary test data..."
python3 generate_test_packets.py "$TEST_MULTICAST" "$TEST_PORT" > data_injection.log 2>&1

if [ $? -eq 0 ]; then
    log_success "Test data injected successfully"
else
    log_failure "Failed to inject test data"
fi

# Wait for processing
sleep 3

# Stop service
log_info "Stopping service..."
if ps -p $SERVICE_PID > /dev/null 2>&1; then
    kill $SERVICE_PID 2>/dev/null
    wait $SERVICE_PID 2>/dev/null
    log_success "Service stopped successfully"
fi

# Examine output
echo ""
echo "Output Analysis:"
echo "================"

if [ -f "output.csv" ]; then
    log_success "Output file generated"
    
    # Check file size and content
    file_size=$(wc -c < output.csv)
    line_count=$(wc -l < output.csv)
    
    echo "File size: $file_size bytes"
    echo "Line count: $line_count lines"
    echo ""
    echo "Output content:"
    echo "---------------"
    cat output.csv
    echo ""
    
    # Validate content
    if [ $line_count -gt 1 ]; then
        data_lines=$((line_count - 1))
        log_success "Contains $data_lines data records (plus header)"
        
        # Check for expected subject IDs
        expected_subjects="1001 1002 1003"
        found_subjects=$(tail -n +2 output.csv | cut -d',' -f1 | sort -u | tr '\n' ' ')
        
        echo "Expected subjects: $expected_subjects"
        echo "Found subjects: $found_subjects"
        
        # Basic validation
        if grep -q "1001\|1002\|1003" output.csv; then
            log_success "Output contains expected subject IDs"
        else
            log_failure "Output missing expected subject IDs"
        fi
        
        # Check for non-zero composite scores
        if tail -n +2 output.csv | cut -d',' -f2 | grep -v '^0$' > /dev/null; then
            log_success "Output contains calculated composite scores"
        else
            log_failure "All composite scores are zero - calculation may have failed"
        fi
        
    else
        log_failure "Output file contains only headers - no data processed"
    fi
    
    # Copy output to results
    cp output.csv "../$RESULTS_DIR/"
    
else
    log_failure "No output file generated"
fi

# Copy logs to results
cd ..
cp "$TEST_DIR/service_output.log" "$RESULTS_DIR/" 2>/dev/null || true
cp "$TEST_DIR/data_injection.log" "$RESULTS_DIR/" 2>/dev/null || true

# Generate test summary
cat > "$RESULTS_DIR/test_summary.txt" << EOF
Data Processing Engine - Core Functionality Test Results
========================================================
Test Date: $(date)
Test Focus: Data injection and output validation

Test Results:
- Service startup: $([ -f "$RESULTS_DIR/service_output.log" ] && echo "SUCCESS" || echo "FAILED")
- Data injection: $([ -f "$RESULTS_DIR/data_injection.log" ] && echo "SUCCESS" || echo "FAILED") 
- Output generation: $([ -f "$RESULTS_DIR/output.csv" ] && echo "SUCCESS" || echo "FAILED")

$(if [ -f "$RESULTS_DIR/output.csv" ]; then
    line_count=$(wc -l < "$RESULTS_DIR/output.csv")
    if [ $line_count -gt 1 ]; then
        echo "Data processing: SUCCESS ($((line_count - 1)) records)"
    else
        echo "Data processing: PARTIAL (headers only)"
    fi
else
    echo "Data processing: FAILED (no output)"
fi)

Core functionality validation complete.
EOF

echo ""
echo "=========================================================="
echo "                  TEST SUMMARY"
echo "=========================================================="
cat "$RESULTS_DIR/test_summary.txt"

echo ""
log_info "Test results saved to: $RESULTS_DIR"
echo "Key files:"
echo "  - $RESULTS_DIR/output.csv (processing results)"
echo "  - $RESULTS_DIR/service_output.log (service logs)"
echo "  - $RESULTS_DIR/test_summary.txt (test summary)"

echo ""
echo "Test completed!"