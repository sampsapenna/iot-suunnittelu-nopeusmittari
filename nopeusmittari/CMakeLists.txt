add_executable(blink
        nopeusmittari.c
        )

# pull in common dependencies
target_link_libraries(pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(nopeusmittari)
