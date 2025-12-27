// Sample JS for lexer smoke test
var x = 42;
let str = "hello";
// unterminated string to trigger L1 recovery
let bad = 'oops
x++; // increment
/* unterminated block comment starts
function test(a, b) {
  return a + b;
}
