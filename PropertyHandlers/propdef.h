
// IMPORTANT ------------------------------------------------------------------
// When you add a new property, you have to add matching information
// in the following three places
// 1. propdef.h: g_rgPROPERTYMAP
// 2. pkeys.h: TODO make this auto generated
// 3. DicomProperties.propdesc: TODO make this auto generated
// ----------------------------------------------------------------------------

//
// Map of property keys to the locations of their value(s) in the .recipe XML schema
//
PROPERTYMAP g_rgPROPERTYMAP[] =
{
    { PKEY_BYU_DicomShell_TransferSyntax, DCM_METATRANSFERSYNTAX,	0, 1, 0 },

    { PKEY_BYU_DicomShell_ImageType, 	DCM_IDIMAGETYPE,            1, 0, 0 },
    { PKEY_BYU_DicomShell_InstName, 	DCM_IDINSTITUTIONNAME,      1, 0, 0 },
    { PKEY_BYU_DicomShell_StudyDesc, 	DCM_IDSTUDYDESCRIPTION,     1, 0, 0 },
    { PKEY_BYU_DicomShell_RefPhysician, DCM_IDREFERRINGPHYSICIAN,   1, 0, 0 },
    { PKEY_BYU_DicomShell_ImageDate, 	DCM_IDIMAGEDATE,            1, 0, 1 },
    { PKEY_BYU_DicomShell_SopClassUID, 	DCM_IDSOPCLASSUID,          0, 1, 0 },
    { PKEY_BYU_DicomShell_ManufacturerModel, DCM_IDMANUFACTURERMODEL, 1, 0, 0 },

    { PKEY_BYU_DicomShell_PatName, 	    DCM_PATNAME,                1, 0, 0 },
    { PKEY_BYU_DicomShell_PatID, 	    DCM_PATID,                  1, 0, 0 },
    { PKEY_BYU_DicomShell_PatBirthDate, DCM_PATBIRTHDATE,           1, 0, 1 },
    { PKEY_BYU_DicomShell_PatGender, 	DCM_PATSEX,                 1, 0, 0 },

    { PKEY_BYU_DicomShell_TransducerData, DCM_ACQTRANSDUCERDATA,    1, 0, 0 },

// Define the elements for the IMAGE PRESENTATION group (0028)
    { PKEY_BYU_DicomShell_SamplesPerPixel, DCM_IMGSAMPLESPERPIXEL,  0, 0, 0 },
    { PKEY_BYU_DicomShell_Photometric,  DCM_IMGPHOTOMETRICINTERP,   0, 0, 0 },
    { PKEY_BYU_DicomShell_NumFrames,    DCM_IMGNUMBEROFFRAMES,      0, 0, 0 },
    { PKEY_BYU_DicomShell_ImgRows, 	    DCM_IMGROWS,                0, 0, 0 },
    { PKEY_BYU_DicomShell_ImgColumns, 	DCM_IMGCOLUMNS,             0, 0, 0 },
    { PKEY_BYU_DicomShell_ImgPlanes,    DCM_IMGPLANES,              0, 0, 0 },
    { PKEY_BYU_DicomShell_BitsAllocated, DCM_IMGBITSALLOCATED,      0, 0, 0 },
    { PKEY_BYU_DicomShell_BitsStored,   DCM_IMGBITSSTORED,          0, 0, 0 },
    { PKEY_BYU_DicomShell_HighBit,      DCM_IMGHIGHBIT,             0, 0, 0 },
    { PKEY_BYU_DicomShell_PixelRepresentation, DCM_IMGPIXELREPRESENTATION, 0, 0, 0 },

// Philips private tags (200d)
    { PKEY_BYU_DicomShell_PhImageFormat, DMM_PHIMAGEFORMAT,         0, 0, 0 },
};

