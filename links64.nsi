;NSIS Modern User Interface
;Basic Example Script
;Written by Joost Verburg

SetCompressor /SOLID lzma

;--------------------------------
;Include Modern UI

  !include "MUI.nsh"

;--------------------------------
;General

  ;Name and file
  Name "Links WWW Browser"
  ;Icon "links.ico"
  ;!define MUI_ICON "links.ico"
  OutFile "Links-64bit-install.exe"

  ;Default installation folder
  InstallDir "$PROGRAMFILES64\Links"
  
  ;Get installation folder from registry if available
  InstallDirRegKey HKCU "Software\Links" ""

  RequestExecutionLevel admin

  Var MUI_TEMP
  Var STARTMENU_FOLDER

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_LICENSE "COPYING"
;  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY

!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKCU" 
!define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\Links" 
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Links"
  
!insertmacro MUI_PAGE_STARTMENU Application $STARTMENU_FOLDER

  !insertmacro MUI_PAGE_INSTFILES
  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  
;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Installer Sections

Section "-Default Links browser files" DefaultSection

  SetOutPath "$INSTDIR"
  
  ;ADD YOUR OWN FILES HERE...

File BRAILLE_HOWTO
File COPYING
File KEYS
File README
File links.exe
File links-g.exe
File c:\cygwin64\bin\cygbz2-1.dll
File c:\cygwin64\bin\cygcrypto-1.0.0.dll
File c:\cygwin64\bin\cygjpeg-8.dll
File c:\cygwin64\bin\cyglzma-5.dll
File c:\cygwin64\bin\cygpng15-15.dll
File c:\cygwin64\bin\cygssl-1.0.0.dll
File c:\cygwin64\bin\cygtiff-6.dll
File c:\cygwin64\bin\cygwin1.dll
File c:\cygwin64\bin\cygz.dll
  
  ;Store installation folder
  WriteRegStr HKCU "Software\Links" "" $INSTDIR
  
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

!insertmacro MUI_STARTMENU_WRITE_BEGIN Application

CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER"
CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Links.lnk" "$INSTDIR\Links.exe"
CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Links Graphics.lnk" "$INSTDIR\Links-g.exe" "-g"
CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
!insertmacro MUI_STARTMENU_WRITE_END
SectionEnd


;--------------------------------
;Uninstaller Section

Section "Uninstall"

  ;ADD YOUR OWN FILES HERE...
Delete "$INSTDIR\BRAILLE_HOWTO"
Delete "$INSTDIR\COPYING"
Delete "$INSTDIR\KEYS"
Delete "$INSTDIR\README"
Delete "$INSTDIR\links.exe"
Delete "$INSTDIR\links-g.exe"
Delete "$INSTDIR\cygbz2-1.dll"
Delete "$INSTDIR\cygcrypto-1.0.0.dll"
Delete "$INSTDIR\cygjpeg-8.dll"
Delete "$INSTDIR\cyglzma-5.dll"
Delete "$INSTDIR\cygpng15-15.dll"
Delete "$INSTDIR\cygssl-1.0.0.dll"
Delete "$INSTDIR\cygtiff-6.dll"
Delete "$INSTDIR\cygwin1.dll"
Delete "$INSTDIR\cygz.dll"
Delete "$INSTDIR\.links\*"
RMDir "$INSTDIR\.links"

  Delete "$INSTDIR\Uninstall.exe"

  RMDir "$INSTDIR"

!insertmacro MUI_STARTMENU_GETFOLDER Application $MUI_TEMP

Delete "$SMPROGRAMS\$MUI_TEMP\Uninstall.lnk"
Delete "$SMPROGRAMS\$MUI_TEMP\Links.lnk"
Delete "$SMPROGRAMS\$MUI_TEMP\Links Graphics.lnk"
StrCpy $MUI_TEMP "$SMPROGRAMS\$MUI_TEMP"

startMenuDeleteLoop:
ClearErrors
RMDir $MUI_TEMP
GetFullPathName $MUI_TEMP "$MUI_TEMP\.."

IfErrors startMenuDeleteLoopDone

StrCmp $MUI_TEMP $SMPROGRAMS startMenuDeleteLoopDone startMenuDeleteLoop
startMenuDeleteLoopDone:

  DeleteRegKey /ifempty HKCU "Software\Links"

SectionEnd
