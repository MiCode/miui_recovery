SET(SRC_LIST \
    recovery_ui.c \
    mount.c \
    miui_intent.c \
    bootloader.c \
    install.c \
    roots.c \
    nandroid.c \
    verifier.c \
    recovery.c )
ADD_EXECUTABLE(recovery ${SRC_LIST})


ADD_SUBDIRECTORY(bmlutils)
ADD_SUBDIRECTORY(falshutils)
ADD_SUBDIRECTORY(libcrecovery)
ADD_SUBDIRECTORY(miui)
ADD_SUBDIRECTORY(minelf)
ADD_SUBDIRECTORY(minzip)
ADD_SUBDIRECTORY(mtdutils)
ADD_SUBDIRECTORY(mmcutils)
ADD_SUBDIRECTORY(tools)
ADD_SUBDIRECTORY(edify)
ADD_SUBDIRECTORY(updater)
ADD_SUBDIRECTORY(applypatch)

