# ByteScout Lossless Video Codec

Copyright (c) 2016, [ByteScout](https://bytescout.com/)

## Installation instruction


### Installing the codec manually by hand:
* Make sure that `BytescoutLosslessCodec.inf` and `BytescoutLosslessCodec.dll` are located in the same folder
* Right-click with mouse on `ByteScoutLoslessCodec.inf` and select `Install` in the popup menu

### To install the codec from software

* Put BytescoutLosslessCodec.inf and BytescoutLosslessCodec.dll into the same folder
* Run the following command line (beware of numbers, dots, commas and do not change the order or remove them):
`rundll32.exe setupapi,InstallHinfSection DefaultInstall 132 .\BytescoutLosslessCodec.inf`

### Removing the codec
* use Add/Remove Programs to remove the codec
