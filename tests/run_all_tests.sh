#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

PASSED=0
FAILED=0
TOTAL=0

echo "========================================"
echo "  PHASE 0 - TEST SUITE"
echo "========================================"
echo ""

run_test() {
	local test_name=$1
	local exe_name=$2
	local source_files=$3
	
	echo -n "Running $test_name... "
	
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
		cat /tmp/test_output.txt
		((FAILED++))
		((TOTAL++))
		return 1
	fi
}

echo "--- Running Tests ---"
echo ""

run_test "Config Parsing" "test_config_parsing" ""
run_test "ServerConfig" "test_serverconfig" ""
run_test "Location" "test_location" ""
run_test "HttpRequest" "test_http_request" ""
run_test "HttpResponse" "test_http_response" ""
run_test "Server Integration" "test_server_integration" ""

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
	echo ""
	echo "Phase 0 is complete!"
	exit 0
else
	echo -e "${RED}✗ SOME TESTS FAILED OR NOT COMPILED${NC}"
	exit 1
fi
