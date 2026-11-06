extern write(val)

greet(name) = do
    write("Hello, ")
    write(name)
    write("!\n")
end

main() = do
    name = "World"
    greet(name)
    0
end

