//
//

#include <shobjidl.h>    // IInitializeWithStream, IDestinationStreamFactory
#include <propsys.h>     // Property System APIs and interfaces
#include <propkey.h>     // System PROPERTYKEY definitions
#include <propvarutil.h> // PROPVARIANT and VARIANT helper APIs
#include <msxml6.h>      // DOM interfaces
#include <comutil.h>     // _bstr_t
#include <wincrypt.h>    // CryptBinaryToString, CryptStringToBinary
#include <strsafe.h>     // StringCchPrintf

#include "ctn-3.0.6\dicom.h"
#include "ctn-3.0.6\dicom_uids.h"
#include "ctn-3.0.6\ctnthread.h"
#include "ctn-3.0.6\lst.h"
#include "ctn-3.0.6\condition.h"
#include "ctn-3.0.6\dicom_objects.h"
#include "ctn-3.0.6\utility.h"

#include "DicomPropertyHandler.h"

// IMPORTANT ------------------------------------------------------------------
// When you add a new property, you have to add matching information
// in the following three places
// 1. propdef.h: g_rgPROPERTYMAP
// 2. pkeys.h: TODO make this auto generated
// 3. DicomProperties.propdesc: TODO make this auto generated
// ----------------------------------------------------------------------------

//
// Define PROPERTYKEYs for any custom properties defined in DicomProperties.propdesc
//
#define INITGUID
#include "pkeys.h"
#undef INITGUID

//
// Map of property keys to the locations of their value(s) in the .recipe XML schema
//
#include "propdef.h"

//
// Dicom property handler class definition
//
class CDicomPropertyStore :
    public IPropertyStore,
    public IPropertyStoreCapabilities,
    public IInitializeWithStream
{
public:
    static HRESULT CreateInstance(REFIID riid, void **ppv);

    // IUnknown
    IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv)
    {
        static const QITAB qit[] = {
            QITABENT(CDicomPropertyStore, IPropertyStore),
            QITABENT(CDicomPropertyStore, IPropertyStoreCapabilities),
            QITABENT(CDicomPropertyStore, IInitializeWithStream),
            { 0 },
        };
        return QISearch(this, qit, riid, ppv);
    }

    IFACEMETHODIMP_(ULONG) AddRef()
    {
        DllAddRef();
        return InterlockedIncrement(&_cRef);
    }

    IFACEMETHODIMP_(ULONG) Release()
    {
        DllRelease();
        ULONG cRef = InterlockedDecrement(&_cRef);
        if (!cRef)
        {
            delete this;
        }
        return cRef;
    }

    // IPropertyStore
    IFACEMETHODIMP GetCount(__out DWORD *pcProps);
    IFACEMETHODIMP GetAt(DWORD iProp, __out PROPERTYKEY *pkey);
    IFACEMETHODIMP GetValue(REFPROPERTYKEY key, __out PROPVARIANT *pPropVar);
    IFACEMETHODIMP SetValue(REFPROPERTYKEY key, REFPROPVARIANT propVar);
    IFACEMETHODIMP Commit();

    // IPropertyStoreCapabilities
    IFACEMETHODIMP IsPropertyWritable(REFPROPERTYKEY key);

    // IInitializeWithStream
    IFACEMETHODIMP Initialize(IStream *pStream, DWORD grfMode);

protected:
    CDicomPropertyStore() :
        _cRef(1),
        _pStream(NULL),
        _grfMode(0),
		_pDcmObj(NULL),
        _bPart10(false),
		_bSkippedPixelData(false),
        _pCache(NULL)
    {
        DllAddRef();
    }

    ~CDicomPropertyStore()
    {
        SAFE_RELEASE(_pStream);
        SAFE_RELEASE(_pCache);

		if (_pDcmObj)
		{
			DCM_CloseObject(&_pDcmObj);
			_pDcmObj = NULL;
		}
    }

    IStream*             _pStream; // data stream passed in to Initialize, and saved to on Commit
    DWORD                _grfMode; // STGM mode passed to Initialize
    IPropertyStoreCache* _pCache;  // internal value cache to abstract IPropertyStore operations from the DOM back-end

	DCM_OBJECT*	_pDcmObj;
	bool	_bPart10;
	bool	_bSkippedPixelData;

    // helpers to load data from the stream
    HRESULT _LoadCacheFromStream(IStream* pStream);
    HRESULT _LoadProperty(const PROPERTYMAP &map);
    HRESULT _LoadSearchContent();
	int _CalcHeaderSize(IStream* pStream);
	CONDITION _ImportStream(unsigned char *buf, unsigned long length,
				 unsigned long opt, DCM_OBJECT ** callerObject);

    // helpers to save data to the stream
    HRESULT _SaveProperty(const PROPERTYMAP &map);
    HRESULT _SaveCacheToDcmObj();
    HRESULT _SaveDcmObjToStream(IStream *pStream);

