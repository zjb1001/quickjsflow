// Example with console.log statements
function greet(name) {
  console.log("Starting greet function");
  console.log("Name is:", name);
  
  const message = "Hello, " + name + "!";
  console.log("Message:", message);
  
  return message;
}

function calculate(a, b) {
  console.log("Calculating...");
  const result = a + b;
  console.log("Result:", result);
  return result;
}

const x = 10;
const y = 20;
console.log("x =", x);
console.log("y =", y);

const sum = calculate(x, y);
console.log("Sum:", sum);

const greeting = greet("World");
console.log("Final greeting:", greeting);
