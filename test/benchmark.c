#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../test/benchmark_framework.h"
#include "../include/quickjsflow/lexer.h"
#include "../include/quickjsflow/parser.h"
#include "../include/quickjsflow/scope.h"
#include "../include/quickjsflow/codegen.h"

// Sample JavaScript files for benchmarking
static const char* SMALL_CODE = 
    "function add(a, b) { return a + b; }\n"
    "const x = 10;\n"
    "console.log(add(x, 20));\n";

static const char* MEDIUM_CODE =
    "class Calculator {\n"
    "  constructor() {\n"
    "    this.result = 0;\n"
    "  }\n"
    "  add(x) { this.result += x; return this; }\n"
    "  subtract(x) { this.result -= x; return this; }\n"
    "  multiply(x) { this.result *= x; return this; }\n"
    "  divide(x) { this.result /= x; return this; }\n"
    "  getResult() { return this.result; }\n"
    "}\n"
    "const calc = new Calculator();\n"
    "calc.add(10).multiply(2).subtract(5).divide(3);\n"
    "console.log(calc.getResult());\n"
    "\n"
    "function fibonacci(n) {\n"
    "  if (n <= 1) return n;\n"
    "  return fibonacci(n - 1) + fibonacci(n - 2);\n"
    "}\n"
    "\n"
    "for (let i = 0; i < 10; i++) {\n"
    "  console.log(`fib(${i}) = ${fibonacci(i)}`);\n"
    "}\n";

static const char* LARGE_CODE =
    "// Large code sample\n"
    "class EventEmitter {\n"
    "  constructor() {\n"
    "    this.events = {};\n"
    "  }\n"
    "  on(event, listener) {\n"
    "    if (!this.events[event]) {\n"
    "      this.events[event] = [];\n"
    "    }\n"
    "    this.events[event].push(listener);\n"
    "    return this;\n"
    "  }\n"
    "  emit(event, ...args) {\n"
    "    if (!this.events[event]) return false;\n"
    "    this.events[event].forEach(listener => listener(...args));\n"
    "    return true;\n"
    "  }\n"
    "  removeListener(event, listenerToRemove) {\n"
    "    if (!this.events[event]) return this;\n"
    "    this.events[event] = this.events[event].filter(\n"
    "      listener => listener !== listenerToRemove\n"
    "    );\n"
    "    return this;\n"
    "  }\n"
    "}\n"
    "\n"
    "class HttpServer extends EventEmitter {\n"
    "  constructor(port) {\n"
    "    super();\n"
    "    this.port = port;\n"
    "    this.routes = new Map();\n"
    "  }\n"
    "  route(path, handler) {\n"
    "    this.routes.set(path, handler);\n"
    "    return this;\n"
    "  }\n"
    "  handleRequest(req) {\n"
    "    const handler = this.routes.get(req.path);\n"
    "    if (handler) {\n"
    "      const response = handler(req);\n"
    "      this.emit('response', response);\n"
    "      return response;\n"
    "    }\n"
    "    this.emit('notFound', req);\n"
    "    return { status: 404, body: 'Not Found' };\n"
    "  }\n"
    "  listen() {\n"
    "    console.log(`Server listening on port ${this.port}`);\n"
    "    this.emit('listening', this.port);\n"
    "  }\n"
    "}\n"
    "\n"
    "const server = new HttpServer(3000);\n"
    "server.route('/', (req) => ({ status: 200, body: 'Home' }));\n"
    "server.route('/api/users', (req) => ({\n"
    "  status: 200,\n"
    "  body: JSON.stringify([{ id: 1, name: 'Alice' }])\n"
    "}));\n"
    "server.on('listening', (port) => {\n"
    "  console.log(`Listening event fired for port ${port}`);\n"
    "});\n"
    "server.listen();\n"
    "\n"
    "// Data processing utilities\n"
    "function map(array, fn) {\n"
    "  const result = [];\n"
    "  for (let i = 0; i < array.length; i++) {\n"
    "    result.push(fn(array[i], i));\n"
    "  }\n"
    "  return result;\n"
    "}\n"
    "\n"
    "function filter(array, predicate) {\n"
    "  const result = [];\n"
    "  for (let i = 0; i < array.length; i++) {\n"
    "    if (predicate(array[i], i)) {\n"
    "      result.push(array[i]);\n"
    "    }\n"
    "  }\n"
    "  return result;\n"
    "}\n"
    "\n"
    "function reduce(array, fn, initial) {\n"
    "  let accumulator = initial;\n"
    "  for (let i = 0; i < array.length; i++) {\n"
    "    accumulator = fn(accumulator, array[i], i);\n"
    "  }\n"
    "  return accumulator;\n"
    "}\n";

