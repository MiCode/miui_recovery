libmiui_common_includes = $(LOCAL_PATH)/include $(LOCAL_PATH)/src
libmiui_common_src_files :=libs/png/png.c \
    libs/png/pngerror.c \
    libs/png/pnggccrd.c \
    libs/png/pngget.c \
    libs/png/pngmem.c \
    libs/png/pngpread.c \
    libs/png/pngread.c \
    libs/png/pngrio.c \
    libs/png/pngrtran.c \
    libs/png/pngrutil.c \
    libs/png/pngset.c \
    libs/png/pngtrans.c \
    libs/png/pngvcrd.c \
    libs/minutf8/minutf8.c \
    libs/freetype/autofit/autofit.c \
    libs/freetype/base/basepic.c \
    libs/freetype/base/ftapi.c \
    libs/freetype/base/ftbase.c \
    libs/freetype/base/ftbbox.c \
    libs/freetype/base/ftbitmap.c \
    libs/freetype/base/ftdbgmem.c \
    libs/freetype/base/ftdebug.c \
    libs/freetype/base/ftglyph.c \
    libs/freetype/base/ftinit.c \
    libs/freetype/base/ftpic.c \
    libs/freetype/base/ftstroke.c \
    libs/freetype/base/ftsynth.c \
    libs/freetype/base/ftsystem.c \
    libs/freetype/cff/cff.c \
    libs/freetype/pshinter/pshinter.c \
    libs/freetype/psnames/psnames.c \
    libs/freetype/raster/raster.c \
    libs/freetype/sfnt/sfnt.c \
    libs/freetype/smooth/smooth.c \
    libs/freetype/truetype/truetype.c \
	src/edify/lex.yy.c \
	src/edify/parser.c \
	src/edify/expr.c \
    src/libs/miui_array.c \
    src/libs/miui_freetype.c \
    src/libs/miui_graph.c \
    src/libs/miui_input.c \
    src/libs/miui_languages.c \
    src/libs/miui_libs.c \
    src/libs/miui_memory.c \
    src/libs/miui_png.c \
    src/controls/miui_control_button.c \
    src/controls/miui_control_check.c \
    src/controls/miui_control_optbox.c \
    src/controls/miui_control_textbox.c \
    src/controls/miui_control_checkbox.c \
    src/controls/miui_control_menubox.c \
    src/controls/miui_control_sdmenu.c \
    src/controls/miui_control_title.c \
    src/controls/miui_controls.c \
    src/controls/miui_control_threads.c \
    src/main/miui_ui.c \
    src/main/common_ui.c \
    src/main/lang_ui.c \
    src/main/back_ui.c \
    src/main/sd_ui.c \
    src/main/power_ui.c \
    src/main/mount_ui.c \
    src/main/wipe_ui.c \
    src/main/backup_ui.c \
    src/main/info_ui.c \
    src/main/tool_ui.c \
    src/main/menu_node.c \
    src/main/miui_installer.c \
    src/main/miui.c

LOCAL_SRC_FILES := \
    $(libmiui_common_src_files)
LOCAL_C_INCLUDES += $(libmiui_common_includes)
LOCAL_CFLAGS := $(MYDEFINE_CFLAGS)
LOCAL_STATIC_LIBRARIES += libc libm
LOCAL_MODULE := libmiui

OBJS := $($(LOCAL_SRC_FILES),.c=.o)
DEPS := $($(LOCAL_SRC_FILES),.c=.d)
sinclude $(DEPS)
$(LOCAL_MODULE):
	$(TARGET_BUILD_CC) -shared -o $(LOCAL_MODULE).a 
%.d: %.c
	@echo updating dependency file $@ ...
	@set -e; rm -f $@;\
	$(TARGET_BUILD_CC) -MM $< > $@.$$$$;\
	set 's,\($*\).o[ :]*,\1.0 $@ : ,g' < $@.$$$$ > $@;\
	rm -f $@.$$$$;
