//! \file License.h
//! \brief License management.
//! \author Oleg Krivtsov
//! \date 2012

#pragma once
#include "stdafx.h"
#include "HardwareInfo.h"

//! Product IDs (v.1.0.x)
#define CRASHFIX_EVAL_LINUX_1_0      L"Y3Jhc2hmaXgtZXZhbC1saW51eC0xLTA="
#define CRASHFIX_PRO_LINUX_1_0       L"Y3Jhc2hmaXgtcHJvLWxpbnV4LTEtMA=="
#define CRASHFIX_EVAL_WINDOWS_1_0    L"Y3Jhc2hmaXgtZXZhbC13aW5kb3dzLTEtMA=="
#define CRASHFIX_PRO_WINDOWS_1_0     L"Y3Jhc2hmaXgtcHJvLXdpbmRvd3MtMS0w"

//! Product IDs (v.1.0.0)
#define CRASHFIX_EVAL_LINUX_1_0_0    L"Y3Jhc2hmaXgtZXZhbC1saW51eC0xLjAuMA=="
#define CRASHFIX_PRO_LINUX_1_0_0     L"Y3Jhc2hmaXgtcHJvLWxpbnV4LTEtMC0w"
#define CRASHFIX_EVAL_WINDOWS_1_0_0  L"Y3Jhc2hmaXgtZXZhbC13aW5kb3dzLTEtMC0w"
#define CRASHFIX_PRO_WINDOWS_1_0_0   L"Y3Jhc2hmaXgtcHJvLXdpbmRvd3MtMS0wLTA="


//! Customer information
struct CustomerInfo
{
	std::wstring m_sCustomerID;   //!< Customer's unique identifier string.
	std::wstring m_sName;         //!< Name of the person this license was granted to.
	std::wstring m_sSurname;      //!< Surname of the person.
	std::wstring m_sCompanyName;  //!< Optional name of the company where the customer works.
	std::wstring m_sCountry;      //!< Country name.
	std::wstring m_sState;        //!< State/province/region.
	std::wstring m_sCity;         //!< City.
	std::wstring m_sPostalCode;   //!< Postal code/ZIP code.
	std::wstring m_sAddress;      //!< Address.	
};

//! Product's operating system.
enum eProductOsType
{
	POS_UNDEFINED = 0, //!< Undefined.
	POS_LINUX     = 1, //!< Product works on Linux.
	POS_WINDOWS   = 2  //!< Product works on Windows.
};

//! Product features type.
enum eProductFeaturesType
{
	FT_UNDEFINED = 0, //!< Undefined.
	FT_LITE      = 1, //!< Lite version.
	FT_PRO       = 2, //!< Professional version.
	FT_PREMIUM   = 3 //!< Premium version.
};

//! Product information.
struct ProductInfo
{
	//! Constructor.
	ProductInfo()
	{
		m_FeaturesType = FT_UNDEFINED;
		m_bEval = true;
		m_OsType = POS_UNDEFINED;
		m_nVerMajor = 0;
		m_nVerMinor = 0;
		m_nVerBuild = 0;
	}

	std::string m_sProductName; //!< Name of the product.
	eProductFeaturesType m_FeaturesType; //!< Features type.	
	bool m_bEval; //!< Evaluation version flag.
	eProductOsType m_OsType; //!< Operating system type.
	int m_nVerMajor; //!< Major version number.
	int m_nVerMinor; //!< Minor version number.
	int m_nVerBuild; //!< Build number.
};

//! \class CLicense
//! \brief Manages license reading/writing.
class CLicense
{
public:

	//! Constructor.
	CLicense();

	//! Destructor.
	virtual ~CLicense();

	//! Returns true if this is a valid license.
	bool IsValid();

