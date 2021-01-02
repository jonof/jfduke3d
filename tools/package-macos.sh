#!/bin/bash

APPLEID=appledev@jonof.id.au
APPLEIDPW=@keychain:appledev-altool
PRODUCT=jfduke3d
BUNDLEID=au.id.jonof.$PRODUCT
VERSION=$(date +%Y%m%d)

if [ "$1" = "build" ]; then
    set -xe

    # Clean everything including JFAudioLib's ogg/vorbis builds.
    (cd xcode && xcrun xcodebuild -target all clean)
    rm -rf jfaudiolib/third-party/osx/out

    # Configure code signing.
    cat >xcode/Signing.xcconfig <<EOT
CODE_SIGN_IDENTITY = Developer ID Application
CODE_SIGN_STYLE = Manual
DEVELOPMENT_TEAM = S7U4E54CHC
CODE_SIGN_INJECT_BASE_ENTITLEMENTS = NO
OTHER_CODE_SIGN_FLAGS = --timestamp
EOT

    # Set the build version.
    cat >xcode/Version.xcconfig <<EOT
CURRENT_PROJECT_VERSION = $VERSION
EOT

    # Build away.
    (cd xcode && xcrun xcodebuild -parallelizeTargets -target all -configuration Release)

elif [ "$1" = "notarise" ]; then
    set -xe

    # Zip up the app bundles. Can't use 'zip' because it fscks with Apple's notarisation.
    ditto -c -k --sequesterRsrc xcode/build/Release notarise.zip

    # Send the zip to Apple.
    xcrun altool --notarize-app --file notarise.zip \
        --primary-bundle-id "$BUNDLEID" \
        -u "$APPLEID" -p "$APPLEIDPW"

elif [ "$1" = "notarystatus" ]; then
    if [ -z "$2" ]; then
        set -xe
        xcrun altool --notarization-history 0 -u "$APPLEID" -p "$APPLEIDPW"
    else
        set -xe
        xcrun altool --notarization-info "$2" -u "$APPLEID" -p "$APPLEIDPW"
    fi

elif [ "$1" = "finish" ]; then
    set -xe

    # Clean a previous packaging attempt.
    rm -rf $PRODUCT-$VERSION-mac $PRODUCT-$VERSION-mac.zip

    # Put all the pieces together.
    mkdir $PRODUCT-$VERSION-mac
    cp -R xcode/build/Release/*.app $PRODUCT-$VERSION-mac
    cp jfbuild/buildlic.txt $PRODUCT-$VERSION-mac
    cp GPL.TXT $PRODUCT-$VERSION-mac
    sed "s/\\\$VERSION/$VERSION/g" < releasenotes.html > $PRODUCT-$VERSION-mac/readme.html

    # Staple notary tickets to the applications.
    find $PRODUCT-$VERSION-mac -maxdepth 1 -name '*.app' -print0 | xargs -t -0 -I% xcrun stapler staple -v %

    # Zip it all up.
    ditto -c -k --sequesterRsrc --keepParent $PRODUCT-$VERSION-mac $PRODUCT-$VERSION-mac.zip

else
    echo package-macos.sh build
    echo package-macos.sh notarise
    echo package-macos.sh notarystatus '[uuid]'
    echo package-macos.sh finish
fi
