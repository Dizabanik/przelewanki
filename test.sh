#!/bin/bash

# --- Configuration ---
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
NC='\033[0m'

PROGRAM="./test.out"

# --- Pre-run Checks ---

g++ lew.cpp @opcjeCpp -o test.out

if [ -z "$1" ]; then
    echo -e "${RED}Error: No test directory specified.${NC}"
    echo "Usage: $0 <path_to_test_directory>"
    exit 1
fi

TEST_DIR="tests/$1"
NO_OUTPUT="${2:-}"

if [ ! -d "$TEST_DIR" ]; then
    echo -e "${RED}Error: Directory '${TEST_DIR}' not found.${NC}"
    exit 1
fi

if [ ! -x "$PROGRAM" ]; then
    echo -e "${RED}Error: Program '${PROGRAM}' not found or is not executable.${NC}"
    exit 1
fi

# --- Test Execution ---

passed=0
failed=0
skipped=0
total=0

total_time_ns=0

echo "Starting tests for '$PROGRAM' in directory '$TEST_DIR'..."
echo ""

shopt -s nullglob

for input_file in "$TEST_DIR"/*.in; do
    total=$((total + 1))
    filename=$(basename -- "$input_file")
    i="${filename%.in}"
    output_file="$TEST_DIR/$i.out"

    if [ ! -f "$output_file" ]; then
        echo -e "[${YELLOW}SKIPPED${NC}] $filename (Missing $i.out)"
        skipped=$((skipped + 1))
        continue
    fi

    # --- TIME MEASUREMENT START ---
    start_ns=$(date +%s%N)

    program_output=$("$PROGRAM" < "$input_file")
    diff_output=$(diff -q <(echo "$program_output") "$output_file")
    exit_code=$?

    end_ns=$(date +%s%N)
    elapsed_ns=$((end_ns - start_ns))
    total_time_ns=$((total_time_ns + elapsed_ns))

    # human-readable ms
    elapsed_ms=$(awk "BEGIN {printf \"%.3f\", $elapsed_ns/1e6}")

    if [ "$exit_code" -eq 0 ]; then
        passed=$((passed + 1))
        if [ "$NO_OUTPUT" != "no" ]; then
            echo -e "[ ${GREEN}PASSED${NC} ]  $filename  (${elapsed_ms} ms)"
        fi
    else
        failed=$((failed + 1))
        if [ "$NO_OUTPUT" != "no" ]; then
            echo -e "[ ${RED}FAILED${NC} ]  $filename  (${elapsed_ms} ms)"
        fi
    fi
done

# --- Summary ---

echo ""
echo "-------------------------------------"
echo "Test Run Summary"
echo "-------------------------------------"
echo -e "Total Tests:   $total"
echo -e "${GREEN}Passed:        $passed${NC}"
echo -e "${RED}Failed:        $failed${NC}"
echo -e "${YELLOW}Skipped:       $skipped${NC}"

# compute total + average
total_ms=$(awk "BEGIN {printf \"%.3f\", $total_time_ns/1e6}")
avg_ms=$(awk "BEGIN {printf \"%.3f\", ($total_time_ns/1e6)/$total}")

echo "-------------------------------------"
echo -e "Total Time:    ${total_ms} ms"
echo -e "Average Time:  ${avg_ms} ms/test"
echo "-------------------------------------"

# return non-zero if failed
[ "$failed" -gt 0 ] && exit 1 || exit 0
