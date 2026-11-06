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
    writeln(3 + 33 * 2)
    writeln(4000 / 10 + 40-20)
    0
end

