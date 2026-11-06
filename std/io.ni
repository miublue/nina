extern write(_)

writeln(_) = do
    len = length(_va_list)
    i = 0
    while i < len
        write(_va_list[i])
        i = i + 1
    end
    write('\n')
end