private:
    long _cRef;
};


//
// Instantiates a Dicom property store object
//
HRESULT CDicomPropertyStore::CreateInstance(REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;

    CDicomPropertyStore *pNew = new CDicomPropertyStore;
    if (pNew)
    {
        hr = pNew->QueryInterface(riid, ppv);
        SAFE_RELEASE(pNew);
    }

    return hr;
}

HRESULT CDicomPropertyStore_CreateInstance(REFIID riid, void **ppv)
{
    return CDicomPropertyStore::CreateInstance(riid, ppv);
}


//
// Accessor methods forward directly to internal value cache
//
HRESULT CDicomPropertyStore::GetCount(__out DWORD *pcProps)
{
    HRESULT hr = E_UNEXPECTED;
    if (_pCache)
    {
        hr = _pCache->GetCount(pcProps);
    }

    return hr;
}

HRESULT CDicomPropertyStore::GetAt(DWORD iProp, __out PROPERTYKEY *pkey)
{
    HRESULT hr = E_UNEXPECTED;
    if (_pCache)
    {
        hr = _pCache->GetAt(iProp, pkey);
    }

    return hr;
}

HRESULT CDicomPropertyStore::GetValue(REFPROPERTYKEY key, __out PROPVARIANT *pPropVar)
{
    HRESULT hr = E_UNEXPECTED;
    if (_pCache)
    {
        hr = _pCache->GetValue(key, pPropVar);
    }

    return hr;
}


//
// SetValue just updates the internal value cache
//
HRESULT CDicomPropertyStore::SetValue(REFPROPERTYKEY key, REFPROPVARIANT propVar)
{
    HRESULT hr = E_UNEXPECTED;
    if (_pCache)
    {
        // check grfMode to ensure writes are allowed
        hr = STG_E_ACCESSDENIED;
        if (_grfMode & STGM_READWRITE &&
            !IsEqualPropertyKey(key, PKEY_Search_Contents))  // this property is read-only
        {
            hr = _pCache->SetValueAndState(key, &propVar, PSC_DIRTY);
        }
    }

    return hr;
}


//
// Commit writes the internal value cache back out to the stream passed to Initialize
//
HRESULT CDicomPropertyStore::Commit()
{
    HRESULT hr = E_UNEXPECTED;
    if (_pCache)
    {
        // check grfMode to ensure writes are allowed
        hr = STG_E_ACCESSDENIED;
        if (_grfMode & STGM_READWRITE)
        {
			// Future performance enhancement
			// 1. check old and new header size and add padding if new < old
			// 
			// reload the DICOM object including the pixels
			// 
            // save the internal value cache to DICOM object
            hr = _SaveCacheToDcmObj();
            if (SUCCEEDED(hr))
            {
                // reset the output stream
                LARGE_INTEGER liZero = {0};
                hr = _pStream->Seek(liZero, STREAM_SEEK_SET, NULL);
                if (SUCCEEDED(hr))
                {
                    // obtain a temporary destination stream for manual safe-save
                    IDestinationStreamFactory *pSafeCommit;
                    hr = _pStream->QueryInterface(IID_PPV_ARGS(&pSafeCommit));
                    if (SUCCEEDED(hr))
                    {
                        IStream *pStreamCommit;
                        hr = pSafeCommit->GetDestinationStream(&pStreamCommit);
                        if (SUCCEEDED(hr))
                        {
                            // write the DICOM object out to the temprorary stream and commit it
                            hr = _SaveDcmObjToStream(pStreamCommit);
                            if (SUCCEEDED(hr))
                            {
                                hr = pStreamCommit->Commit(STGC_DEFAULT);
                                if (SUCCEEDED(hr))
                                {
                                    // commit the real output stream
                                    _pStream->Commit(STGC_DEFAULT);
                                }
                            }

                            SAFE_RELEASE(pStreamCommit);
                        }

                        SAFE_RELEASE(pSafeCommit);
                    }
                }
            }
        }
    }

    return hr;
}


