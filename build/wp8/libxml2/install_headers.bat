SET curdir=%CD%
SET incdir=..\..\..\..\libxml2\include\libxml
SET installdir=%1\libxml

Xcopy /I /Y %incdir%\*.h %installdir%\
Xcopy /I /Y xmlversion.h %installdir%\
