//! \file License.cpp
//! \brief License management.
//! \author Oleg Krivtsov
//! \date 2012

#include "stdafx.h"
#include "License.h"
#include "ProtectedFile.h"
#include <tinyxml.h>
#include "strconv.h"
#include "Misc.h"
#include "base64.h"

CLicense::CLicense()
{
	// Init variables
	m_bValid = false;
	m_DateCreated = 0;
	m_nDaysExpiresFromCreation = -1;
	m_pHwInfo = NULL;
	m_bOwnHwInfo = false;
}

CLicense::~CLicense()
{
	Destroy();
}

bool
CLicense::CreateNew(
	 std::wstring sProductID,
	 CustomerInfo* pCustomerInfo,
	 CHardwareInfo* pHwInfo,
	 int nDaysExpiresFromCreation
	 )
{
	bool bResult = false;
	std::wostringstream s;
	m_sErrorMsg = L"Undefined error.";
	wchar_t szBuf[256] = L"";
	struct tm* timeinfo = NULL;
	ProductInfo pi;
	const HardwareInfo* phw = NULL;
	bool bGetInfo = false;

	// Validate input
	if(sProductID.empty() ||
		sProductID.length()>64)
	{
		m_sErrorMsg = L"Product ID is empty or longer than 64 characters.";
		goto cleanup;
	}

	if(pHwInfo==NULL || !pHwInfo->IsValid())
	{
		m_sErrorMsg = L"Invalid hardware info passed.";
		goto cleanup;
	}

	// Validate customer info
	if(!ValidateCustomerInfo(pCustomerInfo))
		goto cleanup;

	// Save customer info internally
	m_CustomerInfo = *pCustomerInfo;

	// Save hardware info
	m_pHwInfo = pHwInfo;
	m_bOwnHwInfo = false;

	// Save product ID
	m_sProductID = sProductID;

	bGetInfo = GetProductInfo(&pi);
	if(!bGetInfo)
	{
		m_sErrorMsg = L"Product ID is invalid";
		goto cleanup;
	}

	if(pi.m_bEval)
	{
		if(nDaysExpiresFromCreation<=0 || nDaysExpiresFromCreation>60)
		{
			m_sErrorMsg = L"Evaluation expiration period is invalid.";
			goto cleanup;
		}
	}
	else
	{
		if(nDaysExpiresFromCreation!=-1)
		{
			m_sErrorMsg = L"Expiration period is not allowed for regular license.";
			goto cleanup;
		}
	}

	phw = m_pHwInfo->GetCurInfo();
	if(!phw)
	{
		m_sErrorMsg = L"Hardware info is invalid";
		goto cleanup;
	}

	// Check OS type in license and OS type in hardware info match
	if(pi.m_OsType==POS_LINUX && phw->m_OpSysType==ST_LINUX ||
		pi.m_OsType==POS_WINDOWS && phw->m_OpSysType==ST_WINDOWS)
	{
		// Correct
	}
	else
	{
		m_sErrorMsg = L"OS type mismatch.";
		goto cleanup;
	}

	// Save expiration period
	m_nDaysExpiresFromCreation = nDaysExpiresFromCreation;

	// Fill creation date
	time(&m_DateCreated);

	timeinfo = localtime(&m_DateCreated);
	wcsftime(szBuf, 256, L"_%Y_%m_%d_%H%M", timeinfo);

	// Format license ID
	s << pCustomerInfo->m_sName.at(0);
	s << pCustomerInfo->m_sSurname.substr(0, min(pCustomerInfo->m_sSurname.length(), 3));
	s << szBuf;
	m_sLicenseID = s.str();

	// Success
	m_bValid = true;
	bResult = true;
	m_sErrorMsg.clear();

cleanup:

	if(!m_bValid)
	{
		std::wstring sErr = m_sErrorMsg;
		Destroy();
		m_sErrorMsg = sErr;
	}

	return bResult;
}

bool CLicense::IsValid()
{
	return m_bValid;
}

