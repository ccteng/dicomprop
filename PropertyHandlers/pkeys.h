
// #define INITGUID define this in one c file only
#include <propkeydef.h>

#define DPK(key, id) DEFINE_PROPERTYKEY(key, 0xa691c6de, 0x235c, 0x4b41, 0x88, 0xa9, 0x51, 0xf8, 0x87, 0xc, 0x82, 0x67, id)

// Define the elements in the File Meta Header Group (0x0002)
DPK(PKEY_BYU_DicomShell_TransferSyntax, DCM_METATRANSFERSYNTAX); // 0x0010

// Define the elements in the IDENTIFYING group, 0008
DPK(PKEY_BYU_DicomShell_ImageType, DCM_IDIMAGETYPE); // 0x0008
DPK(PKEY_BYU_DicomShell_InstName, DCM_IDINSTITUTIONNAME); // 0x0080
DPK(PKEY_BYU_DicomShell_StudyDesc, DCM_IDSTUDYDESCRIPTION); // 0x1030
DPK(PKEY_BYU_DicomShell_RefPhysician, DCM_IDREFERRINGPHYSICIAN); // 0x0090
DPK(PKEY_BYU_DicomShell_ImageDate, DCM_IDIMAGEDATE); // 0x0023
DPK(PKEY_BYU_DicomShell_SopClassUID, DCM_IDSOPCLASSUID); // 0x0016
DPK(PKEY_BYU_DicomShell_ManufacturerModel, DCM_IDMANUFACTURERMODEL); // 0x1090

// Define the elements in the PATIENT INFORMATION group (0x0010)
DPK(PKEY_BYU_DicomShell_PatName, DCM_PATNAME); // 0x0010
DPK(PKEY_BYU_DicomShell_PatID, DCM_PATID); // 0x0020
DPK(PKEY_BYU_DicomShell_PatBirthDate, DCM_PATBIRTHDATE); // 0x0030
DPK(PKEY_BYU_DicomShell_PatGender, DCM_PATSEX); // 0x0040

// Define the elements in the ACQUISITION INFORMATION group (0018)
DPK(PKEY_BYU_DicomShell_TransducerData, DCM_ACQTRANSDUCERDATA); // 0x5010

// Define the elements for the IMAGE PRESENTATION group (0028)
DPK(PKEY_BYU_DicomShell_SamplesPerPixel, DCM_IMGSAMPLESPERPIXEL); // 0x0002
DPK(PKEY_BYU_DicomShell_Photometric, DCM_IMGPHOTOMETRICINTERP); // 0x0004
DPK(PKEY_BYU_DicomShell_NumFrames, DCM_IMGNUMBEROFFRAMES); // 0x0008
DPK(PKEY_BYU_DicomShell_ImgRows, DCM_IMGROWS); // 0x0010
DPK(PKEY_BYU_DicomShell_ImgColumns, DCM_IMGCOLUMNS); // 0x0011
DPK(PKEY_BYU_DicomShell_ImgPlanes, DCM_IMGPLANES); // 0x0012
DPK(PKEY_BYU_DicomShell_BitsAllocated, DCM_IMGBITSALLOCATED); // 0x0100
DPK(PKEY_BYU_DicomShell_BitsStored, DCM_IMGBITSSTORED); // 0x0101
DPK(PKEY_BYU_DicomShell_HighBit, DCM_IMGHIGHBIT); // 0x0102
DPK(PKEY_BYU_DicomShell_PixelRepresentation, DCM_IMGPIXELREPRESENTATION); // 0x0103

// Philips private tags (200d)
DPK(PKEY_BYU_DicomShell_PhImageFormat, DMM_PHIMAGEFORMAT); // 0x2005

// #undef INITGUID

