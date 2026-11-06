# TECC Key Management Module Test Suite

## Overview

This directory contains the test suite for the TECC Key Management Module. The tests verify the functionality, performance, and error handling of the key management system.

## Test Structure

### 0. Minimal and Standalone Tests
- **`test_minimal.c`**: Basic verification of core concepts and utilities
- **`test_simple_standalone.c`**: Self-contained test with mock implementations
- **Purpose**: Quick validation without complex dependencies

### 1. Basic Tests (`test_key_manager_basic.c`)
- **Purpose**: Verify core functionality of high-level API
- **Tests**:
  - `tecc_key_get_kek()` - Retrieve Key Encryption Key
  - `tecc_key_get_dek()` - Retrieve Data Encryption Key
  - `tecc_key_decrypt_dek()` - Decrypt DEK using KEK
  - `tecc_key_get_decrypted_dek()` - Combined get+decrypt operation
  - `tecc_key_get_kek_for_dek()` - Get KEK associated with a DEK
  - `tecc_key_validate_dek()` - Validate DEK existence and status
  - `tecc_key_validate_kek()` - Validate KEK existence
  - API version checking functions
  - Error handling and message persistence

### 2. Integration Tests (`test_key_manager_integration.c`)
- **Purpose**: Test module interactions and complex scenarios
- **Tests**:
  - Multiple DEK operations with different keys
  - DEKs associated with different KEKs
  - DEK status handling (active, expired, non-existent)
  - Concurrent access patterns
  - Memory management and secure erase
  - Error recovery scenarios

### 3. Error Handling Tests (`test_key_manager_error.c`)
- **Purpose**: Test error conditions, edge cases, and fault tolerance
- **Tests**:
  - NULL parameter handling
  - Empty string handling
  - Invalid key name patterns
  - Cache failure simulation
  - Decryption failure handling
  - Memory allocation failure scenarios
  - Error message persistence
  - Concurrent error handling

### 4. Performance Tests (`test_key_manager_performance.c`)
- **Purpose**: Test throughput, latency, and resource usage
- **Tests**:
  - KEK throughput (operations per second)
  - DEK throughput
  - KEK latency measurements
  - DEK latency measurements
  - Mixed workload simulation
  - Memory footprint analysis
  - Cache performance and hit ratios

## Mock Dependencies

The tests use mock implementations for:
- `tecc_cache_get_dek_by_name()` - Cache interface (replaces old tecc_metadata_cache_get_column_key())
- `security_decrypt_dek_with_hmac()` - Decryption function
- `tecc_key_secure_erase()` - Secure memory erasure

## Build and Run

### Building Tests
```bash
# Build all key manager tests
make -C test_key_manager

# Or from the parent directory
make key_manager

# Build minimal standalone tests
make test_minimal
make test_simple_standalone
```

### Running Tests
```bash
# Run all key manager tests
make -C test_key_manager test

# Run specific test
./test_key_manager_basic
./test_key_manager_integration
./test_key_manager_error
./test_key_manager_performance

# Run standalone tests
./test_minimal
./test_simple_standalone
```

### Running All TECC Tests
```bash
# From tecc_test directory
make test
```

## Test Coverage

### Functional Coverage
- [x] KEK retrieval (fixed KEK implementation)
- [x] DEK retrieval from cache
- [x] DEK decryption with KEK
- [x] Combined get+decrypt operations
- [x] Key validation
- [x] API version management
- [x] Error handling and reporting

### Integration Coverage
- [x] Multiple key operations
- [x] Different KEK associations
- [x] Status-based access control
- [x] Concurrent access patterns
- [x] Memory lifecycle management
- [x] Error recovery paths

### Performance Coverage
- [x] Throughput measurements
- [x] Latency profiling
- [x] Mixed workload simulation
- [x] Memory usage tracking
- [x] Cache performance analysis

### Error Coverage
- [x] Invalid parameter handling
- [x] Edge case scenarios
- [x] Failure simulation
- [x] Error message persistence
- [x] Concurrent error handling

## Test Data

### Mock Cache Entries
The tests use the following mock cache entries:

1. **dek_customer_id**
   - KEK ID: 10001
   - Supports: DET, Search
   - Status: active

2. **dek_ssn**
   - KEK ID: 10001
   - Supports: DET, OPE
   - Status: active

3. **dek_salary**
   - KEK ID: 10002
   - Supports: OPE
   - Status: active

4. **dek_expired**
   - KEK ID: 10001
   - Supports: DET, Search
   - Status: expired

### Performance Test Keys
- `dek_perf_001` to `dek_perf_005` - For throughput and latency tests
- `dek_uncached_001` to `dek_uncached_004` - For cache miss simulation

## Expected Results

### Basic Tests
- All high-level API functions should work correctly
- Error messages should be informative and persistent
- API version checking should enforce compatibility

### Integration Tests
- Multiple operations should complete successfully
- Status-based access control should work
- Memory should be properly managed and securely erased

### Error Tests
- Invalid inputs should be rejected with appropriate errors
- System should recover gracefully from failures
- Error messages should not interfere with subsequent operations

### Performance Tests
- Throughput should be reasonable for the operations
- Latency should be within acceptable limits
- Memory usage should not grow unbounded
- Cache should provide performance benefits

## Notes

1. **Fixed KEK**: The current implementation uses a fixed KEK for testing. In production, this would be retrieved from a KMS or database.

2. **Mock Cache**: Tests use a mock cache implementation. Integration with the real `tecc_metadata_cache` module would be needed for production testing.

3. **Security Functions**: The `security_decrypt_dek_with_hmac()` function is mocked. Real security testing would require the actual encryption module.

4. **Performance Baseline**: Performance results are system-dependent and should be used for relative comparisons rather than absolute thresholds.

## Maintenance

### Adding New Tests
1. Create new test file following existing patterns
2. Add to `Makefile` in `KEY_MANAGER_TEST_SRCS`
3. Update `TEST_SUMMARY.md` with test description

### Updating Mock Data
1. Modify mock cache entries in test files as needed
2. Ensure test data covers new scenarios
3. Update documentation accordingly

### Running Regression Tests
```bash
# Clean and rebuild
make -C test_key_manager clean
make -C test_key_manager test
```

## Author

Authored by <wanghaoyan@highgo.com>, 20260413

## Copyright

Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.