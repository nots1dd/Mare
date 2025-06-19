# Here are some known issues with this compiler:

1. Type promotion
2. When calling a function, evaluate the callee's prototype and the argument being passed:

Take the example of:

```mare
extern __mare_sqrtd(double a) -> void;

# Square root (sqrt)
fn unary?(v) -> double
{
  ret __mare_sqrtd(v);
}

# here the arg is double by default
fn custom(x) -> void
{
  var n = ?x;
  __mare_printstr("\n----------------\n");
  __mare_printd(n);
  __mare_printstr("----------------\n");
}

fn retint(i16 a) -> i16
{
  ret a*a;
}

# by default the return type of this is void.
fn problemFunction()
{
  var x = retint(12); # this is now of type i16
  custom(x); # !!! Type escalation problem !!! (x is of i16 but being passed as a double)
}
```
