
To install:

1. Build the DLL in Visual Studio.
2. Copy DLL, DicomProperties.propdesc, and DicomPropertyHandler.reg to a directory on your machine.
3. Execute the following commands from an elevated command prompt (right-click cmd.exe -> Run as Administrator):
    a. 'reg.exe import C:\Path\To\Your\Files\DicomPropertyHandler.reg'  (registers the .dcm filetype)
    b. 'regsvr32.exe C:\Path\To\Your\Files\DicomPropertyHandler.dll'  (registers the property handler with COM and for the .dcm filetype)
    c. 'rundll32.exe C:\Path\To\Your\Files\DicomPropertyHandler.dll,InstallPropertySchema C:\Path\To\Your\Files\DicomProperties.propdesc'  (registers custom .dcm properties with the system schema; NOTE: you must specify the full path to the .propdesc file)

See install.cmd for example.