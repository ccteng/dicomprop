reg.exe import C:\test\DicomPropertyHandler.reg
regsvr32.exe C:\test\DicomPropertyHandler.dll
rundll32.exe C:\test\DicomPropertyHandler.dll,InstallPropertySchema C:\test\DicomProperties.propdesc
