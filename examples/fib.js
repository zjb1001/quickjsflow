function fibonacci(n) {
  if (n <= 1) {
    return n;
  }
  var a = 0;
  var b = 1;
  for (var i = 2; i <= n; i++) {
    var temp = a + b;
    a = b;
    b = temp;
  }
  return b;
}