	//! Replaces internal license data with new data (creates new license).
	//! @param[in] sProductID Product ID this license allows to use. Required.
	//! @param[in] pCustomerInfo Information about the customer this license is granted to. Required.
	//! @param[in] pHwInfo Hardware info.	
	//! @param[in] nDaysExpiresFromCreation Count of days since creation date the evaluation license will expire in.	
	bool CreateNew(
		std::wstring sProductID, 
		CustomerInfo* pCustomerInfo, 
		CHardwareInfo* pHwInfo, 		
		int nDaysExpiresFromCreation		
		);
	
	//! Clears internal license data.
	void Destroy();

	//! Returns read-only pointer to customer info, or NULL if license is invalid.
	const CustomerInfo* GetCustomerInfo();

	//! Returns read-only hardware info recorded in the license.
	const HardwareInfo* GetHardwareInfo();

	//! Returns license ID string.
	std::wstring GetLicenseID();

	//! Returns product ID this license allows to use.
	//! The product ID is a BASE-64 encoded string containing product name, version and other parameters.
	std::wstring GetProductID();

	//! Returns product info.
	//! @param[out] pProductInfo Product information recorded in license.
	bool GetProductInfo(ProductInfo* pProductInfo);

	//! Returns the date this license was created.
	time_t GetDateCreated();

	//! Returns the date this license expires.
	time_t GetDateExpires();

	//! Returns count of days since licence creation date this license expires in.
	int GetDaysExpiresFromCreation();

	//! Returns count of days from today this license expires in.
	int GetDaysExpiresFromNow();
	
	//! Reads license from an encrypted file.
	//! @param[in] sFileName License file name to read from.
	//! @param[in] bUseTestKey Flag telling if test key pair should be used (true) or normal key pair should be used (false).
	bool ReadFile(std::string sFileName, bool bUseTestKey=false);

	//! Saves license to an encrypted file.
	//! @param[in] sFileName File name to save to.
	//! @param[in] sPKFile Private key encrypted file.
	//! @param[in] sPassPhrase Private key decrypting pass phrase.
	//! @param[in] bUseTestKey Flag telling if test key pair should be used (true) or normal key pair should be used (false).
	bool SaveFile(std::string sFileName, bool bEncrypted, std::string sPKFile, std::string sPassPhrase, bool bUseTestKey=false);

	//! Compares product info, current hardware info with the info recorded in license,
	//! checks license expiration date. If everthing is correct, returns true; otherwise returns false.
	//! @param[in] zsProductName Product name string.
	//! @param[in] nVerMajor Major version number.
	//! @param[in] nVerMinor Minor version number.
	//! @param[in] nVerBuild Build version number.
	bool Authorize(LPCSTR szProductName, int nVerMajor, int nVerMinor, int nVerBuild);

	//! Returns last error message (or empty string if there were no errors).
	std::wstring GetErrorMsg();

private:

	//! Checks customer info.
	//! @param[in] pCustomerInfo Customer information strucutre.
	bool ValidateCustomerInfo(CustomerInfo* pCustomerInfo);

	//! Generates an XML string based on internal license data.
	//! @param[out] sXml XML as a string.
	bool GenerateXML(std::string& sXml);

	//! Fills internal license data based on data stored in XML.
	//! @param[in] sXml XML as a string.
	bool ReadXML(std::string sXml);	

	/* Privately used variables */

	bool m_bValid;                //!< Is this a valid license?
	std::wstring m_sErrorMsg;     //!< Error message (empty if there were no errors).
	std::wstring m_sLicenseID;    //!< Unique license ID string.
	std::wstring m_sProductID;    //!< ID of the product this license allows to use.
	CustomerInfo m_CustomerInfo;  //!< Customer's info.
	bool m_bOwnHwInfo;            //!< Hardware info owned or not.
	CHardwareInfo* m_pHwInfo;     //!< Customer's hardware info.
	time_t m_DateCreated;         //!< Date/time this license was created.
	int m_nDaysExpiresFromCreation; //!< Count of days since creation date when this license expires.	
};