//
// Indicates whether the users should be able to edit values for the given property key
//
HRESULT CDicomPropertyStore::IsPropertyWritable(REFPROPERTYKEY key)
{
	HRESULT hr = S_OK;

    // System.Search.Contents is not supported for writing
    if (IsEqualPropertyKey(key, PKEY_Search_Contents))
	{
		hr = S_FALSE;
	}
    else if (_bSkippedPixelData)
	{
		// can't write if we can't handle the pixel data
		hr = S_FALSE;
	}
    else
    {
        for (UINT i = 0; i < ARRAYSIZE(g_rgPROPERTYMAP); ++i)
        {
            if (IsEqualPropertyKey(key, g_rgPROPERTYMAP[i].key))
            {
                if (!g_rgPROPERTYMAP[i].bWritable)
                {
                    hr = S_FALSE;
                }
                break;
            }
        }
    }

    return hr;
}


//
// Initialize populates the internal value cache with data from the specified stream
//
HRESULT CDicomPropertyStore::Initialize(IStream *pStream, DWORD grfMode)
{
    HRESULT hr = E_UNEXPECTED;
    if (!_pStream)
    {
        hr = E_POINTER;
        if (pStream)
        {
			// load the internal value cache from the input stream
			hr = _LoadCacheFromStream(pStream);
			if (SUCCEEDED(hr))
			{
				// save a reference to the stream as well as the grfMode
				hr = pStream->QueryInterface(IID_PPV_ARGS(&_pStream));
				if (SUCCEEDED(hr))
				{
					_grfMode = grfMode;
				}
			}
        }
    }

    return hr;
}

CONDITION TryImportStream(unsigned char *buf, unsigned long length,
				 unsigned long opt, DCM_OBJECT ** callerObject)
{
    CONDITION cond = DCM_FILEOPENFAILED;

    try {
        cond = DCM_ImportStream(buf, length, opt, callerObject);
    }
    catch(char *str) {
		cond = DCM_FILEOPENFAILED;
        // cout << "Caught some other exception: " << str << "\n";
    }

    return cond;
}

HRESULT CanReadPixel(DCM_OBJECT ** callerObject)
{
    HRESULT hr = E_FAIL;
    DCM_FILE_META* pDcmFM;

    // supported transfer syntax
    static const char* c_arSupportedTSUID[] = {
        DICOM_TRANSFERLITTLEENDIAN,
        DICOM_TRANSFERLITTLEENDIANEXPLICIT,
        DICOM_TRANSFERBIGENDIANEXPLICIT,
    };

	return hr;
}

CONDITION CDicomPropertyStore::_ImportStream(unsigned char *buf, unsigned long length,
				 unsigned long opt, DCM_OBJECT ** callerObject)
{
    HRESULT hr = S_OK;

    CONDITION cond = DCM_FILEOPENFAILED;
	cond = TryImportStream(buf, length, opt | DCM_PART10FILE, callerObject);
    if (cond == DCM_NORMAL)
    {
        _bPart10 = true;
    }
    else
    {
        // try open without part 10
        cond = TryImportStream(buf, length, opt, callerObject);
    }

    if (cond != DCM_NORMAL)
    {
		// try not read the pixel data
		opt |= DCM_SKIPPIXELDATA;

		cond = TryImportStream(buf, length, opt | DCM_PART10FILE, callerObject);
		if (cond == DCM_NORMAL)
		{
			_bPart10 = true;
		}
		else
		{
			// try open without part 10
			cond = TryImportStream(buf, length, opt, callerObject);
	    }

		if (cond == DCM_NORMAL)
		{
			_bSkippedPixelData = true;
		}
	}

	return cond;
}

