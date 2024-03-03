#!/bin/bash

APPLEID=appledev@jonof.id.au
KEYCHAINPROFILE=default
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

    # Zip up the app bundles.
    mkdir -p xcode/build/Release/notarise
    cp -R xcode/build/Release/*.app xcode/build/Release/notarise
    ditto -c -k --sequesterRsrc xcode/build/Release/notarise notarise.zip
    rm -rf xcode/build/Release/notarise

    # Send the zip to Apple.
    xcrun notarytool submit -p $KEYCHAINPROFILE --wait notarise.zip

elif [ "$1" = "notarystatus" ]; then
    if [ -z "$2" ]; then
        set -xe
        xcrun notarytool history -p $KEYCHAINPROFILE
    else
        set -xe
        xcrun notarytool log -p $KEYCHAINPROFILE "$2"
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
