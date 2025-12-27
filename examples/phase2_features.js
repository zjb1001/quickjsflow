// Phase 2: Modern JavaScript Features Examples
// This file demonstrates ES6+ features supported by QuickJSFlow

// 1. for-of loop
for (const item of items) {
    console.log(item);
}

// 2. for-in loop
for (const key in object) {
    console.log(key, object[key]);
}

// 3. Template literals
const name = "World";
const greeting = `Hello, ${name}!`;
const multiline = `
    Line 1
    Line 2
    Line 3
`;

// 4. Class declarations
class Animal {
    constructor(name) {
        this.name = name;
    }
    
    speak() {
        console.log(`${this.name} makes a sound`);
    }
}

// 5. Class inheritance with extends and super
class Dog extends Animal {
    constructor(name, breed) {
        super(name);
        this.breed = breed;
    }
    
    speak() {
        super.speak();
        console.log(`${this.name} barks`);
    }
}

// 6. this keyword
const obj = {
    value: 42,
    getValue: function() {
        return this.value;
    }
};

// 7. Arrow functions (simplified version)
const add = (a, b) => a + b;
const square = x => x * x;

// 8. Spread operator
const arr1 = [1, 2, 3];
const arr2 = [...arr1, 4, 5];

// 9. Rest parameters
function sum(...numbers) {
    return numbers.reduce((a, b) => a + b, 0);
}

// 10. Destructuring assignment
const {x, y} = point;
const [first, second] = array;

// 11. Default parameters
function greet(name = "Guest") {
    console.log(`Hello, ${name}!`);
}

// 12. Object shorthand
const name2 = "Alice";
const age = 30;
const person = {name2, age};

// 13. Method shorthand
const calculator = {
    add(a, b) {
        return a + b;
    },
    multiply(a, b) {
        return a * b;
    }
};

// 14. Async/await (parsed but not executed)
async function fetchData() {
    const response = await fetch(url);
    return response.json();
}

// 15. Class expression
const MyClass = class {
    constructor() {
        this.value = 0;
    }
};
