//! \file HardwareInfo.h
//! \brief Hardware information collection.
//! \author Oleg Krivtsov
//! \date 2011

#pragma once
#include "stdafx.h"
#include "LibDumperVersion.h"

#ifdef _WIN32
#pragma comment(lib, "wbemuuid.lib")
#endif

//! Describes network adapter
struct NetworkAdapterInfo
{
    std::wstring m_sAdapterName;   //!< Adapter name
    std::wstring m_sManufacturer;  //!< Adapter manufacturer
    std::wstring m_sPNPDeviceID;   //!< Plug-and-Play device ID
    std::wstring m_sMacAddress;    //!< MAC address
};

//! Hard disk drive info
struct HardDiskInfo
{
    //! Constructor
    HardDiskInfo()
    {
        
    }

    std::wstring m_sModel;        //!< Model
    std::wstring m_sSerialNumber; //!< Serial number.
	std::wstring m_sPNPDeviceID; //!< Manufacturer.
};

//! Operating system type enumaration
enum eSystemType
{
    ST_UNDEFINED = 0, //!< Undefined
    ST_WINDOWS   = 1, //!< Windows
    ST_LINUX     = 2, //!< Linux
};

//! Describes hardware information.
struct HardwareInfo
{
	//! Constructor
	HardwareInfo()
	{
	    m_OpSysType = ST_UNDEFINED;
		m_SystemTime = 0;
	}

	std::wstring m_sGeneratorVersion;      //!< Version of the tool that generated this info.
    eSystemType m_OpSysType;               //!< Operating system type.
    std::wstring m_sMachineType;           //!< Machine type.
    std::wstring m_sOSVersion;             //!< Operating system version.
    std::wstring m_sOSRelease;             //!< Operating system release number.
    std::wstring m_sCPUName;               //!< CPU name.
	time_t m_SystemTime;                   //!< System time this information was captured at.
    std::wstring m_sComputerName;          //!< Computer name
    std::wstring m_sComputerManufacturer;  //!< Computer manufacturer
    std::wstring m_sDomain;                //!< Domain name
	std::vector<NetworkAdapterInfo> m_aNetworkAdapters; //!< The list of network adapters
	std::vector<HardDiskInfo> m_aHDDs;     //!< The list of hard disks.
};

class TiXmlDocument; // Forward declaration

//! \class CHardwareInfo
//! \brief Collects hardware info and dumps it to file.
class CHardwareInfo
{
	friend class CLicense;
public:

	//! Constructor
    CHardwareInfo();

	//! Destructor
    ~CHardwareInfo();

	//! Retrieves info about computer's hardware.
    bool RetreiveFromComputer();

	//! Returns true if this object contains valid hardware info.
	bool IsValid();

	//! Returns a read-only pointer to the hardware info previously
	//! retrieved from computer or read from an encrypted file.
	const HardwareInfo* GetCurInfo();

	//! Frees internal hardware info structure.
	void FreeInfo();

	//! Calculates a fingerprint of this hardware.
	bool CalcFingerPrint(std::wstring& sFingerPrint);

	//! Compares this hardware info with another hardware info.
	//! Returns true if they match, otherwise false.
	bool Compare(CHardwareInfo* pAnotherHwInfo);

	//! Reads hardware info from an encrypted file.
	//! @param[in] sFileName File name to read from.
	//! @param[in] sPKFile Private key file (protected by pass phrase).
	//! @param[in] sPassPhrase Pass phrase.
	bool ReadFile(std::string sFileName, std::string sPKFile, std::string sPassPhrase, bool bUseTestKey=false);

    //! Writes hardware info to file, either encrypted or not.
	//! @param[in] sFileName File name to write to.
	//! @param[in] bEncrypt If set, file will be encrypted with RSA public key.
	bool SaveFile(std::string sFileName, bool bEncrypt=true, bool bUseTestKey=false);

	//! Returns error message.
	std::string GetErrorMsg();

private:

	//! Writes hardware info to XML file and returns it as a string.
	//! @param[out] sXml Output XML string.
	bool GenerateXML(std::string& sXml);

	//! Writes hardware info to XML file and returns it as a TinyXml document.
	//! @param[out] ppDoc Output XML document.
	bool GenerateXML(TiXmlDocument* ppDoc);

	//! Fills internal hardware info structure with data read from XML file.
	//! @param[in] sXml Input XML string.
	bool ReadXML(std::string& sXml);

#ifdef _WIN32
	
	//! Inits WMI interfaces.
	bool InitWMI();

	//! Executes a WMI query.
	//! @param[in] szQuery Query string.
	//! @param[out] ppEnumerator Returned enumerator interface.
    BOOL ExecWMIQuery (LPCWSTR szQuery, IEnumWbemClassObject** ppEnumerator);

	//! Returns machine info.
	bool GetMachineInfoWindows();
	
	//! Returns OS friendly name
	bool GetWindowsFriendlyName(std::wstring& sOSName);

	//! Collects hard disk drive info
	bool GetHDDInfoWindows();
	
	//! Collects info on network adapter
	bool GetNetworkAdapterInfoWindows();

#else
    //! Returns machine info.
    bool GetMachineInfoLinux();

	//! Collects hard disk drive info in Linux
	bool GetHDDInfoLinux();

	//! Collects info on network adapter
	bool GetNetworkAdapterInfoLinux();
#endif

	/* Privately used variables */

#ifdef _WIN32
	// WMI related variables
    CComPtr<IWbemServices> m_pSvc;  //!< WMI service.
    CComPtr<IWbemLocator> m_pLoc;   //!< WMI locator.
#endif
	// Hardware info
	std::string m_sErrorMsg;        //!< Error message.
	bool m_bInfoRetrieved;          //!< If TRUE, info was retrieved.
	HardwareInfo m_Info;            //!< Hardware info.
};