//
// Populates the internal value cache from the input stream
//
HRESULT CDicomPropertyStore::_LoadCacheFromStream(IStream* pStream)
{
    HRESULT hr = S_OK;

    if (!_pCache)
    {
        // create the internal value cache
        hr = PSCreateMemoryPropertyStore(IID_PPV_ARGS(&_pCache));
        if (SUCCEEDED(hr))
        {
#if 1
            ULARGE_INTEGER uliSize;
            hr = IStream_Size(pStream, &uliSize);
            if (SUCCEEDED(hr))
            {
                ULONG cbRead = 0;
                BYTE* pbBuf = (BYTE*)CoTaskMemAlloc(uliSize.u.LowPart);
                if (pbBuf)
                {
                    hr = pStream->Read(pbBuf, uliSize.u.LowPart, &cbRead);
                    if (SUCCEEDED(hr))
                    {
                        CONDITION cond = _ImportStream(pbBuf, cbRead, DCM_ORDERLITTLEENDIAN | DCM_ACCEPTVRMISMATCH, &_pDcmObj);
                        if (cond == DCM_NORMAL)
                        {
                            // TODO: check error

                            // populate native properties directly from the dicom object
                            // DCM_GetElementValue
                            // DCM_GetString
                            //
                            for (UINT i = 0; i < ARRAYSIZE(g_rgPROPERTYMAP); ++i)
                            {
                                _LoadProperty(g_rgPROPERTYMAP[i]);
                            }
                        }
                    }

                    CoTaskMemFree(pbBuf);
                }
            }
#else
/*
			// initialize the dicom object
			// HACK! Just use 8k header buffer for now
			ULONG cbHeaderSize = 12*1024;
			ULONG cbRead = 0;
			BYTE* pbHeader = (BYTE*)CoTaskMemAlloc(cbHeaderSize);
			if (pbHeader)
			{
				hr = pStream->Read(pbHeader, cbHeaderSize, &cbRead);
				if (SUCCEEDED(hr))
				{
					CONDITION cond = DCM_ImportStream(pbHeader, cbRead, DCM_EXPLICITLITTLEENDIAN | DCM_PART10FILE | DCM_ACCEPTVRMISMATCH | DCM_SKIPPIXELDATA, &_pDcmObj);
					if (cond == DCM_NORMAL)
					{
						// populate native properties directly from the dicom object
						// DCM_GetElementValue
						// DCM_GetString
						//
						for (UINT i = 0; i < ARRAYSIZE(g_rgPROPERTYMAP); ++i)
						{
							_LoadProperty(g_rgPROPERTYMAP[i]);
						}
					}
				}

				CoTaskMemFree(pbHeader);
			}
*/
#endif

            // load extended properties and search content
//            _LoadExtendedProperties();
//            _LoadSearchContent();
        }
    }

    return hr;
}


//
// Loads the data for the property specified in the given map into the internal value cache
//
HRESULT CDicomPropertyStore::_LoadProperty(const PROPERTYMAP &map)
{
	HRESULT hr = E_FAIL;
	PROPVARIANT propvarValues = {0};

	char* pszPropValue = DCM_GetString(&_pDcmObj, map.tag);
	if (pszPropValue)
	{
		// assert FAILED(hr)
	    if (map.bDate)
		{
			int y, m, d;
			int i = sscanf(pszPropValue, "%4d%2d%2d", &y, &m, &d);
			if (i == 3)
			{
				SYSTEMTIME lt = {0}, st = {0};
				FILETIME ft = {0};
				lt.wYear = y;
				lt.wMonth = m;
				lt.wDay = d;
				if (TzSpecificLocalTimeToSystemTime(NULL, &lt, &st) &&
					SystemTimeToFileTime(&st, &ft))
				{
					hr = InitPropVariantFromFileTime(&ft, &propvarValues);
				}
			}
		}
        else if (map.bLookupUID)
        {
            UID_DESCRIPTION descUID;
            CONDITION cond = UID_Lookup(pszPropValue, &descUID);
            if (cond == UID_NORMAL)
            {
                hr = InitPropVariantFromString(_bstr_t(descUID.description), &propvarValues);
            }
        }

		// fall back
        if FAILED(hr)
        {
            hr = InitPropVariantFromString(_bstr_t(pszPropValue), &propvarValues);
        }
		DCM_Free(pszPropValue);
    }

	if (hr == S_OK)
	{
		// coerce the value(s) to the appropriate type for the property key
		hr = PSCoerceToCanonicalValue(map.key, &propvarValues);
		if (SUCCEEDED(hr))
		{
			// cache the value(s) loaded
			hr = _pCache->SetValueAndState(map.key, &propvarValues, PSC_NORMAL);
		}

		PropVariantClear(&propvarValues);
	}

    return hr;
}


