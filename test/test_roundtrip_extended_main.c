/**
 * @file test_roundtrip_extended_main.c
 * @brief Main entry point for test_roundtrip_extended
 */

#include <stdio.h>
#include "quickjsflow/ast.h"
#include "test_framework_extended.h"

int main(void) {
    test_framework_init();
    printf("Round-trip extended framework initialized.\n");
    printf("This binary provides extended testing utilities for round-trip validation.\n");
    test_framework_cleanup();
    return 0;
}
