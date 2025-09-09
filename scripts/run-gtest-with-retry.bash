#!/bin/bash

# ==============================================================================
# A script to run GoogleTest tests one by one with a specified number of retries.
#
# Overview:
#   - Automatically retrieves a list of test cases from the specified GoogleTest binary.
#   - Executes each test case individually with retries on failure.
#   - If a test succeeds even once, it is considered "PASSED".
#   - With the --verbose option, it shows the output for all attempts.
#   - Without it, it only shows the output for failed attempts.
#
# Usage:
#   ./run_gtest_with_retry.sh [--verbose] [path_to_test_binary] [max_retries]
#
# Examples:
#   ./run_gtest_with_retry.sh ./my_test_app 10
#   ./run_gtest_with_retry.sh --verbose ./my_test_app 5
#   ./run_gtest_with_retry.sh ./my_test_app --verbose
#
# Default values if arguments are omitted:
#   - Path to test binary: ./gtest_main
#   - Max retries: 10
# ==============================================================================

# --- Configuration & Argument Parsing ---
VERBOSE=false
POSITIONAL_ARGS=()

# Separate --verbose from other arguments
for arg in "$@"; do
  case $arg in
    --verbose)
      VERBOSE=true
      ;;
    *)
      POSITIONAL_ARGS+=("$arg")
      ;;
  esac
done

# Assign positional arguments
TEST_BINARY=${POSITIONAL_ARGS[0]:-./gtest_main}
MAX_RETRIES=${POSITIONAL_ARGS[1]:-10}

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
if [ "$VERBOSE" = true ]; then
  echo "Verbose Mode: Enabled"
fi
echo

# 1. Get the list of test cases
echo "Getting the list of test cases..."
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
        
        output=$("$TEST_BINARY" --gtest_filter="$test_case" 2>&1)
        exit_code=$?

        if [ $exit_code -eq 0 ]; then
            echo "  -> Result: PASSED"
            is_successful=true
            # In verbose mode, show output even on success
            if [ "$VERBOSE" = true ]; then
                echo "    vvv Test output vvv"
                echo "$output" | sed 's/^/    | /'
                echo "    ^^^ End of output ^^^"
            fi
            break 
        else
            echo "  -> Result: FAILED (Exit Code: $exit_code)"
            # Always show output on failure
            echo "    vvv Test output vvv"
            echo "$output" | sed 's/^/    | /'
            echo "    ^^^ End of output ^^^"
        fi
    done

    # Add the test case to the appropriate list based on the final result
    if $is_successful; then
        succeeded_tests+=("$test_case")
    else
        echo "[FAILED]: $test_case did not pass after $MAX_RETRIES attempts."
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
