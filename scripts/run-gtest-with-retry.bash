#!/bin/bash

# ==============================================================================
# A script to run GoogleTest tests one by one with a specified number of retries.
#
# Overview:
#   - Automatically retrieves a list of test cases from the specified GoogleTest binary.
#   - Executes each test case individually.
#   - If a test fails, it will be retried up to a maximum number of times until it succeeds.
#   - If a test succeeds even once, it is considered "PASSED".
#   - After all test cases have been run, a summary of passed and failed tests is displayed.
#
# Usage:
#   ./run_gtest_with_retry.sh [path_to_test_binary] [max_retries]
#
# Example:
#   ./run_gtest_with_retry.sh ./my_test_app 10
#
# Default values if arguments are omitted:
#   - Path to test binary: ./gtest_main
#   - Max retries: 10
# ==============================================================================

# --- Configuration ---
# Use the first argument if provided, otherwise use the default value
TEST_BINARY=${1:-./gtest_main}
# Use the second argument if provided, otherwise use the default value
MAX_RETRIES=${2:-10}
# Set to true to suppress the standard output of successful GoogleTest runs for a cleaner log
HIDE_SUCCESS_OUTPUT=true

# --- Pre-flight Checks ---
# Check if the test binary exists and has execute permissions
if [ ! -f "$TEST_BINARY" ]; then
    echo "Error: Test binary '$TEST_BINARY' not found."
    exit 1
fi
if [ ! -x "$TEST_BINARY" ]; then
    echo "Error: Test binary '$TEST_BINARY' does not have execute permissions."
    exit 1
fi

# --- Main Processing ---
echo "Test Binary: $TEST_BINARY"
echo "Max Retries: $MAX_RETRIES"
echo

# 1. Get the list of test cases
echo "Getting the list of test cases..."
# Parse the output of `--gtest_list_tests` to format it as "TestSuite.TestCase"
test_cases=$(
  "$TEST_BINARY" --gtest_list_tests | \
  awk '
    /^[A-Za-z0-9_]+\./ {
      current_suite = $1
      sub(/\.$/, "", current_suite)
    }
    /^[ ]{2}[A-Za-z0-9_]+/ {
      print current_suite "." $1
    }
  '
)

if [ -z "$test_cases" ]; then
    echo "Warning: No executable test cases were found."
    exit 0
fi
echo "Number of test cases found: $(echo "$test_cases" | wc -w)"
echo

# 2. Execute each test case
succeeded_tests=()
failed_tests=()

echo "Starting tests..."
echo "========================================"

for test_case in $test_cases; do
    echo "[RUNNING]: $test_case"
    
    is_successful=false
    for i in $(seq 1 "$MAX_RETRIES"); do
        echo "  - Attempt: $i/$MAX_RETRIES"
        
        # Store GoogleTest output in a variable and get the exit code
        output=$("$TEST_BINARY" --gtest_filter="$test_case" 2>&1)
        exit_code=$?

        if [ $exit_code -eq 0 ]; then
            echo "  -> Result: PASSED"
            is_successful=true
            # If successful, show GoogleTest output based on the configuration
            if [ "$HIDE_SUCCESS_OUTPUT" = false ]; then
                echo "$output" | sed 's/^/    | /'
            fi
            break # Succeeded, so break the retry loop
        else
            echo "  -> Result: FAILED (Exit Code: $exit_code)"
            sleep 1
        fi
    done
    
    if $is_successful; then
        succeeded_tests+=("$test_case")
    else
        echo "[FAILED]: $test_case did not pass after $MAX_RETRIES attempts."
        # Display the output from the final failed attempt
        echo "    vvv Output from the last attempt vvv"
        echo "$output" | sed 's/^/    | /'
        echo "    ^^^ End of output ^^^"
        failed_tests+=("$test_case")
    fi
    echo "----------------------------------------"
done

# 3. Display the final summary
total_tests=$(echo "$test_cases" | wc -w)
num_succeeded=${#succeeded_tests[@]}
num_failed=${#failed_tests[@]}

echo "========================================"
echo "Test Result Summary"
echo "========================================"
echo "Total test cases: $total_tests"
echo "Passed: $num_succeeded"
echo "Failed: $num_failed"
echo

if [ $num_failed -gt 0 ]; then
    echo "List of failed test cases:"
    for test in "${failed_tests[@]}"; do
        echo "  - $test"
    done
    echo
    echo "Tests failed."
    exit 1
else
    echo "All test cases passed."
    exit 0
fi
