#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
GREY='\033[90m'
NC='\033[0m'


if [ -z "${TESTS_BINARIES:-}" ]; then
	echo -e "${RED}Error: \$TESTS_BINARIES not set${NC}"
	exit 1
fi

PASSED=0
FAILED=0
TOTAL=0

run_test() {
	local exe_name=$1
	
	echo -ne "${GREY}Running $exe_name ${NC}"
	
	if [ ! -f "${exe_name}" ]; then
		echo -e "${RED}✗ SKIP (not compiled)${NC}"
		return 1
	fi
	
	./${exe_name} > /tmp/test_output.txt 2>&1
	
	if [ $? -eq 0 ]; then
		echo -e "${GREEN}✓ PASS${NC}"
		((PASSED++))
		((TOTAL++))
		return 0
	else
		echo -e "${RED}✗ FAIL${NC}"
		echo -e "${YELLOW}Test output:${NC}"
		cat /tmp/test_output.txt
		echo

		((FAILED++))
		((TOTAL++))

		return 1
	fi
}

echo "--- Running Tests ---"
echo ""

for test_bin in ${TESTS_BINARIES}; do
	run_test $test_bin
done

echo ""
echo "========================================"
echo "  TEST SUMMARY"
echo "========================================"
echo -e "Total:  ${TOTAL}"
echo -e "Passed: ${GREEN}${PASSED}${NC}"
echo -e "Failed: ${RED}${FAILED}${NC}"
echo ""

if [ $FAILED -eq 0 ] && [ $TOTAL -eq 6 ]; then
       echo -e "${GREEN}✓ ALL TESTS PASSED${NC}"
fi
