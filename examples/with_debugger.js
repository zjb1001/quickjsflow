// Example with debugger statements
function factorial(n) {
  debugger;
  
  if (n <= 1) {
    debugger;
    return 1;
  }
  
  debugger;
  const result = n * factorial(n - 1);
  
  debugger;
  return result;
}

function fibonacci(n) {
  debugger;
  
  if (n <= 1) {
    return n;
  }
  
  debugger;
  return fibonacci(n - 1) + fibonacci(n - 2);
}

debugger;
const fact5 = factorial(5);
debugger;
const fib7 = fibonacci(7);
debugger;
