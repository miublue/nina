import std.io

# I started sketching out an OOP system, but I can't make it work well
# without static typing (Neither the analyzer nor the runtime).
# The compiler can parse this though.

class Person
    fun new(self, name, age)
        self.name = name
        self.age  = age
    end

    fun say(self, message)
        writeln(self.name, ' says: ', message)
    end
end

fun main()
    p = Person("Emanon", 30000000)
    p.say("There may be a new chapter sometime this century...")
end

