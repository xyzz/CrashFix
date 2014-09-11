//! \file HardwareInfo.cpp
//! \brief Hardware information collection.
//! \author Oleg Krivtsov
//! \date 2011

#include "stdafx.h"
#include "HardwareInfo.h"
#include "strconv.h"
#include <tinyxml.h>
#include "ProtectedFile.h"
#include "Misc.h"
#include "md5.h"

using namespace std;

CHardwareInfo::CHardwareInfo()
{
	m_bInfoRetrieved = FALSE;
}

CHardwareInfo::~CHardwareInfo()
{
}

#ifdef _WIN32
BOOL CHardwareInfo::ExecWMIQuery (LPCWSTR szQuery, IEnumWbemClassObject** ppEnumerator)
{
	HRESULT hres = m_pSvc->ExecQuery(
        bstr_t("WQL"),
        bstr_t(szQuery),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        ppEnumerator);

    if (FAILED(hres))
    {
        return FALSE;               // failed.
    }

	return TRUE;
}
#endif

bool CHardwareInfo::RetreiveFromComputer()
{
	m_bInfoRetrieved = FALSE;

	// First clean internal hadware info structure
	m_Info = HardwareInfo();

#ifdef _WIN32

	if(!InitWMI())
		return false;

	if(!GetMachineInfoWindows())
        return false;

    if(!GetNetworkAdapterInfoWindows())
        return false;

    if(!GetHDDInfoWindows())
        return false;

#else // Linux

    if(!GetMachineInfoLinux())
        return false;

    if(!GetNetworkAdapterInfoLinux())
        return false;

    //if(!GetHDDInfoLinux())
    //    return false;

#endif

	// Get current system time
	time(&m_Info.m_SystemTime);

	// Set generator version
	m_Info.m_sGeneratorVersion = strconv::a2w(LIBDUMPER_VER);

	m_bInfoRetrieved = TRUE;
    return true;
}

#ifdef _WIN32

bool CHardwareInfo::InitWMI()
{
	// Initialize COM
    CoInitialize(NULL);

	HRESULT hres = E_FAIL;

    hres =  CoInitializeSecurity(
        NULL,
        -1,                          // COM authentication
        NULL,                        // Authentication services
        NULL,                        // Reserved
        RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication
        RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation
        NULL,                        // Authentication info
        EOAC_NONE,                   // Additional capabilities
        NULL                         // Reserved
        );

    if (FAILED(hres))
    {
        if(hres!=RPC_E_TOO_LATE)
            return false;
    }

    hres = CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID *) &m_pLoc);

    if (FAILED(hres))
    {
        return false;
    }

    hres = m_pLoc->ConnectServer(
         _bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
         NULL,                    // User name. NULL = current user
         NULL,                    // User password. NULL = current
         0,                       // Locale. NULL indicates current
         NULL,                    // Security flags.
         0,                       // Authority (e.g. Kerberos)
         0,                       // Context object
         &m_pSvc                    // pointer to IWbemServices proxy
         );

    if (FAILED(hres))
    {
        return false;
    }

	hres = CoSetProxyBlanket(
       m_pSvc,                        // Indicates the proxy to set
       RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
       RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
       NULL,                        // Server principal name
       RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx
       RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
       NULL,                        // client identity
       EOAC_NONE                    // proxy capabilities
    );

    if (FAILED(hres))
    {
        return false;
    }

	return true;
}

