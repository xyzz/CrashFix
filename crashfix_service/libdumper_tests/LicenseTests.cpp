#include "stdafx.h"
#include "Tests.h"
#include "License.h"
#include "Utils.h"
#include "base64.h"
#include "strconv.h"

class LicenseTests : public CTestCase
{
    BEGIN_TEST_MAP(LicenseTests, "CLicense class tests")
        REGISTER_TEST(Test_CreateNew);
		REGISTER_TEST(Test_ReadSaveFile);

    END_TEST_MAP()

public:

    void SetUp();
    void TearDown();

    void Test_CreateNew();
	void Test_ReadSaveFile();

};

REGISTER_TEST_CASE( LicenseTests );

void LicenseTests::SetUp()
{
}

void LicenseTests::TearDown()
{
}

#define CRASHFIX_PRO_WINDOWS_2_11_60_BASE64 L"Y3Jhc2hmaXgtcHJvLXdpbmRvd3MtMi0xMS02MA=="
#define CRASHFIX_PRO_LINUX_2_11_60_BASE64   L"Y3Jhc2hmaXgtcHJvLWxpbnV4LTItMTEtNjA="
#define CRASHFIX_EVAL_WINDOWS_2_11_60_BASE64 L"Y3Jhc2hmaXgtZXZhbC13aW5kb3dzLTItMTEtNjA="
#define CRASHFIX_EVAL_LINUX_2_11_60_BASE64   L"Y3Jhc2hmaXgtZXZhbC1saW51eC0yLTExLTYw"

void LicenseTests::Test_CreateNew()
{
    CLicense license;
	CustomerInfo ci;
	CHardwareInfo hwinfo;
	ProductInfo pi;
	bool bAuthorize = false;
	bool bAuthorize2 = false;
	bool bAuthorize3 = false;
	bool bCreate = false;
	const CustomerInfo* pCustomerInfo = NULL;
	const HardwareInfo* pHwInfo = NULL;
	bool bGetProductInfo = false;

#ifdef _WIN32
	const wchar_t* szProductID = CRASHFIX_EVAL_WINDOWS_2_11_60_BASE64;
#else
	const wchar_t* szProductID = CRASHFIX_EVAL_LINUX_2_11_60_BASE64;
#endif

	// Get hardware info
	bool bGetHwInfo = hwinfo.RetreiveFromComputer();
	TEST_ASSERT(bGetHwInfo==true);

	// Assume licence is invalid initially
	TEST_ASSERT(!license.IsValid());

	ci.m_sCustomerID = L"agray_2012_02_04";
	ci.m_sName = L"Abraham";
	ci.m_sSurname = L"Gray";
	ci.m_sCompanyName = L"Treasure Island Inc.";
	ci.m_sCountry = L"USA";
	ci.m_sState = L"Washington DC.";
	ci.m_sCity = L"Washington";
	ci.m_sPostalCode = L"325345";
	ci.m_sAddress = L"Roosvelt Ave. block 1043a";

	// Create new license - expect success
	bCreate = license.CreateNew(szProductID, &ci, &hwinfo, 60);
	TEST_ASSERT(bCreate);

	// Assume licence is valid
	TEST_ASSERT(license.IsValid());

	// Assume error message is empty
	TEST_ASSERT(license.GetErrorMsg().empty());

	// Check customer info
	pCustomerInfo = license.GetCustomerInfo();
	TEST_ASSERT(pCustomerInfo!=NULL);
	TEST_ASSERT(pCustomerInfo->m_sCustomerID==ci.m_sCustomerID);
	TEST_ASSERT(pCustomerInfo->m_sName==ci.m_sName);
	TEST_ASSERT(pCustomerInfo->m_sSurname==ci.m_sSurname);
	TEST_ASSERT(pCustomerInfo->m_sCompanyName==ci.m_sCompanyName);
	TEST_ASSERT(pCustomerInfo->m_sCountry==ci.m_sCountry);
	TEST_ASSERT(pCustomerInfo->m_sState==ci.m_sState);
	TEST_ASSERT(pCustomerInfo->m_sCity==ci.m_sCity);
	TEST_ASSERT(pCustomerInfo->m_sPostalCode==ci.m_sPostalCode);
	TEST_ASSERT(pCustomerInfo->m_sAddress==ci.m_sAddress);

	// Check hardware info
	pHwInfo = license.GetHardwareInfo();
	TEST_ASSERT(pCustomerInfo!=NULL);

	// Check product ID
	TEST_ASSERT(license.GetProductID()==szProductID);

	// Check product info
	bGetProductInfo = license.GetProductInfo(&pi);
	TEST_ASSERT(bGetProductInfo==true);
	TEST_ASSERT(pi.m_sProductName=="crashfix");
#ifdef _WIN32
	TEST_ASSERT(pi.m_OsType==POS_WINDOWS);
#else
	TEST_ASSERT(pi.m_OsType==POS_LINUX);
#endif
	TEST_ASSERT(pi.m_FeaturesType==FT_PRO);
	TEST_ASSERT(pi.m_nVerMajor==2);
	TEST_ASSERT(pi.m_nVerMinor==11);
	TEST_ASSERT(pi.m_nVerBuild==60);

	// Check expiration period
	TEST_ASSERT(license.GetDaysExpiresFromNow()==60);

	// Check authorize - assume success
	bAuthorize = license.Authorize("crashfix", 2, 11, 60);
	TEST_ASSERT(bAuthorize);

	// Try to authorize with incorrect version - assume failure
	bAuthorize2 = license.Authorize("crashfix", 1, 11, 60);
	TEST_ASSERT(!bAuthorize2);

	// Try to authorize with incorrect product name - assume failure
	bAuthorize3 = license.Authorize("croshfax", 2, 11, 60);
	TEST_ASSERT(!bAuthorize3);

	// Free license
	license.Destroy();

	// Assume licence is invalid after destroying it
	TEST_ASSERT(!license.IsValid());

	// Assume customer info can't be accessed
	pCustomerInfo = license.GetCustomerInfo();
	TEST_ASSERT(pCustomerInfo==NULL);

	// Assume hardware info can't be accessed
	pHwInfo = license.GetHardwareInfo();
	TEST_ASSERT(pCustomerInfo==NULL);

    __TEST_CLEANUP__;
}

