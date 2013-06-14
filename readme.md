1 the project is build android recovery only;

2 before building, make sure copy project <b>android_prebuilt</b>,<del> <b>android_bionic</b></del> and <b>anroid_hardware</b> to the directory ,which from ICS or higher 

3 Steps as following:
 <code>
    $. build/envsetup.sh
    $make crespo
</code>
  please see the out/patch_device/crespo/ , find recovery.img
 
4 please check Makefile to obtain the supported devices;

 
<h1><ins>Modified by sndnvaps@gmail.com </ins></h1>



[http://www.gaojiquan.com](http://www.gaojiqu.com "Gaojiquan.com") 



<h2> Changelog :</h2> 
<h3>2013/4/12 </h3>
     support : ZTE n909 

***Setps to build n909 :***

      <code>
          # build/envsetup.sh
          # lunch full_n909-eng
          # make && make n909 
      </code>

<h3>2013/06/14 </h3>
     support: ZTE N880F
     
***Setps to build n880f: ***
   <code>
    # source build/envsetup.sh
    # lunch full_n880f-eng
    # make && make n880f
  </code>
You cand find recovery.img in folder out/patch_device/device_name
 
*Notification*

   support usb-toggle for internal sdcard && sdcard 
   support ORS scripts , 
   support root device func 
   support to disable to restore the official Recovery from install.zip


<h1> Get the android_bionic && external/busybox && external/bash  submodule </h1>
    <code>
    $ cd MIUI_Recovery_N909 
    $ git submodule init 
    $ git submodule update 
   </code>