bool CHardwareInfo::GetMachineInfoWindows()
{
	m_Info.m_OpSysType = ST_WINDOWS;

	SYSTEM_INFO SystemInfo;
    GetSystemInfo(&SystemInfo);
	if(SystemInfo.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
		m_Info.m_sMachineType = L"AMD64";
	else if(SystemInfo.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_INTEL)
		m_Info.m_sMachineType = L"x86";
	else
		m_Info.m_sMachineType = L"Unknown";

	GetWindowsFriendlyName(m_Info.m_sOSVersion);

    OSVERSIONINFOEX osvi;
    memset(&osvi, 0, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((LPOSVERSIONINFO)&osvi);

	wchar_t szBuffer[64]=L"";
	wsprintf(szBuffer, L"%d.%d.%d %s", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber, osvi.szCSDVersion);
	m_Info.m_sOSRelease = szBuffer;

	/* Get CPU info */

	// Get extended ids.
    int CPUInfo[4] = {-1};
    __cpuid(CPUInfo, 0x80000000);
    unsigned int nExIds = CPUInfo[0];

    // Get the information associated with each extended ID.
    char CPUBrandString[0x40] = { 0 };
    for( unsigned int i=0x80000000; i<=nExIds; ++i)
    {
        __cpuid(CPUInfo, i);

        // Interpret CPU brand string and cache information.
        if  (i == 0x80000002)
        {
            memcpy( CPUBrandString,
            CPUInfo,
            sizeof(CPUInfo));
        }
        else if( i == 0x80000003 )
        {
            memcpy( CPUBrandString + 16,
            CPUInfo,
            sizeof(CPUInfo));
        }
        else if( i == 0x80000004 )
        {
            memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
        }
	}
	m_Info.m_sCPUName = strconv::a2w(CPUBrandString);
	std::replace( m_Info.m_sCPUName.begin(), m_Info.m_sCPUName.end(), ' ', '_');
    m_Info.m_sCPUName.erase(std::remove(m_Info.m_sCPUName.begin(), m_Info.m_sCPUName.end(), '\t'), m_Info.m_sCPUName.end());
    m_Info.m_sCPUName.erase(std::remove(m_Info.m_sCPUName.begin(), m_Info.m_sCPUName.end(), '\r'), m_Info.m_sCPUName.end());
    m_Info.m_sCPUName.erase(std::remove(m_Info.m_sCPUName.begin(), m_Info.m_sCPUName.end(), '\n'), m_Info.m_sCPUName.end());

	/* Get computer name */

	CComPtr<IEnumWbemClassObject> pEnumerator;
    CComPtr<IWbemClassObject> pclsObj;
    ULONG uReturn = 0;

	wchar_t* szQuery = L"SELECT Name, Manufacturer, Domain FROM Win32_ComputerSystem";

	if(!ExecWMIQuery(szQuery, &pEnumerator))
        return false;

	while (pEnumerator)
    {
		if(pclsObj)
			pclsObj.Release();

        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
            &pclsObj, &uReturn);

		if(0 == uReturn)
			break;

        VARIANT vtProp;
        VariantInit(&vtProp);

        hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
        if(SUCCEEDED(hr) && vtProp.vt!=VT_NULL)
		{
            m_Info.m_sComputerName = std::wstring(vtProp.bstrVal);
		}

        hr = pclsObj->Get(L"Manufacturer", 0, &vtProp, 0, 0);
        if(SUCCEEDED(hr) && vtProp.vt!=VT_NULL)
		{
            m_Info.m_sComputerManufacturer = std::wstring(vtProp.bstrVal);					
			wtrim(m_Info.m_sComputerManufacturer);
		}

        hr = pclsObj->Get(L"Domain", 0, &vtProp, 0, 0);
        if(SUCCEEDED(hr) && vtProp.vt!=VT_NULL)
		{
            m_Info.m_sDomain = std::wstring(vtProp.bstrVal);
		}

        VariantClear(&vtProp);

    }

	return true;
}

bool CHardwareInfo::GetWindowsFriendlyName(std::wstring& sOSName)
{
    sOSName.clear();
    CRegKey regKey;
    LONG lResult = regKey.Open(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), KEY_READ);
    if(lResult==ERROR_SUCCESS)
    {
        TCHAR buf[1024];
        ULONG buf_size = 0;

        TCHAR* PRODUCT_NAME = _T("ProductName");
        TCHAR* CURRENT_BUILD_NUMBER = _T("CurrentBuildNumber");
        TCHAR* CSD_VERSION = _T("CSDVersion");

#pragma warning(disable:4996)

        buf_size = 1023;
        if(ERROR_SUCCESS == regKey.QueryValue(buf, PRODUCT_NAME, &buf_size))
        {
            sOSName += buf;
        }

        buf_size = 1023;
        if(ERROR_SUCCESS == regKey.QueryValue(buf, CURRENT_BUILD_NUMBER, &buf_size))
        {
            sOSName += _T(" Build ");
            sOSName += buf;
        }

        buf_size = 1023;
        if(ERROR_SUCCESS == regKey.QueryValue(buf, CSD_VERSION, &buf_size))
        {
            sOSName += _T(" ");
            sOSName += buf;
        }

#pragma warning(default:4996)

        regKey.Close();
        return true;
    }

    return false;
}

bool CHardwareInfo::GetHDDInfoWindows()
{
	CComPtr<IEnumWbemClassObject> pEnumerator;
    CComPtr<IWbemClassObject> pclsObj;
    ULONG uReturn = 0;

	wchar_t* szQuery = L"SELECT Model, SerialNumber, PNPDeviceID FROM Win32_DiskDrive";

	if(!ExecWMIQuery(szQuery, &pEnumerator))
        return false;

	while (pEnumerator)
    {
		if(pclsObj)
			pclsObj.Release();

        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
            &pclsObj, &uReturn);

		if(0 == uReturn)
			break;

        VARIANT vtProp;
        VariantInit(&vtProp);

		HardDiskInfo hdi;

        hr = pclsObj->Get(L"Model", 0, &vtProp, 0, 0);
        if(SUCCEEDED(hr) && vtProp.vt!=VT_NULL)
		{
			hdi.m_sModel = std::wstring(vtProp.bstrVal);
		}

        hr = pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0);
        if(SUCCEEDED(hr) && vtProp.vt!=VT_NULL)
		{
			hdi.m_sSerialNumber = std::wstring(vtProp.bstrVal);
		}

        hr = pclsObj->Get(L"PNPDeviceID", 0, &vtProp, 0, 0);
        if(SUCCEEDED(hr) && vtProp.vt!=VT_NULL)
		{
            hdi.m_sPNPDeviceID = std::wstring(vtProp.bstrVal);
		}

		if(hdi.m_sPNPDeviceID.substr(0, 8)==L"USBSTOR\\")
			continue; // Skip USB disks

        VariantClear(&vtProp);

		m_Info.m_aHDDs.push_back(hdi);
    }

	return true;
}

