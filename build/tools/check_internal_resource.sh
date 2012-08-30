#!/bin/bash

# Add the APP name (the value of LOCAL_PACKAGE_NAME defined in local android.mk)
# here if want to prevent it using the internal resources
apps_need_to_check=" \
    AntiSpam \
    Backup \
    BugReport \
    Calculator \
    Calendar \
    CalendarProvider \
    CloudService \
    Contacts \
    ContactsProvider \
    DeskClock \
    DownloadProvider \
    Email \
    FileExplorer \
    MiuiCompass \
    MiuiGallery \
    MiuiHome \
    MiuiPhone \
    MiuiSystemUI \
    Mms \
    Monitor \
    Music \
    Notes \
    PackageInstaller \
    QuickSearchBox \
    SoundRecorder \
    SuperMarket \
    TelephonyProvider \
    TelocationProvider \
    ThemeManager \
    Updater \
"

declare -A jars_need_to_check
jars_need_to_check=(\
                    [framework]="core"  \
               [android.policy]="policy" \
                     [services]="services" \
                   )

module=$1

if [ -z "$2" ]; then
    src_path=${jars_need_to_check[$module]}
    if [ -z "$src_path" ]; then
        echo "Jar module $module is allowed to use internal resources."
        exit 0
    fi
    xml_path=frameworks/miui/$src_path
    src_path=frameworks/miui/$src_path
else
    need_to_check=`echo "$apps_need_to_check" | sed -n -e "/ $module /p"`
    if [ -z "$need_to_check" ]; then
        echo "Package module $module is allowed to use internal resources."
        exit 0
    fi
    xml_path=$2/res
    src_path=$2/src
fi

xml_internal=`find $xml_path \( -name alias.xml -prune \) -o \( -name "*.xml" \) -exec grep -H -n "\@\*android" {} \;`

java_internal=`find $src_path -name "*.java" -exec grep -H -n com.android.internal.R {} \;`

if [ -n "$java_internal" -o -n "$xml_internal" ]; then
        echo "Error: internal resources are used in following files:" >&2
        if [ -n "$java_internal" ]; then
                echo "$java_internal"| sed -e "s/\s\+/ /g" >&2
        fi
        if [ -n "$xml_internal" ] ; then
                echo "$xml_internal" | sed -e "s/\s\+/ /g" >&2
        fi
        exit 1
fi

echo "Module $module: Internal resources using check is passed."
exit 0

