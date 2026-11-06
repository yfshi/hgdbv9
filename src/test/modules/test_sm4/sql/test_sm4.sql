-- SM4 algorithm regression tests
-- This file contains SQL tests for SM4 encryption algorithms

-- Load the test module
CREATE EXTENSION IF NOT EXISTS test_sm4;

-- Test SM4 algorithm functions
SELECT test_sm4_all();

-- Clean up
DROP EXTENSION test_sm4;