bool CHardwareInfo::GetNetworkAdapterInfoWindows()
{
	CComPtr<IEnumWbemClassObject> pEnumerator;
    CComPtr<IWbemClassObject> pclsObj;
    ULONG uReturn = 0;

	wchar_t* szQuery = L"SELECT Caption, PNPDeviceID, Manufacturer, MACAddress FROM Win32_NetworkAdapter";

	if(!ExecWMIQuery(szQuery, &pEnumerator))
        return false;

	while (pEnumerator)
    {
		if(pclsObj)
			pclsObj.Release();

        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
            &pclsObj, &uReturn);

		if(0 == uReturn)
			break;

        VARIANT vtProp;
        VariantInit(&vtProp);

        NetworkAdapterInfo nai;

        hr = pclsObj->Get(L"Caption", 0, &vtProp, 0, 0);
        if(SUCCEEDED(hr) && vtProp.vt!=VT_NULL)
		{
            nai.m_sAdapterName = std::wstring(vtProp.bstrVal);
		}

        hr = pclsObj->Get(L"Manufacturer", 0, &vtProp, 0, 0);
        if(SUCCEEDED(hr) && vtProp.vt!=VT_NULL)
		{
            nai.m_sManufacturer = std::wstring(vtProp.bstrVal);
		}

        hr = pclsObj->Get(L"PNPDeviceID", 0, &vtProp, 0, 0);
        if(SUCCEEDED(hr) && vtProp.vt!=VT_NULL)
		{
            nai.m_sPNPDeviceID = std::wstring(vtProp.bstrVal);
		}

        hr = pclsObj->Get(L"MACAddress", 0, &vtProp, 0, 0);
        if(SUCCEEDED(hr) && vtProp.vt!=VT_NULL)
		{
            nai.m_sMacAddress = std::wstring(vtProp.bstrVal);
		}

		/*hr = pclsObj->Get(L"IPAddress", 0, &vtProp, 0, 0);
        if(SUCCEEDED(hr) && vtProp.vt!=VT_NULL)
		{
            nai.m_sIpAddress = std::wstring(vtProp.bstrVal);
		}*/

		// Ignore Microsoft-manufactured (virtual) adapters
        if(nai.m_sManufacturer.compare(L"Microsoft")==0)
            continue;

		// Ignore virtual network adapters
        if(nai.m_sPNPDeviceID.substr(0, 4)!=L"PCI\\")
            continue;

		// Ignore network adapters without MAC address
		// or with incorrect MAC address length
		if(nai.m_sMacAddress.length()!=17)
            continue;

		// Add this network adapter to the list
        m_Info.m_aNetworkAdapters.push_back(nai);

        VariantClear(&vtProp);

    }

	return true;
}

#else // Linux

bool CHardwareInfo::GetMachineInfoLinux()
{
    m_Info.m_OpSysType = ST_LINUX;

    char szHostName[1024] = "";
    gethostname(szHostName, 1024);
    m_Info.m_sComputerName = strconv::a2w(szHostName);

    struct utsname sysver;
    if(0==uname(&sysver))
    {
        m_Info.m_sOSRelease = strconv::a2w(sysver.release);
        m_Info.m_sOSVersion = strconv::a2w(sysver.version);
        m_Info.m_sMachineType = strconv::a2w(sysver.machine);
    }

    FILE * fp = NULL;
    char res[128] = "";
    fp = popen("/bin/cat /proc/cpuinfo | grep model | grep name","r");
    if(fp!=NULL)
    {
        int nBytes = fread(res, 1, 127, fp);
        fclose(fp);
        if(nBytes==0)
            return false; // Nothing read
        m_Info.m_sCPUName = strconv::a2w(res).substr(13);
        std::replace( m_Info.m_sCPUName.begin(), m_Info.m_sCPUName.end(), ' ', '_');
        m_Info.m_sCPUName.erase(std::remove(m_Info.m_sCPUName.begin(), m_Info.m_sCPUName.end(), '\t'), m_Info.m_sCPUName.end());
        m_Info.m_sCPUName.erase(std::remove(m_Info.m_sCPUName.begin(), m_Info.m_sCPUName.end(), '\r'), m_Info.m_sCPUName.end());
        m_Info.m_sCPUName.erase(std::remove(m_Info.m_sCPUName.begin(), m_Info.m_sCPUName.end(), '\n'), m_Info.m_sCPUName.end());
    }

    return true;
}

bool CHardwareInfo::GetHDDInfoLinux()
{
    static struct hd_driveid hd;
    int fd = -1;

    if ((fd = open("/dev/hda", O_RDONLY|O_NONBLOCK)) < 0)
    {
        m_sErrorMsg = "Cannot open device /dev/sda. Are you root?";
        return false;
    }

    if (!ioctl(fd, HDIO_GET_IDENTITY, &hd))
    {
        HardDiskInfo hdd;
        hdd.m_sModel = strconv::a2w((const char*)hd.model);
        hdd.m_sSerialNumber = strconv::a2w((const char*)hd.serial_no);

        m_Info.m_aHDDs.push_back(hdd);
    }
    else if (errno == -ENOMSG)
    {
        m_sErrorMsg = "No hard disk identification information available";
        return false;
    }
    else
    {
        m_sErrorMsg = "HDIO_GET_IDENTITY";
        return false;
    }

    return true;
}

bool CHardwareInfo::GetNetworkAdapterInfoLinux()
{
    int nSD = -1; // Socket descriptor
    struct ifreq sIfReq; // Interface request
    struct if_nameindex *pIfList; // Ptr to interface name index
    struct if_nameindex *pListSave; // Ptr to interface name index

    //
    // Initialize this function
    //

    pIfList = (struct if_nameindex *)NULL;
    pListSave = (struct if_nameindex *)NULL;

#ifndef SIOCGIFADDR
        m_sErrorMsg = "The kernel does not support the required ioctls";
        return false;
#endif

    //
    // Create a socket that we can use for all of our ioctls
    //
    nSD = socket( PF_INET, SOCK_STREAM, 0 );
    if ( nSD < 0 )
    {
        m_sErrorMsg = "Socket creation failed, this is a fatal error";
        return false;
    }

    //
    // Obtain a list of dynamically allocated structures
    //
    pIfList = pListSave = if_nameindex();

    //
    // Walk thru the array returned and query for each interface's
    // address
    //
    for ( pIfList; *(char *)pIfList != 0; pIfList++ )
    {
        //
        // Determine if we are processing the interface that we
        // are interested in
        //
        if ( strcmp(pIfList->if_name, "eth0") )
            continue; // Nope, check the next one in the list

        strncpy( sIfReq.ifr_name, pIfList->if_name, IF_NAMESIZE );

        //
        // Get the MAC address for this interface
        //
        if ( ioctl(nSD, SIOCGIFHWADDR, &sIfReq) != 0 )
        {
            m_sErrorMsg = "Failed to get the MAC address for the interface";
            return false;
        }

        const unsigned char* pMacAddr = (const unsigned char*)&sIfReq.ifr_ifru.ifru_hwaddr.sa_data[0];
        char szMacAddr[32] = "";
        sprintf(szMacAddr, "%02X:%02X:%02X:%02X:%02X:%02X",
                pMacAddr[0], pMacAddr[1], pMacAddr[2],
                pMacAddr[3], pMacAddr[4], pMacAddr[5] );
        NetworkAdapterInfo nai;
        nai.m_sAdapterName = L"eth0";
        nai.m_sMacAddress = strconv::a2w(szMacAddr);
        m_Info.m_aNetworkAdapters.push_back(nai);

        break;
    }

    //
    // Clean up things and return
    //
    if_freenameindex( pListSave );
    close( nSD );

    return(true);
}
#endif