void CLicense::Destroy()
{
	// Clear internal license data.
	m_bValid = false;
	//m_sErrorMsg.clear();
	m_sLicenseID.clear();
	m_sProductID.clear();
	m_DateCreated = 0;
	m_nDaysExpiresFromCreation = -1;
	m_CustomerInfo = CustomerInfo();
	if(m_pHwInfo && m_bOwnHwInfo)
	{
		delete m_pHwInfo;
	}
	m_pHwInfo = NULL;
	m_bOwnHwInfo = false;
}

const CustomerInfo* CLicense::GetCustomerInfo()
{
	if(!m_bValid)
		return NULL;

	return &m_CustomerInfo;
}

const HardwareInfo* CLicense::GetHardwareInfo()
{
	if(!m_bValid)
		return NULL;

	if(m_pHwInfo==NULL)
		return NULL;

	return m_pHwInfo->GetCurInfo();
}

std::wstring CLicense::GetLicenseID()
{
	return m_sLicenseID;
}

std::wstring CLicense::GetProductID()
{
	return m_sProductID;
}

bool CLicense::GetProductInfo(ProductInfo* pProductInfo)
{
	// Validate input
	if(pProductInfo==NULL)
		return false; // Invalid param

	std::string sDecodedProductID = base64_decode(strconv::w2a(m_sProductID));

	char* szNext = NULL;

	char* szProductName = strtok_r((char*)sDecodedProductID.c_str(), "-", &szNext);
	if(szProductName==NULL)
		return false;

	pProductInfo->m_sProductName = szProductName;

	char* szFeaturesType = strtok_r(NULL, "-", &szNext);
	if(szFeaturesType==NULL)
		return false;

	if(strcmp(szFeaturesType, "lite")==0)
	{
		pProductInfo->m_bEval = false;
		pProductInfo->m_FeaturesType = FT_LITE;
	}
	else if(strcmp(szFeaturesType, "pro")==0)
	{
		pProductInfo->m_bEval = false;
		pProductInfo->m_FeaturesType = FT_PRO;
	}
	else if(strcmp(szFeaturesType, "eval")==0)
	{
		// Evaluation version.
		pProductInfo->m_bEval = true;
		pProductInfo->m_FeaturesType = FT_PRO;
	}
	else
		return false;

	char* szOSType = strtok_r(NULL, "-", &szNext);
	if(szOSType==NULL)
		return false;

	if(strcmp(szOSType, "windows")==0)
		pProductInfo->m_OsType = POS_WINDOWS;
	else if(strcmp(szOSType, "linux")==0)
		pProductInfo->m_OsType = POS_LINUX;
	else
		return false;

	// Get product major ver
	char* szProductVerMajor = strtok_r(NULL, "-", &szNext);
	if(szProductVerMajor==NULL)
		return false;

	int nProductVerMajor = atoi(szProductVerMajor);
	if(nProductVerMajor<1 || nProductVerMajor>10)
		return false;

	pProductInfo->m_nVerMajor = nProductVerMajor;

	// Get product minor ver	
	char* szProductVerMinor = strtok_r(NULL, "-", &szNext);
	if(szProductVerMinor==NULL)
		return false;

	int nProductVerMinor = atoi(szProductVerMinor);
	if(nProductVerMinor<0 || nProductVerMinor>20)
		return false;
		
	pProductInfo->m_nVerMinor = nProductVerMinor;

	// Build is optional
	char* szProductVerBuild = strtok_r(NULL, "-", &szNext);
	if(szProductVerBuild!=NULL)
	{
		// Get build number
		int nProductVerBuild = atoi(szProductVerBuild);
		if(nProductVerBuild<0 || nProductVerBuild>99)
			return false; // Invalid number

		// Save build number
		pProductInfo->m_nVerBuild = nProductVerBuild;

		// Check if there are other characters after build number
		char* szLast = strtok_r(NULL, "-", &szNext);
		if(szLast!=NULL)
			return false; // Unexpected characters after the end of product ID string.
	}
	else
	{
		pProductInfo->m_nVerBuild = 0;
	}
	
	// Success.
	return true;
}

