extern write(_)
extern length(val)

writeln(_) = do
    len = length(_va_list)
    i = 0
    while i < len
        write(_va_list[i])
        i = i + 1
    end
    write('\n')
end

greet(name) = writeln('Hello, ', name, '!')

main() = do
    name = "World"
    greet(name)

    array = [69, 0, "string", ["foo", "bar"]]
    writeln(array[1] = 420)
    writeln(array)

    NICE0 = 3 + 33 * 2
    NICE1 = 4000 / 10 + 40-20
    val = 80085

    if (val == NICE0)
        writeln("val is 69")
    else if (val == NICE1)
        writeln("val is 420")
    else
        writeln("val is not 69 nor 420, it is actually ", val)
    end

    # loop from 0 to 9, skip 3, break on 7
    i = 0
    while i < 10
        i = i + 1
        if i == 3 continue end
        if i == 7 break end
        writeln(i)
    end

    i = 0
    while i < length(array)
        writeln(array[i])
        i = i + 1
    end

    0
end