bool CHardwareInfo::IsValid()
{
	return m_bInfoRetrieved;
}

const HardwareInfo* CHardwareInfo::GetCurInfo()
{
	if(m_bInfoRetrieved == FALSE)
		return NULL;

	return &m_Info;
}

void CHardwareInfo::FreeInfo()
{
	m_bInfoRetrieved = FALSE;
	m_Info = HardwareInfo();
}

bool CHardwareInfo::CalcFingerPrint(std::wstring& sFingerPrint)
{
	m_sErrorMsg = "Unspecified error.";
	bool bStatus = false;
	sFingerPrint.clear();
	std::wstring str = L"HwHW$$_";
	unsigned char md5_hash[16]="";
	MD5 md5;
    MD5_CTX md5_ctx;
	const HardwareInfo* pMyInfo = NULL;
	size_t i;

	// Check if hardware info is valid
	if(!IsValid())
	{
		m_sErrorMsg = "Hardware info was not retrieved or loaded yet.";
		goto cleanup;
	}

	pMyInfo = GetCurInfo();
	if(pMyInfo==NULL)
		goto cleanup; // Info shouldn't be NULL.
	
	if(pMyInfo->m_OpSysType==ST_UNDEFINED || 
		pMyInfo->m_sMachineType.empty())
		goto cleanup; // OS type undefined

	// Append OS type
	if(pMyInfo->m_OpSysType==ST_LINUX)
		str += L"Linux";
	else
		str += L"Windows";

	// Append machine type
	str += pMyInfo->m_sMachineType;

	// Check if there are network adapters
	if(pMyInfo->m_aNetworkAdapters.size()!=0)
	{
		// Walk through network adapters
		size_t i;
		for(i=0; i<pMyInfo->m_aNetworkAdapters.size(); i++)
		{
			const NetworkAdapterInfo& nai = pMyInfo->m_aNetworkAdapters[i];
			// Append MAC address
			str += nai.m_sMacAddress;			
		}
	}
	else // if there are no network adapters
	{
		// Require CPU name 
		if(pMyInfo->m_sCPUName.empty())
		{
			m_sErrorMsg = "CPU name is empty.";
			goto cleanup;
		}

		// Append CPU name
		str += pMyInfo->m_sCPUName;

		// OS-specific checks
		if(pMyInfo->m_OpSysType==ST_WINDOWS)
		{
			// Require at least one HDD
			if(pMyInfo->m_aHDDs.size()==0)
			{
				m_sErrorMsg = "There are no hard disk drives.";
				goto cleanup;
			}

			// Walk through hard disks			
			for(i=0; i<pMyInfo->m_aHDDs.size(); i++)
			{
				const HardDiskInfo& hddi = pMyInfo->m_aHDDs[i];
				// Use HDD serial number
				str += hddi.m_sSerialNumber;
			}		
		}
		else 
		{
			// Use computer name in Linux
			if(pMyInfo->m_sComputerName.empty())
			{
				m_sErrorMsg = "Could not get computer name.";
				goto cleanup;
			}

			str += pMyInfo->m_sComputerName;
		}			
	}
			
	// Calc MD5 hash
	md5.MD5Init(&md5_ctx);
	md5.MD5Update(&md5_ctx, (BYTE*)str.c_str(), str.size()*2);
	md5.MD5Final(md5_hash, &md5_ctx);

	// Convert MD5 buffer to string	
    for(i=0; i<16; i++)
    {
        wchar_t number[10];
#ifdef _WIN32
        swprintf(number, 10, L"%02x", md5_hash[i]);
#else
        swprintf(number, 10, L"%02x", md5_hash[i]);
#endif
        sFingerPrint += number;
    }

	// Success.
	m_sErrorMsg.clear();
	bStatus = true;

cleanup:

	return bStatus;
}