// INCOMPLETE
int CDicomPropertyStore::_CalcHeaderSize(IStream* pStream)
{
	// save current seek pointer, restore at the end

	// reset seek pointer to begining
	LARGE_INTEGER dlibMove = {DCM_PREAMBLELENGTH + 4, 0};
	HRESULT hr = pStream->Seek(dlibMove, STREAM_SEEK_SET, NULL);

	// parse through preamble and group tags
	int cbHeader = DCM_PREAMBLELENGTH + 4;
	while (1)
	{
		DWORD dwTag = 0;
		ULONG cbRead = 0;
		hr = pStream->Read(&dwTag, sizeof(DWORD), &cbRead);
		if (SUCCEEDED(hr))
		{

			hr = pStream->Read(&dwTag, sizeof(DWORD), &cbRead);

		}
	}

	dlibMove.LowPart = 0;
	hr = pStream->Seek(dlibMove, STREAM_SEEK_SET, NULL);

	return 0;
}


//
// Populates the System.Search.Contents property in the internal value cache
//
HRESULT CDicomPropertyStore::_LoadSearchContent()
{
    // generate a space-delimited list for of patient, study, image information for full text search
    HRESULT hr = E_OUTOFMEMORY;

	return hr;
}


//
// Saves the data for the property specified in the given map into the DICOM object
//
HRESULT CDicomPropertyStore::_SaveProperty(const PROPERTYMAP &map)
{
	HRESULT hr = E_FAIL;

    // TODO: do we want to support reverse UID look up?
    if (map.bLookupUID)
    {
        return hr;
    }

    // check the cache state; only save dirty properties
    PSC_STATE psc;
    hr = _pCache->GetState(map.key, &psc);
    if (SUCCEEDED(hr) && psc == PSC_DIRTY)
    {
        // get the cached value
        PROPVARIANT propvar = { 0 };
        hr = _pCache->GetValue(map.key, &propvar);
        if (SUCCEEDED(hr))
        {
            // save propvar
            WCHAR wszStr[100];
            char szStr[100];
            bool bValidStr = false;
            hr = PropVariantToString(propvar, wszStr, ARRAYSIZE(wszStr));
            if (SUCCEEDED(hr))
            {
                if (WideCharToMultiByte(CP_ACP, 0, wszStr, wcslen(wszStr)+1, szStr, ARRAYSIZE(szStr), 0, 0))
                {
                    bValidStr = true;
                }
            }
            
            DCM_ELEMENT el = {0};
            CONDITION cond = DCM_GetElement(&_pDcmObj, map.tag, &el);
            if (cond == DCM_NORMAL)
            {
                // check element value type and set new value
                // 
                // TODO
                if (DCM_IsString(el.representation))
                {
                    if (bValidStr)
                    {
                        el.length = strlen(szStr);
                        el.d.string = szStr;

                        int iUpdateCount = 0;
                        cond = DCM_ModifyElements(&_pDcmObj, &el, 1, NULL, 0, &iUpdateCount);
                    }
                }

                // TODO: check error
            }
            else if (cond == DCM_ELEMENTNOTFOUND)
            {
                // if the element does not exist, we need to add one
                el.tag = map.tag;
                cond = DCM_LookupElement(&el);
                if (cond == DCM_NORMAL)
                {
                    if (DCM_IsString(el.representation))
                    {
                        if (bValidStr)
                        {
                            el.length = strlen(szStr);
                            el.d.string = szStr;

                            cond = DCM_AddElement(&_pDcmObj, &el);
                        }
                    }
                }

                // TODO: check error
            }

            PropVariantClear(&propvar);
        }
    }

    return hr;
}


//
// Saves the values in the internal cache back to the DICOM object
//
HRESULT CDicomPropertyStore::_SaveCacheToDcmObj()
{
#if 1
    HRESULT hr = S_OK;

    for (UINT i = 0; i < ARRAYSIZE(g_rgPROPERTYMAP); ++i)
    {
        // TODO: check error
        _SaveProperty(g_rgPROPERTYMAP[i]);
    }

    return hr;
#else
/*
    // iterate over each property in the internal value cache
    DWORD cProps;
    HRESULT hr = _pCache->GetCount(&cProps);
    if (SUCCEEDED(hr))
    {
        for (UINT i = 0; SUCCEEDED(hr) && i < cProps; ++i)
        {
            PROPERTYKEY key;
            hr = _pCache->GetAt(i, &key);
            if (SUCCEEDED(hr))
            {
                // check the cache state; only save dirty properties
                PSC_STATE psc;
                hr = _pCache->GetState(key, &psc);
                if (SUCCEEDED(hr) && psc == PSC_DIRTY)
                {
                    // get the cached value
                    PROPVARIANT propvar = { 0 };
                    hr = _pCache->GetValue(key, &propvar);
                    if (SUCCEEDED(hr))
                    {
						// TBD save propvar
                        DCM_SetString

                        PropVariantClear(&propvar);
                    }
                }
            }
        }
    }

    return hr;
*/    
#endif
}


