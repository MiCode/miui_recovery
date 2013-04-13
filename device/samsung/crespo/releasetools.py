# Copyright (C) 2010 The Android Open Source Project
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

import sha
import re

import common

def FindRadio(zipfile):
  matches = []
  for name in zipfile.namelist():
    if re.match(r"^RADIO/radio[.](.+[.])?img$", name):
      matches.append(name)
  if len(matches) > 1:
    raise ValueError("multiple radio images in target-files zip!")
  if matches:
    print "using %s as radio.img" % (matches[0],)
    return zipfile.read(matches[0])
  else:
    return None

def FullOTA_InstallEnd(info):
  try:
    bootloader_img = info.input_zip.read("RADIO/bootloader.img")
  except KeyError:
    print "no bootloader.img in target_files; skipping install"
  else:
    common.ZipWriteStr(info.output_zip, "bootloader.img", bootloader_img)
    info.script.Print("Writing bootloader...")
    info.script.WriteRawImage("/bootloader", "bootloader.img")

  radio_img = FindRadio(info.input_zip)
  if not radio_img:
    print "no radio.img in target_files; skipping install"
  else:
    common.ZipWriteStr(info.output_zip, "radio.img", radio_img)
    info.script.Print("Writing radio...")
    info.script.WriteRawImage("/radio", "radio.img")

def IncrementalOTA_VerifyEnd(info):
  target_radio_img = FindRadio(info.target_zip)
  source_radio_img = FindRadio(info.source_zip)
  if not target_radio_img or not source_radio_img: return
  if source_radio_img != target_radio_img:
    info.script.CacheFreeSpaceCheck(len(source_radio_img))
    radio_type, radio_device = common.GetTypeAndDevice("/radio", info.info_dict)
    info.script.PatchCheck("%s:%s:%d:%s:%d:%s" % (
        radio_type, radio_device,
        len(source_radio_img), sha.sha(source_radio_img).hexdigest(),
        len(target_radio_img), sha.sha(target_radio_img).hexdigest()))

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
      common.ZipWriteStr(info.output_zip, "bootloader.img", target_bootloader_img)
      info.script.Print("Writing bootloader...")
      info.script.WriteRawImage("/bootloader", "bootloader.img")

  except KeyError:
    print "no bootloader.img in target target_files; skipping install"


  tf = FindRadio(info.target_zip)
  if not tf:
    # failed to read TARGET radio image: don't include any radio in update.
    print "no radio.img in target target_files; skipping install"
  else:
    tf = common.File("radio.img", tf)

    sf = FindRadio(info.source_zip)
    if not sf:
      # failed to read SOURCE radio image: include the whole target
      # radio image.
      tf.AddToZip(info.output_zip)
      info.script.Print("Writing radio...")
      info.script.WriteRawImage("/radio", tf.name)
    else:
      sf = common.File("radio.img", sf)

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
          info.script.WriteRawImage("radio", tf.name)
        else:
          common.ZipWriteStr(info.output_zip, "radio.img.p", d)
          info.script.Print("Patching radio...")
          radio_type, radio_device = common.GetTypeAndDevice(
              "/radio", info.info_dict)
          info.script.ApplyPatch(
              "%s:%s:%d:%s:%d:%s" % (radio_type, radio_device,
                                     sf.size, sf.sha1, tf.size, tf.sha1),
              "-", tf.size, tf.sha1, sf.sha1, "radio.img.p")