int CLicense::GetDaysExpiresFromCreation()
{
	// Check if license valid
	if(!m_bValid)
		return -1; // Return a negative number to indicate this license has expired.

	return m_nDaysExpiresFromCreation;
}

int CLicense::GetDaysExpiresFromNow()
{
	// Check if license valid
	if(!m_bValid)
		return -1; // Return a negative number to indicate this license has expired.

	// Get current time.
	time_t CurTime;
	time(&CurTime);

	// Calculate difference in seconds between the expiration time and current time
	time_t DateExpires = GetDateExpires();
	double dDiffSec = difftime(DateExpires, CurTime);

	// Calculate difference in days
	int nDiffDays = (int)ceil(dDiffSec/(60*60*24)); // 1 day = 60 sec * 60 min * 24 hours

	return nDiffDays;
}

time_t CLicense::GetDateCreated()
{
	return m_DateCreated;
}

time_t CLicense::GetDateExpires()
{
	// Check if license valid
	if(!m_bValid)
		return 0;

	// Calculate expiration date
	time_t DateExpires = m_DateCreated + m_nDaysExpiresFromCreation*24*60*60; // 24 hours * 60 min * 60 sec

	return DateExpires;
}

bool CLicense::ValidateCustomerInfo(CustomerInfo* pCustomerInfo)
{
	bool bResult = false;

	// Validate customer info
	if(pCustomerInfo==NULL)
	{
		m_sErrorMsg = L"Customer info missing.";
		goto cleanup;
	}

	// Check customer ID
	if(pCustomerInfo->m_sCustomerID.length()==0 ||
		pCustomerInfo->m_sCustomerID.length()>32)
	{
		m_sErrorMsg = L"Customer ID length is invalid (empty or >32 characters).";
		goto cleanup;
	}

	// Check customer name
	if(pCustomerInfo->m_sName.length()==0 ||
		pCustomerInfo->m_sName.length()>32)
	{
		m_sErrorMsg = L"Customer name is empty or too long (>32 characters).";
		goto cleanup;
	}

	// Check company name
	if(!pCustomerInfo->m_sCompanyName.empty())
	{
		if(pCustomerInfo->m_sCompanyName.length()>32)
		{
			m_sErrorMsg = L"Company name is too long (>32 characters).";
			goto cleanup;
		}
	}

	// Check country name
	if(!pCustomerInfo->m_sCountry.empty())
	{
		if(pCustomerInfo->m_sCountry.length()>32)
		{
			m_sErrorMsg = L"Country name length is invalid.";
			goto cleanup;
		}
	}

	// Customer info is valid
	bResult = true;

cleanup:

	return bResult;
}

std::wstring CLicense::GetErrorMsg()
{
	return m_sErrorMsg;
}

bool CLicense::ReadFile(std::string sFileName, bool bUseTestKey)
{
	bool bResult = false;
	FILE* f = NULL;
	CProtectedFile pf;
	LPBYTE pBuffer = NULL;
	UINT cbBuffer = 0;

	// Destroy, if previously open
	Destroy();

	m_sErrorMsg = L"Unspecified error";

	pf.SetUseTestKeyPair(bUseTestKey);

	// Read XML from protected file.
	if(!pf.Load(sFileName, FALSE, "", ""))
	{
	    m_sErrorMsg = strconv::a2w(pf.GetErrorMsg());
		goto cleanup;
	}

	// Get XML
	if(!pf.GetData(&pBuffer, &cbBuffer))
	{
	    m_sErrorMsg = strconv::a2w(pf.GetErrorMsg());
	    goto cleanup;
	}

	// Check buffer
	if(pBuffer==NULL)
	{
        m_sErrorMsg = L"Invalid buffer";
	    goto cleanup;
	}

	// Fill internal variables with info from XML
	if(!ReadXML((const char*)pBuffer))
	{
	    goto cleanup;
	}

	// Success
	bResult = true;
	m_bValid = true;
	m_sErrorMsg.clear();

cleanup:

	if(f)
		fclose(f);

	if(!m_bValid)
		Destroy();

	return bResult;
}