bool CHardwareInfo::Compare(CHardwareInfo* pAnotherHwInfo)
{
	m_sErrorMsg = "Unspecified error.";

	// Validate input
	if(pAnotherHwInfo==NULL)
	{
		m_sErrorMsg = "Invalid parameter.";
		return false;
	}

	// Check if hardware info is valid
	if(!IsValid() || !pAnotherHwInfo->IsValid())
	{
		m_sErrorMsg = "Hardware info was not retrieved or loaded yet.";
		return false;
	}

	const HardwareInfo* pMyInfo = GetCurInfo();
	if(pMyInfo==NULL)
		return false; // Info shouldn't be NULL.

	const HardwareInfo* pTheirInfo = pAnotherHwInfo->GetCurInfo();
	if(pTheirInfo==NULL)
		return false; // Their info shouldn't be NULL.

	// Compare os type
	if(pMyInfo->m_OpSysType!=pTheirInfo->m_OpSysType)
	{
		return false; // OS type mismatch
	}

	/*// Compare machine type
	if(pMyInfo->m_sMachineType!=pTheirInfo->m_sMachineType)
	{
		// Machine type mismatch
		return false;
	}*/		

	if(pMyInfo->m_aNetworkAdapters.size()!=0)
	{
		// Compare number of network adapters
		if(pMyInfo->m_aNetworkAdapters.size()!=pTheirInfo->m_aNetworkAdapters.size())
		{
			// Network adapter count mismatch
			return false;
		}

		// Walk through network adapters
		size_t i;
		for(i=0; i<pMyInfo->m_aNetworkAdapters.size(); i++)
		{
			const NetworkAdapterInfo& nai = pMyInfo->m_aNetworkAdapters[i];
			bool bMatch = false;
			size_t j;
			for(j=0; j<pTheirInfo->m_aNetworkAdapters.size(); j++)
			{
				const NetworkAdapterInfo& nai2 = pTheirInfo->m_aNetworkAdapters[j];
				if(nai.m_sManufacturer==nai2.m_sManufacturer &&
					nai.m_sMacAddress==nai2.m_sMacAddress)
				{
					bMatch = true;
					break;
				}
			}

			if(!bMatch)
			{
				// Network adapter MAC address mismatch
				return false;
			}
		}
	}
	else // there are no network adapters
	{
		// Compare CPU names
		if(pMyInfo->m_sCPUName!=pTheirInfo->m_sCPUName)
		{
			// CPU name mismatch
			return false;
		}

		// OS-specific checks: in Windows use HDD serials numbers, 
		// in Linux - computer name.
		if(pMyInfo->m_OpSysType==ST_WINDOWS)
		{
			// Compare HDD count
			if(pMyInfo->m_aHDDs.size()!=pTheirInfo->m_aHDDs.size())
			{
				// HDD count mismatch
				return false;
			}

			// Walk through hard disks			
			size_t i;
			for(i=0; i<pMyInfo->m_aHDDs.size(); i++)
			{
				const HardDiskInfo& hddi = pMyInfo->m_aHDDs[i];
				bool bMatch = false;
				size_t j;
				for(j=0; j<pTheirInfo->m_aHDDs.size(); j++)
				{
					const HardDiskInfo& hddi2 = pTheirInfo->m_aHDDs[j];
					if(hddi.m_sSerialNumber==hddi2.m_sSerialNumber)
					{
						bMatch = true;
						break;
					}
				}

				if(!bMatch)
				{
					// HDD serial number mismatch
					return false;
				}
			}	
		}
		else 
		{
			// Use computer name in Linux
			if(pMyInfo->m_sComputerName!=pTheirInfo->m_sComputerName)
			{
				// Computer name mismatch
				return false;
			}			
		}	
	}

	// Success.
	m_sErrorMsg.clear();
	return true;
}

bool CHardwareInfo::SaveFile(std::string sFileName, bool bEncrypt, bool bUseTestKey)
{
	// This method writes internal hardware info data to XML file and optionally encrypts the file.
	// The XML message is encrypted with Blowfish symmetric algorithm, whose session key and message's SHA-1 hash
	// are encrypted with RSA public key are written to the beginning of file.

	// Get XML message
	std::string sXml;
	bool bGetXml = GenerateXML(sXml);
	if(!bGetXml)
		return false;

	// Finally, dump the XML to file (either with encryption or not).
    if(!bEncrypt)
    {
		// Open file
		FILE* f = fopen(sFileName.c_str(), "wt");
		if(f==NULL)
			return false;

		// Dump XML to file without encryption
		fprintf(f, "%s", sXml.c_str());

		// Close the file
		fclose(f);
    }
    else
    {
		// Get XML as a string buffer
		const char* szXml = sXml.c_str();
		size_t uSize = sXml.length();

		// Create protected file
		CProtectedFile pf;
		pf.SetUseTestKeyPair(bUseTestKey);
		pf.SetData((LPBYTE)szXml, (UINT)uSize, false);

		// Save file to disk
		if(!pf.Save(sFileName, TRUE, "", ""))
		{
			m_sErrorMsg = pf.GetErrorMsg();
			return false;
		}
    }


	// Done
	return true;
}

bool CHardwareInfo::ReadFile(std::string sFileName, std::string sPKFile, std::string sPassPhrase, bool bUseTestKey)
{
	// This method reads hardware info from file

	m_sErrorMsg = "Unspecified error.";
	CProtectedFile pf;

	// Free internal info if previously loaded
	FreeInfo();

	pf.SetUseTestKeyPair(bUseTestKey);

	// Open protected file
	if(!pf.Load(sFileName, TRUE, sPKFile, sPassPhrase))
	{
		m_sErrorMsg = "Error reading protected file.";
		return false;
	}

	// Extract XML text
	LPBYTE pBuffer = NULL;
	UINT uBuffSize = 0;
	if(!pf.GetData(&pBuffer, &uBuffSize) || pBuffer==NULL || uBuffSize==0)
	{
		m_sErrorMsg = "Error extracting protected file content.";
		return false;
	}

	// Fill internal vars with info from XML
	std::string sXml = std::string((char*)pBuffer, uBuffSize);
	if(!ReadXML(sXml))
		return false;

	// Success.
	m_sErrorMsg.clear();
	return true;
}

bool CHardwareInfo::GenerateXML(std::string& sXml)
{
	TiXmlDocument doc;

	if(!GenerateXML(&doc))
		return false;

	// Save XML document to memory buffer
    TiXmlPrinter printer;
	printer.SetIndent( "\t" );
	doc.Accept( &printer );

	// Return resulting XML as a string
	sXml = printer.CStr();

	return true;
}

