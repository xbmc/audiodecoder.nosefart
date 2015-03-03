set(SOURCES src/log.c
            src/memguard.c
            src/cpu/nes6502/dis6502.c
            src/cpu/nes6502/nes6502.c
            src/machine/nsf.c
            src/sndhrdw/fds_snd.c
            src/sndhrdw/fmopl.c
            src/sndhrdw/mmc5_snd.c
            src/sndhrdw/nes_apu.c
            src/sndhrdw/vrc7_snd.c
            src/sndhrdw/vrcvisnd.c)

if(NOT WIN32)
  add_options(ALL_LANGUAGES ALL_BUILDS "-fPIC")
endif()

add_definitions(-DNSF_PLAYER)

add_library(nosefart STATIC ${SOURCES})
