

// Releases the specified pointer if not NULL
#define SAFE_RELEASE(p) \
if (p)                  \
{                       \
    (p)->Release();     \
    (p) = NULL;         \
}

// private tags not defined in dicom_objects.h
// Philips
#define DMM_PHIMAGEFORMAT DCM_MAKETAG(0x200d,0x2005) // Philips image format

//
// Map of property keys to the locations of their value(s) in the .recipe XML schema
//
struct PROPERTYMAP
{
    REFPROPERTYKEY key;
	DCM_TAG tag;
	unsigned short bWritable:1;
	unsigned short bLookupUID:1;
	unsigned short bDate:1;
};

extern PROPERTYMAP g_rgPROPERTYMAP[];

//
// Helper functions to opaquely serialize and deserialize PROPVARIANT values to and from string form
//
HRESULT SerializePropVariantAsString(REFPROPVARIANT propvar, PWSTR *pszOut);
HRESULT DeserializePropVariantFromString(PCWSTR pszIn, PROPVARIANT *ppropvar);


//
// DLL lifetime management functions
//
void DllAddRef();
void DllRelease();

