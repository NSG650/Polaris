target remote :1234

b _start

define reset
    mon system_reset
    maintenance flush register-cache
    c
end

reset
