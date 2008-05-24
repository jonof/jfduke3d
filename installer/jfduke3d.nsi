!define PRODUCT_NAME "JFDuke3D"
!define PRODUCT_VERSION "20051009"
!define PRODUCT_PUBLISHER "JonoF"
!define PRODUCT_WEB_SITE "http://www.jonof.id.au/index.php?p=jfduke3d"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\${PRODUCT_NAME}"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"
!define PRODUCT_STARTMENU_REGVAL "NSIS:StartMenuDir"

SetCompressor lzma
Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "jfduke3d-${PRODUCT_VERSION}-setup.exe"
InstallDir "$PROGRAMFILES\${PRODUCT_NAME}"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show
RequestExecutionLevel user
LicenseData "..\GNU.TXT"
XPStyle on

Page license
Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

Section "!Game (required)"
	SectionIn RO
	SetOutPath $INSTDIR

	File "..\duke3d.exe"
	File "..\setup.bat"
	File "..\GNU.TXT"
	File "..\readme.txt"
	File "..\releasenotes.html"
	#File "E:\ports\datainst\datainst.exe"

	CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
	CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\JFDuke3D.lnk" "$INSTDIR\duke3d.exe"
	CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\JFDuke3D Setup.lnk" "$INSTDIR\duke3d.exe" "-setup"
	CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Game Data Installer.lnk" "$INSTDIR\datainst.exe"
SectionEnd

Section /o "Map editor"
	SetOutPath $INSTDIR

	File "..\build.exe"
	File "..\build-setup.bat"

	CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
	CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Build Editor.lnk" "$INSTDIR\build.exe"
	CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Build Editor Setup.lnk" "$INSTDIR\build.exe" "-setup"
SectionEnd

Section "Per-user profiles (recommended)"
	ClearErrors
	FileOpen $0 $INSTDIR\user_profiles_enabled w
	IfErrors done
	FileWrite $0 "User profile data is stored in C:\Documents and Settings\<your username>\Application Data\${PRODUCT_NAME}"
	FileClose $0
	done:
SectionEnd

Section -AdditionalIcons
	WriteIniStr "$INSTDIR\${PRODUCT_NAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"

	CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\JFDuke3D Website.lnk" "$INSTDIR\${PRODUCT_NAME}.url"
	CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk" "$INSTDIR\uninst.exe"
SectionEnd

Section -Post
	WriteUninstaller "$INSTDIR\uninst.exe"

	WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\duke3d.exe"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\duke3d.exe"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "${PRODUCT_STARTMENU_REGVAL}" "${PRODUCT_NAME}"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
	WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
	WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NoModify" 1
	WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NoRepair" 1
SectionEnd

Section "Run data installer when done"
	SetOutPath $INSTDIR
	Exec '"$INSTDIR\datainst.exe"'
SectionEnd

Section Uninstall
	Delete "$INSTDIR\duke3d.exe"
	Delete "$INSTDIR\setup.bat"
	Delete "$INSTDIR\GNU.TXT"
	Delete "$INSTDIR\readme.txt"
	Delete "$INSTDIR\releasenotes.html"
	Delete "$INSTDIR\datainst.exe"

	Delete "$INSTDIR\build.exe"
	Delete "$INSTDIR\build-setup.bat"

	Delete "$SMPROGRAMS\${PRODUCT_NAME}\JFDuke3D.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\JFDuke3D Setup.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Game Data Installer.lnk"

	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Build Editor.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Build Editor Setup.lnk"

	Delete "$INSTDIR\${PRODUCT_NAME}.url"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\JFDuke3D Website.lnk"
	Delete "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk"

	Delete "$INSTDIR\duke3d.grp"
	Delete "$INSTDIR\duke3d.cfg"
	Delete "$INSTDIR\build.cfg"
	Delete "$INSTDIR\user_profiles_enabled"

	Delete "$INSTDIR\uninst.exe"

	RMDir "$SMPROGRAMS\${PRODUCT_NAME}"
	RMDir "$INSTDIR"

	DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
	DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
	SetAutoClose true
SectionEnd