static void benchmark_lexer(BenchmarkSuite* suite, const char* name, 
                           const char* code, int iterations) {
    size_t len = strlen(code);
    
    for (int i = 0; i < iterations; i++) {
        Lexer lexer;
        lexer_init(&lexer, code, len);
        
        BenchmarkTimer timer;
        benchmark_start(&timer);
        
        Token token;
        do {
            token = lexer_next(&lexer);
            token_free(&token);
        } while (token.type != TOKEN_EOF && token.type != TOKEN_ERROR);
        
        benchmark_end(&timer);
        benchmark_suite_update(suite, name, timer.elapsed_ms, len);
    }
}

static void benchmark_parser(BenchmarkSuite* suite, const char* name,
                            const char* code, int iterations) {
    size_t len = strlen(code);
    
    for (int i = 0; i < iterations; i++) {
        Parser parser;
        parser_init(&parser, code, len);
        
        BenchmarkTimer timer;
        benchmark_start(&timer);
        
        AstNode* program = parse_program(&parser);
        
        benchmark_end(&timer);
        benchmark_suite_update(suite, name, timer.elapsed_ms, len);
        
        if (program) {
            ast_free(program);
        }
    }
}

static void benchmark_full_pipeline(BenchmarkSuite* suite, const char* name,
                                   const char* code, int iterations) {
    size_t len = strlen(code);
    
    for (int i = 0; i < iterations; i++) {
        Parser parser;
        parser_init(&parser, code, len);
        
        BenchmarkTimer timer;
        benchmark_start(&timer);
        
        AstNode* program = parse_program(&parser);
        if (program) {
            ScopeManager sm;
            scope_manager_init(&sm);
            scope_analyze(&sm, program, 1); // 1 = is_module
            scope_manager_free(&sm);
            
            CodegenOptions opts = {2, ' ', 0, NULL};
            CodegenResult result = codegen_generate(program, &opts);
            if (result.code) {
                codegen_result_free(&result);
            }
        }
        
        benchmark_end(&timer);
        benchmark_suite_update(suite, name, timer.elapsed_ms, len);
        
        if (program) {
            ast_free(program);
        }
    }
}

int main(void) {
    printf("QuickJSFlow Performance Benchmarks\n");
    printf("===================================\n\n");
    
    BenchmarkSuite* suite = benchmark_suite_create();
    
    // Lexer benchmarks
    printf("Running lexer benchmarks...\n");
    benchmark_lexer(suite, "Lexer - Small (100 iter)", SMALL_CODE, 100);
    benchmark_lexer(suite, "Lexer - Medium (50 iter)", MEDIUM_CODE, 50);
    benchmark_lexer(suite, "Lexer - Large (20 iter)", LARGE_CODE, 20);
    
    // Parser benchmarks
    printf("Running parser benchmarks...\n");
    benchmark_parser(suite, "Parser - Small (100 iter)", SMALL_CODE, 100);
    benchmark_parser(suite, "Parser - Medium (50 iter)", MEDIUM_CODE, 50);
    benchmark_parser(suite, "Parser - Large (20 iter)", LARGE_CODE, 20);
    
    // Full pipeline benchmarks
    printf("Running full pipeline benchmarks...\n");
    benchmark_full_pipeline(suite, "Full - Small (50 iter)", SMALL_CODE, 50);
    benchmark_full_pipeline(suite, "Full - Medium (25 iter)", MEDIUM_CODE, 25);
    benchmark_full_pipeline(suite, "Full - Large (10 iter)", LARGE_CODE, 10);
    
    // Print results
    benchmark_suite_print(suite, stdout);
    
    // Save results to file
    benchmark_suite_save(suite, "build/benchmark/results.txt");
    printf("\nResults saved to build/benchmark/results.txt\n");
    
    benchmark_suite_free(suite);
    return 0;
}