bool CLicense::SaveFile(std::string sFileName, bool bEncrypted, std::string sPKFile,
                        std::string sPassPhrase, bool bUseTestKey)
{
	bool bResult = false;
	m_sErrorMsg = L"Unspecified error";
	std::string sXml;
	CProtectedFile pf;

	// Generate XML
	if(!GenerateXML(sXml))
	{
		m_sErrorMsg = L"Error creating message";
		goto cleanup;
	}

	if(!bEncrypted)
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
		pf.SetUseTestKeyPair(bUseTestKey);

		// Save XML to protected file
		if(!pf.SetData((LPBYTE)sXml.c_str(), (UINT)sXml.length(), false))
			goto cleanup;

		// Save file to disk.
		if(!pf.Save(sFileName, FALSE, sPKFile, sPassPhrase))
			goto cleanup;
	}

	// Success
	bResult = true;
	m_sErrorMsg.clear();

cleanup:

	return bResult;
}

bool CLicense::GenerateXML(std::string& sXml)
{
	bool bResult = false;

	// Check if this license is valid
	if(!m_bValid)
		return false;

	// Generate XML with hardware info
	TiXmlDocument doc;
	if(!m_pHwInfo->GenerateXML(&doc))
		return false;

    // Get root element
	TiXmlHandle hRoot = doc.FirstChild("Root");
	if(hRoot.ToElement()==NULL)
		return false;

	// Add License element
    TiXmlHandle hLicense = new TiXmlElement("License");
    hRoot.ToElement()->LinkEndChild(hLicense.ToNode());

	// Add LicenseID
    {
        TiXmlHandle hElem = new TiXmlElement("LicenseID");
		TiXmlHandle hElemText = new TiXmlText( strconv::w2utf8(m_sLicenseID).c_str() );
        hElem.ToElement()->LinkEndChild(hElemText.ToNode());
        hLicense.ToElement()->LinkEndChild(hElem.ToNode());
    }

	// Add ProductID
    {
        TiXmlHandle hElem = new TiXmlElement("ProductID");
		TiXmlHandle hElemText = new TiXmlText( strconv::w2utf8(m_sProductID).c_str() );
        hElem.ToElement()->LinkEndChild(hElemText.ToNode());
        hLicense.ToElement()->LinkEndChild(hElem.ToNode());
    }

	// Add DateCreated
    {
		std::string sDateCreated;
		Time2String(m_DateCreated, sDateCreated);
        TiXmlHandle hElem = new TiXmlElement("DateCreated");
		TiXmlHandle hElemText = new TiXmlText( strconv::a2utf8(sDateCreated).c_str() );
        hElem.ToElement()->LinkEndChild(hElemText.ToNode());
        hLicense.ToElement()->LinkEndChild(hElem.ToNode());
    }

	// Add Expires
    {
		char szExpires[32];
		sprintf(szExpires, "%d", m_nDaysExpiresFromCreation);
        TiXmlHandle hElem = new TiXmlElement("Expires");
		TiXmlHandle hElemText = new TiXmlText( szExpires );
        hElem.ToElement()->LinkEndChild(hElemText.ToNode());
        hLicense.ToElement()->LinkEndChild(hElem.ToNode());
    }

	// Add CustomerInfo element
	TiXmlHandle hCustomerInfo = new TiXmlElement("CustomerInfo");
    hLicense.ToElement()->LinkEndChild(hCustomerInfo.ToNode());

	// Add CustomerID
    {
        TiXmlHandle hElem = new TiXmlElement("CustomerID");
		TiXmlHandle hElemText = new TiXmlText( strconv::w2utf8(m_CustomerInfo.m_sCustomerID).c_str() );
        hElem.ToElement()->LinkEndChild(hElemText.ToNode());
        hCustomerInfo.ToElement()->LinkEndChild(hElem.ToNode());
    }

	// Add Name
    {
        TiXmlHandle hElem = new TiXmlElement("Name");
		TiXmlHandle hElemText = new TiXmlText( strconv::w2utf8(m_CustomerInfo.m_sName).c_str() );
        hElem.ToElement()->LinkEndChild(hElemText.ToNode());
        hCustomerInfo.ToElement()->LinkEndChild(hElem.ToNode());
    }

	// Add Surname
    {
        TiXmlHandle hElem = new TiXmlElement("Surname");
		TiXmlHandle hElemText = new TiXmlText( strconv::w2utf8(m_CustomerInfo.m_sSurname).c_str() );
        hElem.ToElement()->LinkEndChild(hElemText.ToNode());
        hCustomerInfo.ToElement()->LinkEndChild(hElem.ToNode());
    }

	// Add CompanyName
    {
        TiXmlHandle hElem = new TiXmlElement("CompanyName");
		TiXmlHandle hElemText = new TiXmlText( strconv::w2utf8(m_CustomerInfo.m_sCompanyName).c_str() );
        hElem.ToElement()->LinkEndChild(hElemText.ToNode());
        hCustomerInfo.ToElement()->LinkEndChild(hElem.ToNode());
    }

	// Add Country
    {
        TiXmlHandle hElem = new TiXmlElement("Country");
		TiXmlHandle hElemText = new TiXmlText( strconv::w2utf8(m_CustomerInfo.m_sCountry).c_str() );
        hElem.ToElement()->LinkEndChild(hElemText.ToNode());
        hCustomerInfo.ToElement()->LinkEndChild(hElem.ToNode());
    }

	// Add State
    {
        TiXmlHandle hElem = new TiXmlElement("State");
		TiXmlHandle hElemText = new TiXmlText( strconv::w2utf8(m_CustomerInfo.m_sState).c_str() );
        hElem.ToElement()->LinkEndChild(hElemText.ToNode());
        hCustomerInfo.ToElement()->LinkEndChild(hElem.ToNode());
    }

	// Add City
    {
        TiXmlHandle hElem = new TiXmlElement("City");
		TiXmlHandle hElemText = new TiXmlText( strconv::w2utf8(m_CustomerInfo.m_sCity).c_str() );
        hElem.ToElement()->LinkEndChild(hElemText.ToNode());
        hCustomerInfo.ToElement()->LinkEndChild(hElem.ToNode());
    }

	// Add PostalCode
    {
        TiXmlHandle hElem = new TiXmlElement("PostalCode");
		TiXmlHandle hElemText = new TiXmlText( strconv::w2utf8(m_CustomerInfo.m_sPostalCode).c_str() );
        hElem.ToElement()->LinkEndChild(hElemText.ToNode());
        hCustomerInfo.ToElement()->LinkEndChild(hElem.ToNode());
    }

	// Add Address
    {
        TiXmlHandle hElem = new TiXmlElement("Address");
		TiXmlHandle hElemText = new TiXmlText( strconv::w2utf8(m_CustomerInfo.m_sAddress).c_str() );
        hElem.ToElement()->LinkEndChild(hElemText.ToNode());
        hCustomerInfo.ToElement()->LinkEndChild(hElem.ToNode());
    }

	// Save XML document to memory buffer
    TiXmlPrinter printer;
	printer.SetIndent( "\t" );
	doc.Accept( &printer );

	// Return resulting XML as a string
	sXml = printer.CStr();

	bResult = true;

	return bResult;
}

