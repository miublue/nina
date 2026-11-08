import std.array, std.io

fun main()
    array = [69, 0, "string", ["foo", "bar"]]
    append(array, "message")
    array[1] = 420
    writeln(array[3][0])
    pop(array)
    writeln(array)
    other = pop(array)
    writeln(other[0], other[1])

    i = 0
    while i < length(array)
        writeln(array[i])
        i = i + 1
    end

    0
end