void LicenseTests::Test_ReadSaveFile()
{
	CLicense license;
	CLicense license2;
	CustomerInfo ci;
	CHardwareInfo hwinfo;
	wstring sPKFile = Utils::GetTestDataFolder();
#ifdef _WIN32
    sPKFile += L"keys\\testprivkey.pem";
#else
    sPKFile += L"keys/testprivkey.pem";
#endif
	ProductInfo pi;
	const CustomerInfo* pCustomerInfo = NULL;
	bool bGetHwInfo = false;
	bool bCreate = false;
	bool bSave = false;
	bool bSave2 = false;
	bool bRead = false;
	bool bRead2 = false;
	bool bGetProductInfo = false;

#ifdef _WIN32
	const wchar_t* szProductID = CRASHFIX_PRO_WINDOWS_2_11_60_BASE64;
#else
	const wchar_t* szProductID = CRASHFIX_PRO_LINUX_2_11_60_BASE64;
#endif

    std::string s1 = strconv::w2utf8(L"約翰");
    std::wstring s2 = strconv::utf82w(s1);

	// Get hardware info
	bGetHwInfo = hwinfo.RetreiveFromComputer();
	TEST_ASSERT(bGetHwInfo==true);

	// Assume licence is invalid initially
	TEST_ASSERT(!license.IsValid());

	ci.m_sCustomerID = L"agray_2012_02_04";
	ci.m_sName = L"約翰";
	ci.m_sSurname = L"灰色";
	ci.m_sCompanyName = L"金銀島公司";
	ci.m_sCountry = L"USA";
	ci.m_sState = L"Washington DC.";
	ci.m_sCity = L"Washington";
	ci.m_sPostalCode = L"325345";
	ci.m_sAddress = L"Roosvelt Ave. block 1043a";

	// Create new license - expect success
	bCreate = license.CreateNew(szProductID, &ci, &hwinfo, -1);
	TEST_ASSERT(bCreate);

	// Assume licence is valid
	TEST_ASSERT(license.IsValid());

	// Save license to file - assume success.
	bSave = license.SaveFile("testlicense.lic", true, strconv::w2a(sPKFile), "testpwd", true);
	TEST_ASSERT(bSave);

	// Assume error message is empty
	TEST_ASSERT(license.GetErrorMsg().empty());

	// Save license to file with wrong password - assume failure.
	bSave2 = license.SaveFile("testlicense.lic", true, strconv::w2a(sPKFile), "testpwd2");
	TEST_ASSERT(!bSave2);

	// Assume error message is non-empty
	TEST_ASSERT(!license.GetErrorMsg().empty());

	// Read license from file with wrong key - assume failure
	bRead = license2.ReadFile("testlicense.lic");
	TEST_ASSERT(!bRead);

	// Read license from file - assume success
	bRead2 = license2.ReadFile("testlicense.lic", true);
	TEST_ASSERT(bRead2);

	// Assume license is valid
	TEST_ASSERT(license2.IsValid());

	// Assume error message is empty
	TEST_ASSERT(license2.GetErrorMsg().empty());

	// Check product info
	bGetProductInfo = license2.GetProductInfo(&pi);
	TEST_ASSERT(bGetProductInfo==true);
	TEST_ASSERT(pi.m_sProductName=="crashfix");
#ifdef _WIN32
	TEST_ASSERT(pi.m_OsType==POS_WINDOWS);
#else
	TEST_ASSERT(pi.m_OsType==POS_LINUX);
#endif
	TEST_ASSERT(pi.m_FeaturesType==FT_PRO);
	TEST_ASSERT(pi.m_nVerMajor==2);
	TEST_ASSERT(pi.m_nVerMinor==11);
	TEST_ASSERT(pi.m_nVerBuild==60);

	// Check customer info is identical to the original customer info
	pCustomerInfo = license2.GetCustomerInfo();
	TEST_ASSERT(pCustomerInfo!=NULL);
	TEST_ASSERT(pCustomerInfo->m_sCustomerID==ci.m_sCustomerID);
	TEST_ASSERT(pCustomerInfo->m_sName==ci.m_sName);
	TEST_ASSERT(pCustomerInfo->m_sSurname==ci.m_sSurname);
	TEST_ASSERT(pCustomerInfo->m_sCompanyName==ci.m_sCompanyName);
	TEST_ASSERT(pCustomerInfo->m_sCountry==ci.m_sCountry);
	TEST_ASSERT(pCustomerInfo->m_sState==ci.m_sState);
	TEST_ASSERT(pCustomerInfo->m_sCity==ci.m_sCity);
	TEST_ASSERT(pCustomerInfo->m_sPostalCode==ci.m_sPostalCode);
	TEST_ASSERT(pCustomerInfo->m_sAddress==ci.m_sAddress);

	__TEST_CLEANUP__;

	RemoveFile(L"testlicense.lic");
}

