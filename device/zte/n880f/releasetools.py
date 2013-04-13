# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Emit commands needed for Prime during OTA installation
(installing the bootloader and radio images)."""

import common

def FullOTA_InstallEnd(info):
  try:
    bootloader_img = info.input_zip.read("RADIO/bootloader.img")
  except KeyError:
    print "no bootloader.img in target_files; skipping install"
  else:
    WriteBootloader(info, bootloader_img)

  try:
    radio_img = info.input_zip.read("RADIO/radio.img")
  except KeyError:
    print "no radio.img in target_files; skipping install"
  else:
    WriteRadio(info, radio_img)

  FsSizeFix(info)

def IncrementalOTA_VerifyEnd(info):
  try:
    target_radio_img = info.target_zip.read("RADIO/radio.img")
    source_radio_img = info.source_zip.read("RADIO/radio.img")
  except KeyError:
    # No source or target radio. Nothing to verify
    pass
  else:
    if source_radio_img != target_radio_img:
      info.script.CacheFreeSpaceCheck(len(source_radio_img))
      radio_type, radio_device = common.GetTypeAndDevice("/radio", info.info_dict)
      info.script.PatchCheck("%s:%s:%d:%s:%d:%s" % (
          radio_type, radio_device,
          len(source_radio_img), common.sha1(source_radio_img).hexdigest(),
          len(target_radio_img), common.sha1(target_radio_img).hexdigest()))

def IncrementalOTA_InstallEnd(info):
  try:
    target_bootloader_img = info.target_zip.read("RADIO/bootloader.img")
    try:
      source_bootloader_img = info.source_zip.read("RADIO/bootloader.img")
    except KeyError:
      source_bootloader_img = None

    if source_bootloader_img == target_bootloader_img:
      print "bootloader unchanged; skipping"
    else:
      WriteBootloader(info, target_bootloader_img)
  except KeyError:
    print "no bootloader.img in target target_files; skipping install"

  try:
    target_radio_img = info.target_zip.read("RADIO/radio.img")
    try:
      source_radio_img = info.source_zip.read("RADIO/radio.img")
    except KeyError:
      source_radio_img = None

    WriteRadio(info, target_radio_img, source_radio_img)
  except KeyError:
    print "no radio.img in target target_files; skipping install"

  FsSizeFix(info)

def FsSizeFix(info):
  info.script.Print("Fixing fs_size in crypto footer...")
  info.script.AppendExtra('''assert(samsung.fs_size_fix());''')

def WriteBootloader(info, bootloader_img):
  common.ZipWriteStr(info.output_zip, "bootloader.img", bootloader_img)
  fstab = info.info_dict["fstab"]

  info.script.Print("Writing bootloader...")
  info.script.AppendExtra('''assert(samsung.write_bootloader(
    package_extract_file("bootloader.img"), "%s", "%s"));''' % \
    (fstab["/xloader"].device, fstab["/sbl"].device))

def WriteRadio(info, target_radio_img, source_radio_img=None):
  tf = common.File("radio.img", target_radio_img)
  if source_radio_img is None:
    tf.AddToZip(info.output_zip)
    info.script.Print("Writing radio...")
    info.script.WriteRawImage("/radio", tf.name)
  else:
    sf = common.File("radio.img", source_radio_img);
    if tf.sha1 == sf.sha1:
      print "radio image unchanged; skipping"
    else:
      diff = common.Difference(tf, sf)
      common.ComputeDifferences([diff])
      _, _, d = diff.GetPatch()
      if d is None or len(d) > tf.size * common.OPTIONS.patch_threshold:
        # computing difference failed, or difference is nearly as
        # big as the target:  simply send the target.
        tf.AddToZip(info.output_zip)
        info.script.Print("Writing radio...")
        info.script.WriteRawImage("/radio", tf.name)
      else:
        common.ZipWriteStr(info.output_zip, "radio.img.p", d)
        info.script.Print("Patching radio...")
        radio_type, radio_device = common.GetTypeAndDevice("/radio", info.info_dict)
        info.script.ApplyPatch(
            "%s:%s:%d:%s:%d:%s" % (radio_type, radio_device,
                                   sf.size, sf.sha1, tf.size, tf.sha1),
            "-", tf.size, tf.sha1, sf.sha1, "radio.img.p")
