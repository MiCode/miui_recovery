1 the project is build android recovery only;
2 before building, make sure copy project android_prebuilt, android_bionic and anroid_hardware to the directory ,which from ICS.
3 Steps as following:
    $. build/envsetup.sh
    $make crespo
  please see the out/patch_device/crespo/ , find recovery.img 
4 please check Makefile to obtain the supported devices;

 
<ins>Modified by sndnvaps@gmail.com @Gaojiquan.com[] 
</ins>




[Gaojiquan](http://www.gaojiqu.com) 

2013/4/12 :
 Changelog :
     support : ZTE n909 
       
Setps to build n909 :
      ```bash 
          # build/envsetup.sh
             # make n909 
        ```
You cand find recovery.img in folder out/patch_device/n909 



