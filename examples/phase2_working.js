// Phase-2 Working Features

// Single-parameter arrow functions
const square = x => x * x;
const double = x => { return x * 2; };

// Template literals
const greeting = `Hello`;

// For-of loop
for (const item of items) {
    console.log(item);
}

// For-in loop  
for (const key in obj) {
    console.log(key);
}

// Class declaration
class Point {
    constructor(x, y) {
        this.x = x;
        this.y = y;
    }
}

// Class with extends
class Point3D extends Point {
    constructor(x, y, z) {
        super(x, y);
        this.z = z;
    }
}

// This expression
this.value = 42;

// Super
super.method();
