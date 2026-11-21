# Nina

A very simple compiler for (yet another) toy language.
It actually started as a joke when my friend was taking too long to turn on the game
(he took so long I literally wrote a compiler in the meantime).

The source-to-C transpiler in this repo is built on top of the original code that I wrote on a whim.


# Building

Dependencies: any C11 compiler should work.

Simply run

    make

# Examples

```
import std.io

fun main()
    writeln("Hullo world")
    0
end
```

See more examples [in this directory](/examples).