bool CLicense::ReadXML(std::string sXml)
{
	m_sErrorMsg = L"Unspecified error.";

	TiXmlDocument doc;

	doc.Parse(sXml.c_str());

	if(doc.Error())
	{
		m_sErrorMsg = L"Not a valid XML.";
		return false;
	}

	// Get Root element
	TiXmlHandle hRoot = doc.FirstChild("Root");
	if(hRoot.ToElement()==NULL)
	{
	    m_sErrorMsg = L"Root element not found";
		return false;
	}

	// Read hardware info
	CHardwareInfo* pHwInfo = new CHardwareInfo();
	if(!pHwInfo->ReadXML(sXml))
	{
		delete pHwInfo;
		m_sErrorMsg = L"Invalid hardware info";
		return false;
	}

	m_pHwInfo = pHwInfo;
	m_bOwnHwInfo = true;

	TiXmlHandle hLicense = hRoot.FirstChild("License");
	if(hLicense.ToElement()==NULL)
	{
	    m_sErrorMsg = L"License element not found";
	    return false;
	}

	// Read LicenseID
	{
		TiXmlHandle hElem = hLicense.FirstChild("LicenseID");
		if(hElem.ToElement()!=NULL && hElem.FirstChild().ToText()!=NULL)
		{
			const char* szVal = hElem.FirstChild().ToText()->Value();
			if(szVal)
				m_sLicenseID = strconv::utf82w(szVal);
		}
	}

	// Read ProductID
	{
		TiXmlHandle hElem = hLicense.FirstChild("ProductID");
		if(hElem.ToElement()!=NULL && hElem.FirstChild().ToText()!=NULL)
		{
			const char* szVal = hElem.FirstChild().ToText()->Value();
			if(szVal)
				m_sProductID = strconv::utf82w(szVal);
		}
	}

	// Read Expires
	{
		TiXmlHandle hElem = hLicense.FirstChild("Expires");
		if(hElem.ToElement()!=NULL && hElem.FirstChild().ToText()!=NULL)
		{
			const char* szVal = hElem.FirstChild().ToText()->Value();
			if(szVal)
			{
			    std::wstringstream s;
			    s << strconv::utf82w(szVal);
			    s >> m_nDaysExpiresFromCreation;
			}
		}
	}

	// Read DateCreated
	{
		TiXmlHandle hElem = hLicense.FirstChild("DateCreated");
		if(hElem.ToElement()!=NULL && hElem.FirstChild().ToText()!=NULL)
		{
			const char* szVal = hElem.FirstChild().ToText()->Value();
			if(szVal)
			{
				String2Time(szVal, m_DateCreated);
			}
		}
	}

	// Read CustomerInfo
	TiXmlHandle hCustomerInfo = hLicense.FirstChild("CustomerInfo");
	if(hCustomerInfo.ToElement()==NULL)
	{
	    m_sErrorMsg = L"Customer element not found";
	    return false;
	}

	// Read CustomerID
	{
		TiXmlHandle hElem = hCustomerInfo.FirstChild("CustomerID");
		if(hElem.ToElement()!=NULL && hElem.FirstChild().ToText()!=NULL)
		{
			const char* szVal = hElem.FirstChild().ToText()->Value();
			if(szVal)
				m_CustomerInfo.m_sCustomerID = strconv::utf82w(szVal);
		}
	}

	// Read Name
	{
		TiXmlHandle hElem = hCustomerInfo.FirstChild("Name");
		if(hElem.ToElement()!=NULL && hElem.FirstChild().ToText()!=NULL)
		{
			const char* szVal = hElem.FirstChild().ToText()->Value();
			if(szVal)
				m_CustomerInfo.m_sName = strconv::utf82w(szVal);
		}
	}

	// Read Surname
	{
		TiXmlHandle hElem = hCustomerInfo.FirstChild("Surname");
		if(hElem.ToElement()!=NULL && hElem.FirstChild().ToText()!=NULL)
		{
			const char* szVal = hElem.FirstChild().ToText()->Value();
			if(szVal)
				m_CustomerInfo.m_sSurname = strconv::utf82w(szVal);
		}
	}

	// Read CompanyName
	{
		TiXmlHandle hElem = hCustomerInfo.FirstChild("CompanyName");
		if(hElem.ToElement()!=NULL && hElem.FirstChild().ToText()!=NULL)
		{
			const char* szVal = hElem.FirstChild().ToText()->Value();
			if(szVal)
				m_CustomerInfo.m_sCompanyName = strconv::utf82w(szVal);
		}
	}

	// Read Country
	{
		TiXmlHandle hElem = hCustomerInfo.FirstChild("Country");
		if(hElem.ToElement()!=NULL && hElem.FirstChild().ToText()!=NULL)
		{
			const char* szVal = hElem.FirstChild().ToText()->Value();
			if(szVal)
				m_CustomerInfo.m_sCountry = strconv::utf82w(szVal);
		}
	}

	// Read State
	{
		TiXmlHandle hElem = hCustomerInfo.FirstChild("State");
		if(hElem.ToElement()!=NULL && hElem.FirstChild().ToText()!=NULL)
		{
			const char* szVal = hElem.FirstChild().ToText()->Value();
			if(szVal)
				m_CustomerInfo.m_sState = strconv::utf82w(szVal);
		}
	}

	// Read City
	{
		TiXmlHandle hElem = hCustomerInfo.FirstChild("City");
		if(hElem.ToElement()!=NULL && hElem.FirstChild().ToText()!=NULL)
		{
			const char* szVal = hElem.FirstChild().ToText()->Value();
			if(szVal)
				m_CustomerInfo.m_sCity = strconv::utf82w(szVal);
		}
	}

	// Read PostalCode
	{
		TiXmlHandle hElem = hCustomerInfo.FirstChild("PostalCode");
		if(hElem.ToElement()!=NULL && hElem.FirstChild().ToText()!=NULL)
		{
			const char* szVal = hElem.FirstChild().ToText()->Value();
			if(szVal)
				m_CustomerInfo.m_sPostalCode = strconv::utf82w(szVal);
		}
	}

	// Read Address
	{
		TiXmlHandle hElem = hCustomerInfo.FirstChild("Address");
		if(hElem.ToElement()!=NULL && hElem.FirstChild().ToText()!=NULL)
		{
			const char* szVal = hElem.FirstChild().ToText()->Value();
			if(szVal)
				m_CustomerInfo.m_sAddress = strconv::utf82w(szVal);
		}
	}

	// Success.
	m_sErrorMsg.clear();
	return true;
}

