/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <dumpstate.h>

void dumpstate_board()
{
    dump_file("board revision", "/sys/board_properties/revision");
    dump_file("soc family", "/sys/board_properties/soc/family");
    dump_file("soc revision", "/sys/board_properties/soc/revision");
    dump_file("soc type", "/sys/board_properties/soc/type");
    dump_file("soc die_id", "/sys/board_properties/soc/die_id");
    dump_file("soc production_id", "/sys/board_properties/soc/production_id");
    dump_file("pm_debug count", "/d/pm_debug/count");
    dump_file("pm_debug time", "/d/pm_debug/time");
    dump_file("dsscomp_log", "/d/dsscomp/log");
    dump_file("dsscomp_comps", "/d/dsscomp/comps");
    dump_file("dsscomp_gralloc", "/d/dsscomp/gralloc");
    dump_file("audio media state", "/d/asoc/Tuna/SDP4430\ Media/state");
    dump_file("audio modem state", "/d/asoc/Tuna/SDP4430\ MODEM/state");
    dump_file("audio codec_reg", "/sys/devices/platform/soc-audio/PDM-DL1/codec_reg");
    dump_file("ducati firmware version", "/d/remoteproc/omap-rproc.1/version");
    dump_file("ducati trace1", "/d/remoteproc/omap-rproc.1/trace1");
    dump_file("ducati trace1_last", "/d/remoteproc/omap-rproc.1/trace1_last");
    dump_file("fsa9480 device_type", "/sys/bus/i2c/drivers/fsa9480/4-0025/device_type");
    dump_file("fsa9480 control", "/sys/bus/i2c/drivers/fsa9480/4-0025/control");
    dump_file("tiler 2x1 map", "/d/tiler/map/2x1");
};
