@echo Copying codec
copy BytescoutLosslessCodec.inf %1
@echo Installing codec
cd %1
rundll32.exe setupapi,InstallHinfSection DefaultInstall 132 .\BytescoutLosslessCodec.inf
