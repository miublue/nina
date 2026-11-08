import std.io

fun main()
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

    return 0
end