bool CLicense::Authorize(LPCSTR szProductName, int nVerMajor, int nVerMinor, int nVerBuild)
{
	nVerBuild;
	m_sErrorMsg = L"Unspecifed error.";

	if(!IsValid())
	{
		m_sErrorMsg = L"Not a valid license";
		return false;
	}

	// Get product info.
	ProductInfo pi;
	if(!GetProductInfo(&pi))
	{
		m_sErrorMsg = L"Error retrieving product info.";
		return false;
	}

	// Check product name.
	if(pi.m_sProductName.compare(szProductName)!=0)
	{
		// Product name mismatch.
		return false;
	}

	// Check product version
	if(pi.m_nVerMajor!=nVerMajor ||
		pi.m_nVerMinor!=nVerMinor)
	{
		// Version number mismatch
		m_sErrorMsg = L"Product version mismatch";
		return false;
	}

	// Check os type.
#ifdef _WIN32
	if(pi.m_OsType!=POS_WINDOWS)
		return false; // OS type mismatch
#else
	if(pi.m_OsType!=POS_LINUX)
		return false; // OS type mismatch
#endif

	// Read hardware info from computer
	CHardwareInfo hwinfo;
	if(!hwinfo.RetreiveFromComputer())
	{
		m_sErrorMsg = strconv::a2w(hwinfo.GetErrorMsg());
		return false;
	}

	// Compare hardware info.
	if(!hwinfo.Compare(m_pHwInfo))
	{
		m_sErrorMsg = strconv::a2w(hwinfo.GetErrorMsg());
		return false;
	}

	// Check evaluation expiration period.
	if(pi.m_bEval)
	{
		// Get expiration period (since now).
		int nExpires = GetDaysExpiresFromNow();
		if(nExpires<=0)
		{
			m_sErrorMsg = L"License has expired.";
			return false;
		}
	}

	// Success.
	m_sErrorMsg.clear();
	return true;
}