bool CHardwareInfo::GenerateXML(TiXmlDocument* pDoc)
{
	// This method writes internal hardware info data to XML document
	// and returns the document as a string.

    TiXmlDocument doc;

    // Add declaration
    TiXmlDeclaration* pDecl = new TiXmlDeclaration( "1.0", "", "" );
    doc.LinkEndChild(pDecl);

    // Add root element
    TiXmlHandle hRoot = new TiXmlElement("Root");
    doc.LinkEndChild(hRoot.ToNode());

	// Add generator version attribute
	hRoot.ToElement()->SetAttribute("version", strconv::w2utf8(m_Info.m_sGeneratorVersion).c_str());

	// Add Computer element
    TiXmlHandle hComputer = new TiXmlElement("Computer");
    hRoot.ToElement()->LinkEndChild(hComputer.ToNode());

    // Add SystemType
    {
        std::string sSystemType;
        if(m_Info.m_OpSysType==ST_WINDOWS)
            sSystemType = "Windows";
        else if(m_Info.m_OpSysType==ST_LINUX)
            sSystemType = "Linux";
        else
            sSystemType = "Undefined";
        TiXmlHandle hElem = new TiXmlElement("SystemType");
        TiXmlHandle hElemText = new TiXmlText(sSystemType.c_str());
        hElem.ToElement()->LinkEndChild(hElemText.ToNode());
        hComputer.ToElement()->LinkEndChild(hElem.ToNode());
    }

	// Add ComputerName
    {
        TiXmlHandle hElem = new TiXmlElement("ComputerName");
        TiXmlHandle hElemText = new TiXmlText( strconv::w2utf8(m_Info.m_sComputerName).c_str() );
        hElem.ToElement()->LinkEndChild(hElemText.ToNode());
        hComputer.ToElement()->LinkEndChild(hElem.ToNode());
    }

	// Add Domain
    {
        TiXmlHandle hElem = new TiXmlElement("Domain");
        TiXmlHandle hElemText = new TiXmlText( strconv::w2utf8(m_Info.m_sDomain).c_str() );
        hElem.ToElement()->LinkEndChild(hElemText.ToNode());
        hComputer.ToElement()->LinkEndChild(hElem.ToNode());
    }

	// Add ComputerManufacturer
    {
        TiXmlHandle hElem = new TiXmlElement("ComputerManufacturer");
        TiXmlHandle hElemText = new TiXmlText( strconv::w2utf8(m_Info.m_sComputerManufacturer).c_str() );
        hElem.ToElement()->LinkEndChild(hElemText.ToNode());
        hComputer.ToElement()->LinkEndChild(hElem.ToNode());
    }

    // Add OSRelease
    {
        TiXmlHandle hElem = new TiXmlElement("OSRelease");
        TiXmlHandle hElemText = new TiXmlText( strconv::w2utf8(m_Info.m_sOSRelease).c_str() );
        hElem.ToElement()->LinkEndChild(hElemText.ToNode());
        hComputer.ToElement()->LinkEndChild(hElem.ToNode());
    }

    // Add OSVersion
    {
        TiXmlHandle hElem = new TiXmlElement("OSVersion");
        TiXmlHandle hElemText = new TiXmlText( strconv::w2utf8(m_Info.m_sOSVersion).c_str() );
        hElem.ToElement()->LinkEndChild(hElemText.ToNode());
        hComputer.ToElement()->LinkEndChild(hElem.ToNode());
    }

    // Add MachineType
    {
        TiXmlHandle hElem = new TiXmlElement("MachineType");
        TiXmlHandle hElemText = new TiXmlText( strconv::w2utf8(m_Info.m_sMachineType).c_str() );
        hElem.ToElement()->LinkEndChild(hElemText.ToNode());
        hComputer.ToElement()->LinkEndChild(hElem.ToNode());
    }

    // Add CPUName
    {
        TiXmlHandle hElem = new TiXmlElement("CPUName");
        TiXmlHandle hElemText = new TiXmlText( strconv::w2utf8(m_Info.m_sCPUName).c_str() );
        hElem.ToElement()->LinkEndChild(hElemText.ToNode());
        hComputer.ToElement()->LinkEndChild(hElem.ToNode());
    }

	// Add NetworkAdapters element
    TiXmlHandle hNetworkAdapters = new TiXmlElement("NetworkAdapters");
    hRoot.ToElement()->LinkEndChild(hNetworkAdapters.ToNode());

	// Walk through network adapters
    size_t i;
    for(i=0; i<m_Info.m_aNetworkAdapters.size(); i++)
    {
        NetworkAdapterInfo& nai = m_Info.m_aNetworkAdapters[i];

		// Add NetworkAdapters->NetworkAdapter
        TiXmlHandle hNetworkAdapter = new TiXmlElement("NetworkAdapter");
        hNetworkAdapters.ToElement()->LinkEndChild(hNetworkAdapter.ToNode());

		// Write Name
        {
            TiXmlHandle hElem = new TiXmlElement("Name");
            TiXmlHandle hElemText = new TiXmlText( strconv::w2utf8(nai.m_sAdapterName).c_str() );
            hElem.ToElement()->LinkEndChild(hElemText.ToNode());
            hNetworkAdapter.ToElement()->LinkEndChild(hElem.ToNode());
        }

		// Write PNPDeviceID
        {
            TiXmlHandle hElem = new TiXmlElement("PNPDeviceID");
            TiXmlHandle hElemText = new TiXmlText( strconv::w2utf8(nai.m_sPNPDeviceID).c_str() );
            hElem.ToElement()->LinkEndChild(hElemText.ToNode());
            hNetworkAdapter.ToElement()->LinkEndChild(hElem.ToNode());
        }

		// Write Manufacturer
        {
            TiXmlHandle hElem = new TiXmlElement("Manufacturer");
            TiXmlHandle hElemText = new TiXmlText( strconv::w2utf8(nai.m_sManufacturer).c_str() );
            hElem.ToElement()->LinkEndChild(hElemText.ToNode());
            hNetworkAdapter.ToElement()->LinkEndChild(hElem.ToNode());
        }

		// Write MacAddress
        {
            TiXmlHandle hElem = new TiXmlElement("MacAddress");
            TiXmlHandle hElemText = new TiXmlText( strconv::w2utf8(nai.m_sMacAddress).c_str() );
            hElem.ToElement()->LinkEndChild(hElemText.ToNode());
            hNetworkAdapter.ToElement()->LinkEndChild(hElem.ToNode());
        }
    }

    // Add HardDisks element
    TiXmlHandle hHardDisks = new TiXmlElement("HardDisks");
    hRoot.ToElement()->LinkEndChild(hHardDisks.ToNode());

	// Walk through HDDs
    for(i=0; i<m_Info.m_aHDDs.size(); i++)
    {
        HardDiskInfo& hdi = m_Info.m_aHDDs[i];

		// Add HardDisk
        TiXmlHandle hHardDisk = new TiXmlElement("HardDisk");
        hHardDisks.ToElement()->LinkEndChild(hHardDisk.ToNode());

		// Write Model
        {
            TiXmlHandle hElem = new TiXmlElement("Model");
            TiXmlHandle hElemText = new TiXmlText( strconv::w2utf8(hdi.m_sModel).c_str() );
            hElem.ToElement()->LinkEndChild(hElemText.ToNode());
            hHardDisk.ToElement()->LinkEndChild(hElem.ToNode());
        }

		// Write SearialNumber
        {
            TiXmlHandle hElem = new TiXmlElement("SerialNumber");
            TiXmlHandle hElemText = new TiXmlText( strconv::w2utf8(hdi.m_sSerialNumber).c_str() );
            hElem.ToElement()->LinkEndChild(hElemText.ToNode());
            hHardDisk.ToElement()->LinkEndChild(hElem.ToNode());
        }

		// Write PNPDeviceID
        {
            TiXmlHandle hElem = new TiXmlElement("PNPDeviceID");
            TiXmlHandle hElemText = new TiXmlText( strconv::w2utf8(hdi.m_sPNPDeviceID).c_str() );
            hElem.ToElement()->LinkEndChild(hElemText.ToNode());
            hHardDisk.ToElement()->LinkEndChild(hElem.ToNode());
        }
    }

	// Add FingerPrint
    {
		std::wstring sFingerPrint;
		if(CalcFingerPrint(sFingerPrint))
		{
			TiXmlHandle hElem = new TiXmlElement("FingerPrint");
			TiXmlHandle hElemText = new TiXmlText( strconv::w2utf8(sFingerPrint).c_str() );
			hElem.ToElement()->LinkEndChild(hElemText.ToNode());
			hRoot.ToElement()->LinkEndChild(hElem.ToNode());
		}
    }

    *pDoc = doc;

	return true;
}