CONDITION DcmStreamCB(void *buf, U32 bytesExported, int lastFlag, void *ctx)
{
    IStream *pStream = (IStream*)ctx;
    ULONG cbWritten = 0;

    HRESULT hr = pStream->Write(buf, bytesExported, &cbWritten);

    // TODO: check error

    return DCM_NORMAL;
}


//
// Saves the DICOM object to stream
//
HRESULT CDicomPropertyStore::_SaveDcmObjToStream(IStream *pStream)
{
    HRESULT hr = E_UNEXPECTED;
    BYTE buf[2048];
	unsigned long options = DCM_ORDERLITTLEENDIAN;

    if (_bPart10)
    {
        options |= DCM_PART10FILE;
    }

    CONDITION cond = DCM_ExportStream(&_pDcmObj, options, buf, sizeof(buf), DcmStreamCB, pStream);
    if (cond == DCM_NORMAL)
    {
        hr = S_OK;
    }

    // TODO: check error
    
    return hr;
}


//
// Serializes a PROPVARIANT value to string form
//
HRESULT SerializePropVariantAsString(REFPROPVARIANT propvar, PWSTR *pszOut)
{
    SERIALIZEDPROPERTYVALUE *pBlob;
    ULONG cbBlob;

    // serialize PROPVARIANT to binary form
    HRESULT hr = StgSerializePropVariant(&propvar, &pBlob, &cbBlob);
    if (SUCCEEDED(hr))
    {
        // determine the required buffer size
        hr = E_FAIL;
        DWORD cchString;
        if (CryptBinaryToStringW((BYTE *)pBlob, cbBlob, CRYPT_STRING_BASE64, NULL, &cchString))
        {
            // allocate a sufficient buffer
            hr = E_OUTOFMEMORY;
            *pszOut = (PWSTR)CoTaskMemAlloc(sizeof(WCHAR) * cchString);
            if (*pszOut)
            {
                // convert the serialized binary blob to a string representation
                hr = E_FAIL;
                if (CryptBinaryToStringW((BYTE *)pBlob, cbBlob, CRYPT_STRING_BASE64, *pszOut, &cchString))
                {
                    hr = S_OK;
                }
                else
                {
                    CoTaskMemFree(*pszOut);
                }
            }
        }

        CoTaskMemFree(pBlob);
    }

    return S_OK;
}


//
// Deserializes a string value back into PROPVARIANT form
//
HRESULT DeserializePropVariantFromString(PCWSTR pszIn, PROPVARIANT *ppropvar)
{
    HRESULT hr = E_FAIL;
    DWORD dwFormatUsed, dwSkip, cbBlob;

    // compute and validate the required buffer size
    if (CryptStringToBinaryW(pszIn, 0, CRYPT_STRING_BASE64, NULL, &cbBlob, &dwSkip, &dwFormatUsed) &&
        dwSkip == 0 &&
        dwFormatUsed == CRYPT_STRING_BASE64)
    {
        // allocate a buffer to hold the serialized binary blob
        hr = E_OUTOFMEMORY;
        BYTE *pbSerialized = (BYTE *)CoTaskMemAlloc(cbBlob);
        if (pbSerialized)
        {
            // convert the string to a serialized binary blob
            hr = E_FAIL;
            if (CryptStringToBinaryW(pszIn, 0, CRYPT_STRING_BASE64, pbSerialized, &cbBlob, &dwSkip, &dwFormatUsed))
            {
                // deserialized the blob back into a PROPVARIANT value
                hr = StgDeserializePropVariant((SERIALIZEDPROPERTYVALUE *)pbSerialized, cbBlob, ppropvar);
            }

            CoTaskMemFree(pbSerialized);
        }
    }

    return hr;
}

