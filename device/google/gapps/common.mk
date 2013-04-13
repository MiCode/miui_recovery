# Copy Google files
PRODUCT_COPY_FILES += \
    device/google/gapps/etc/permissions/com.google.android.maps.xml:system/etc/permissions/com.google.android.maps.xml \
    device/google/gapps/etc/permissions/com.google.android.media.effects.xml:system/etc/permissions/com.google.android.media.effects.xml \
    device/google/gapps/etc/permissions/com.google.widevine.software.drm.xml:system/etc/permissions/com.google.widevine.software.drm.xml \
    device/google/gapps/etc/permissions/features.xml:system/etc/permissions/features.xml \
    device/google/gapps/framework/com.google.android.maps.jar:system/framework/com.google.android.maps.jar \
    device/google/gapps/framework/com.google.android.media.effects.jar:system/framework/com.google.android.media.effects.jar \
    device/google/gapps/framework/com.google.widevine.software.drm.jar:system/framework/com.google.widevine.software.drm.jar \
    device/google/gapps/lib/liblocSDK_2.5OEM.so:system/lib/liblocSDK_2.5OEM.so \
    device/google/gapps/lib/libpicowrapper.so:system/lib/libpicowrapper.so \
    device/google/gapps/lib/libspeexresampler.so:system/lib/libspeexresampler.so \
    device/google/gapps/lib/libspeexwrapper.so:system/lib/libspeexwrapper.so \
    device/google/gapps/lib/libvoicesearch.so:system/lib/libvoicesearch.so

# Google core packages
PRODUCT_PACKAGES += \
    ChromeBookmarksSyncAdapter \
    GoogleBackupTransport \
    GoogleCalendarSyncAdapter \
    GoogleContactsSyncAdapter \
    GoogleLoginService \
    GooglePartnerSetup \
    GoogleServicesFramework \
    NetworkLocation
