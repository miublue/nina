extern write(val)

writeln(str) = do
    write(str)
    write('\n')
end

greet(name) = do
    write('Hello, ')
    write(name)
    writeln("!")
end

main() = do
    name = "World"
    greet(name)

    NICE0 = 3 + 33 * 2
    NICE1 = 4000 / 10 + 40-20
    val = 80085

    if (val == NICE0)
        writeln("val is 69")
    else if (val == NICE1)
        writeln("val is 420")
    else
        write("val is not 69 nor 420, it is actually ")
        writeln(val)
    end

    0
end