bool CHardwareInfo::ReadXML(std::string& sXml)
{
    TiXmlDocument doc;
	m_sErrorMsg = "Error reading XML document.";

	doc.Parse(sXml.c_str());

	if(doc.Error())
	{
		m_sErrorMsg = "Not a valid XML document.";
		return false;
	}

	// Get Root element
	TiXmlHandle hRoot = doc.FirstChild("Root");
	if(hRoot.ToElement()==NULL)
	{
		return false;
	}

	//! Read generator version
	const char* szGeneratorVersion = hRoot.ToElement()->Attribute("version");
	if(szGeneratorVersion)
	{
		m_Info.m_sGeneratorVersion = strconv::a2w(szGeneratorVersion);
	}

	TiXmlHandle hComputer = hRoot.FirstChild("Computer");
	if(hComputer.ToElement()==NULL)
		return false;

    // Read SystemType
	{
		TiXmlHandle hElem = hComputer.FirstChild("SystemType");
		if(hElem.ToElement()!=NULL && hElem.FirstChild().ToText()!=NULL)
		{
			const char* szVal = hElem.FirstChild().ToText()->Value();
			if(szVal)
			{
			    std::wstring sType = strconv::utf82w(szVal);
			    if(sType==L"Windows")
                    m_Info.m_OpSysType = ST_WINDOWS;
                else if(sType==L"Linux")
                    m_Info.m_OpSysType = ST_LINUX;
                else
                    m_Info.m_OpSysType = ST_UNDEFINED;
			}

		}
	}

	// Read ComputerName
	{
		TiXmlHandle hElem = hComputer.FirstChild("ComputerName");
		if(hElem.ToElement()!=NULL && hElem.FirstChild().ToText()!=NULL)
		{
			const char* szVal = hElem.FirstChild().ToText()->Value();
			if(szVal)
				m_Info.m_sComputerName = strconv::utf82w(szVal);
		}
	}

	// Read ComputerManufacturer
	{
		TiXmlHandle hElem = hComputer.FirstChild("ComputerManufacturer");
		if(hElem.ToElement()!=NULL && hElem.FirstChild().ToText()!=NULL)
		{
			const char* szVal = hElem.FirstChild().ToText()->Value();
			if(szVal)
				m_Info.m_sComputerManufacturer = strconv::utf82w(szVal);
		}
	}

	// Read Domain
	{
		TiXmlHandle hElem = hComputer.FirstChild("Domain");
		if(hElem.ToElement()!=NULL && hElem.FirstChild().ToText()!=NULL)
		{
			const char* szVal = hElem.FirstChild().ToText()->Value();
			if(szVal)
				m_Info.m_sDomain = strconv::utf82w(szVal);
		}
	}

    // Read MachineType
	{
		TiXmlHandle hElem = hComputer.FirstChild("MachineType");
		if(hElem.ToElement()!=NULL && hElem.FirstChild().ToText()!=NULL)
		{
			const char* szVal = hElem.FirstChild().ToText()->Value();
			if(szVal)
				m_Info.m_sMachineType = strconv::utf82w(szVal);
		}
	}

	// Read CPUName
	{
		TiXmlHandle hElem = hComputer.FirstChild("CPUName");
		if(hElem.ToElement()!=NULL && hElem.FirstChild().ToText()!=NULL)
		{
			const char* szVal = hElem.FirstChild().ToText()->Value();
			if(szVal)
				m_Info.m_sCPUName = strconv::utf82w(szVal);
		}
	}

	// Read OSRelease
	{
		TiXmlHandle hElem = hComputer.FirstChild("OSRelease");
		if(hElem.ToElement()!=NULL && hElem.FirstChild().ToText()!=NULL)
		{
			const char* szVal = hElem.FirstChild().ToText()->Value();
			if(szVal)
				m_Info.m_sOSRelease = strconv::utf82w(szVal);
		}
	}

    // Read OSVersion
	{
		TiXmlHandle hElem = hComputer.FirstChild("OSVersion");
		if(hElem.ToElement()!=NULL && hElem.FirstChild().ToText()!=NULL)
		{
			const char* szVal = hElem.FirstChild().ToText()->Value();
			if(szVal)
				m_Info.m_sOSVersion = strconv::utf82w(szVal);
		}
	}

	// Read NetworkAdapters
	TiXmlHandle hNetworkAdapters = hRoot.FirstChild("NetworkAdapters");
	if(hNetworkAdapters.ToElement()==NULL)
		return false;

	TiXmlHandle hNetworkAdapter = hNetworkAdapters.FirstChild("NetworkAdapter");
	while(hNetworkAdapter.ToElement()!=NULL)
	{
		NetworkAdapterInfo nai;

		// Read Name
		{
			TiXmlHandle hElem = hNetworkAdapter.FirstChild("Name");
			if(hElem.ToElement()!=NULL && hElem.FirstChild().ToText()!=NULL)
			{
				const char* szVal = hElem.FirstChild().ToText()->Value();
				if(szVal)
					nai.m_sAdapterName = strconv::utf82w(szVal);
			}
		}

		// Read Manufacturer
		{
			TiXmlHandle hElem = hNetworkAdapter.FirstChild("Manufacturer");
			if(hElem.ToElement()!=NULL && hElem.FirstChild().ToText()!=NULL)
			{
				const char* szVal = hElem.FirstChild().ToText()->Value();
				if(szVal)
					nai.m_sManufacturer = strconv::utf82w(szVal);
			}
		}

		// Read PNPDeviceID
		{
			TiXmlHandle hElem = hNetworkAdapter.FirstChild("PNPDeviceID");
			if(hElem.ToElement()!=NULL && hElem.FirstChild().ToText()!=NULL)
			{
				const char* szVal = hElem.FirstChild().ToText()->Value();
				if(szVal)
					nai.m_sPNPDeviceID = strconv::utf82w(szVal);
			}
		}

		// Read MacAddress
		{
			TiXmlHandle hElem = hNetworkAdapter.FirstChild("MacAddress");
			if(hElem.ToElement()!=NULL && hElem.FirstChild().ToText()!=NULL)
			{
				const char* szVal = hElem.FirstChild().ToText()->Value();
				if(szVal)
					nai.m_sMacAddress = strconv::utf82w(szVal);
			}
		}

		// Add to the list
		m_Info.m_aNetworkAdapters.push_back(nai);

		// Goto next adapter
		hNetworkAdapter = hNetworkAdapter.ToElement()->NextSibling("NetworkAdapter");
	}

	// Read HardDisks
	TiXmlHandle hHardDisks = hRoot.FirstChild("HardDisks");
	if(hHardDisks.ToElement()==NULL)
		return false;

	TiXmlHandle hHardDisk = hHardDisks.FirstChild("HardDisk");
	while(hHardDisk.ToElement()!=NULL)
	{
		HardDiskInfo hdi;

		// Read Model
		{
			TiXmlHandle hElem = hHardDisk.FirstChild("Model");
			if(hElem.ToElement()!=NULL && hElem.FirstChild().ToText()!=NULL)
			{
				const char* szVal = hElem.FirstChild().ToText()->Value();
				if(szVal)
					hdi.m_sModel = strconv::utf82w(szVal);
			}
		}

		// Read SerialNumber
		{
			TiXmlHandle hElem = hHardDisk.FirstChild("SerialNumber");
			if(hElem.ToElement()!=NULL && hElem.FirstChild().ToText()!=NULL)
			{
				const char* szVal = hElem.FirstChild().ToText()->Value();
				if(szVal)
					hdi.m_sSerialNumber = strconv::utf82w(szVal);
			}
		}

		// Read PNPDeviceID
		{
			TiXmlHandle hElem = hHardDisk.FirstChild("PNPDeviceID");
			if(hElem.ToElement()!=NULL && hElem.FirstChild().ToText()!=NULL)
			{
				const char* szVal = hElem.FirstChild().ToText()->Value();
				if(szVal)
					hdi.m_sPNPDeviceID = strconv::utf82w(szVal);
			}
		}

		// Add to the list
		m_Info.m_aHDDs.push_back(hdi);

		// Goto next HDD
		hHardDisk = hHardDisk.ToElement()->NextSibling("HardDisk");
	}


	// Success
	m_bInfoRetrieved = true;
	m_sErrorMsg.clear();
	return true;
}

std::string CHardwareInfo::GetErrorMsg()
{
	return m_sErrorMsg;
}

