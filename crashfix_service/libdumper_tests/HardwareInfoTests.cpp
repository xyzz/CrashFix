#include "stdafx.h"
#include "Tests.h"
#include "HardwareInfo.h"
#include "Utils.h"
#include "strconv.h"

class HardwareInfoTests : public CTestCase
{
    BEGIN_TEST_MAP(HardwareInfoTests, "CHardwareInfo class tests")
        REGISTER_TEST(Test_RetrieveFromComputer);
		REGISTER_TEST(Test_ReadSaveFile);
		REGISTER_TEST(Test_Compare);
		REGISTER_TEST(Test_Compare_no_mac);		
    END_TEST_MAP()

public:

    void SetUp();
    void TearDown();

    void Test_RetrieveFromComputer();
	void Test_ReadSaveFile();
	void Test_Compare();
	void Test_Compare_no_mac();	
};

REGISTER_TEST_CASE( HardwareInfoTests );

void HardwareInfoTests::SetUp()
{
}

void HardwareInfoTests::TearDown()
{
}

void HardwareInfoTests::Test_RetrieveFromComputer()
{
    CHardwareInfo hwinfo;
    bool bRetrieve = false;
	std::wstring sFingerPrint;
	bool bCalcFingerPrint = false;

	// Get info - assume NULL (no info retrieved yet)
	const HardwareInfo* pInfo = hwinfo.GetCurInfo();
	TEST_ASSERT(pInfo==NULL);

	// Retrieve hardware info - assume success
	bRetrieve = hwinfo.RetreiveFromComputer();
	TEST_ASSERT(bRetrieve==true);

	// Assume error message is empty
	TEST_ASSERT_MSG(true==hwinfo.GetErrorMsg().empty(),
             "Expected empty error message, while got: %s", hwinfo.GetErrorMsg().c_str());

	// Get retrieved info - assume not-NULL
	pInfo = hwinfo.GetCurInfo();
	TEST_ASSERT(pInfo!=NULL);

	//! Ensure operating system type is correct
#ifdef _WIN32
	TEST_ASSERT(pInfo->m_OpSysType==ST_WINDOWS);
#else
	TEST_ASSERT(pInfo->m_OpSysType==ST_LINUX);
#endif

	// Ensure generator version is correct
	TEST_ASSERT(pInfo->m_sGeneratorVersion == strconv::a2w(LIBDUMPER_VER));

	// Ensure computer name is not empty
	TEST_ASSERT(!pInfo->m_sComputerName.empty());

	// Ensure OS version is not empty
	TEST_ASSERT(!pInfo->m_sOSVersion.empty());

	// Ensure CPU name is not empty
	TEST_ASSERT(!pInfo->m_sCPUName.empty());

	// Ensure there is at least one network adapter
	TEST_ASSERT(pInfo->m_aNetworkAdapters.size()!=0);

#ifdef _WIN32
	// Ensure there is at least one HDD
	TEST_ASSERT(pInfo->m_aHDDs.size()!=0);
#else
    // Under Linux its a pain to get HDD serial, so skip this check
#endif

	// Ensure fingerprint is not empty
	bCalcFingerPrint = hwinfo.CalcFingerPrint(sFingerPrint);
	TEST_ASSERT(bCalcFingerPrint);
	TEST_ASSERT(sFingerPrint.length()==32);

	// Free info
	hwinfo.FreeInfo();

	// Get info - assume NULL (info freed)
	pInfo = hwinfo.GetCurInfo();
	TEST_ASSERT(pInfo==NULL);

    __TEST_CLEANUP__;
}

void HardwareInfoTests::Test_ReadSaveFile()
{
    CHardwareInfo hwinfo;
	CHardwareInfo hwinfo2;
	wstring sPKFile = Utils::GetTestDataFolder();
#ifdef _WIN32
    sPKFile += L"keys\\testprivkey.pem";
#else
    sPKFile += L"keys/testprivkey.pem";
#endif
    bool bSave = false;
    bool bRead = false;
    const HardwareInfo* pHwInfo = NULL;
    const HardwareInfo* pHwInfo2 = NULL;
    size_t i;

	// Retrieve hardware info from computer - assume success
	bool bRetrieve = hwinfo.RetreiveFromComputer();
	TEST_ASSERT(bRetrieve==true);

	// Assume error message is empty
	TEST_ASSERT(true==hwinfo.GetErrorMsg().empty());

	// Save hardware info to encrypted file - assume true.
	bSave = hwinfo.SaveFile("testhw.info", true, true);
	TEST_ASSERT(bSave);

	// Assume error message is empty
	TEST_ASSERT(true==hwinfo.GetErrorMsg().empty());

	// Read hardware info back from file - assume true.
	bRead = hwinfo2.ReadFile("testhw.info", strconv::w2a(sPKFile), "testpwd", true);
	TEST_ASSERT(bRead);

	// Assume error message is empty
	TEST_ASSERT(true==hwinfo2.GetErrorMsg().empty());

	// Compare fields
	pHwInfo = hwinfo.GetCurInfo();
	TEST_ASSERT(pHwInfo!=NULL);
	pHwInfo2 = hwinfo2.GetCurInfo();
	TEST_ASSERT(pHwInfo2!=NULL);

//! Ensure operating system type is correct
#ifdef _WIN32
	TEST_ASSERT(pHwInfo->m_OpSysType==ST_WINDOWS);
#else
	TEST_ASSERT(pHwInfo->m_OpSysType==ST_LINUX);
#endif

	// Ensure generator version is correct
	TEST_ASSERT(pHwInfo->m_sGeneratorVersion == strconv::a2w(LIBDUMPER_VER));

	// Ensure computer name is not empty
	TEST_ASSERT(!pHwInfo->m_sComputerName.empty());

	// Ensure OS version is not empty
	TEST_ASSERT(!pHwInfo->m_sOSVersion.empty());

	// Ensure OS release is not empty
	TEST_ASSERT(!pHwInfo->m_sOSRelease.empty());

	// Ensure OS machine type is not empty
	TEST_ASSERT(!pHwInfo->m_sMachineType.empty());

	// Ensure CPU name is not empty
	TEST_ASSERT(!pHwInfo->m_sCPUName.empty());

	// Ensure there is at least one network adapter
	TEST_ASSERT(pHwInfo->m_aNetworkAdapters.size()!=0);

#ifdef _WIN32
	// Ensure there is at least one HDD
	TEST_ASSERT(pHwInfo->m_aHDDs.size()!=0);
#else
    // Under Linux its a pain to get HDD serial, so skip this check
#endif

	TEST_ASSERT(pHwInfo->m_sGeneratorVersion==pHwInfo2->m_sGeneratorVersion);
	TEST_ASSERT(pHwInfo->m_OpSysType==pHwInfo2->m_OpSysType);
	TEST_ASSERT(pHwInfo->m_sComputerName==pHwInfo2->m_sComputerName);
	TEST_ASSERT_MSG(pHwInfo->m_sComputerManufacturer==pHwInfo2->m_sComputerManufacturer,
		"Expected '%s', while have '%s'", strconv::w2a(pHwInfo->m_sComputerManufacturer).c_str(), 
		strconv::w2a(pHwInfo2->m_sComputerManufacturer).c_str());
	TEST_ASSERT(pHwInfo->m_sDomain==pHwInfo2->m_sDomain);
	TEST_ASSERT_MSG(pHwInfo->m_sCPUName==pHwInfo2->m_sCPUName,
                 "CPU name mismatch: expected '%s', while got '%s'", strconv::w2a(pHwInfo->m_sCPUName).c_str(), strconv::w2a(pHwInfo2->m_sCPUName).c_str());
	TEST_ASSERT(pHwInfo->m_sOSRelease==pHwInfo2->m_sOSRelease);
	TEST_ASSERT(pHwInfo->m_sOSVersion==pHwInfo2->m_sOSVersion);
	TEST_ASSERT(pHwInfo->m_sMachineType==pHwInfo2->m_sMachineType);
	TEST_ASSERT(pHwInfo->m_aNetworkAdapters.size()==pHwInfo2->m_aNetworkAdapters.size());
	TEST_ASSERT(pHwInfo->m_aHDDs.size()==pHwInfo2->m_aHDDs.size());

	for(i=0; i<pHwInfo->m_aNetworkAdapters.size(); i++)
	{
		TEST_ASSERT(pHwInfo->m_aNetworkAdapters[i].m_sAdapterName==pHwInfo2->m_aNetworkAdapters[i].m_sAdapterName);
		TEST_ASSERT(pHwInfo->m_aNetworkAdapters[i].m_sMacAddress==pHwInfo2->m_aNetworkAdapters[i].m_sMacAddress);
		TEST_ASSERT(pHwInfo->m_aNetworkAdapters[i].m_sPNPDeviceID==pHwInfo2->m_aNetworkAdapters[i].m_sPNPDeviceID);
		TEST_ASSERT(pHwInfo->m_aNetworkAdapters[i].m_sManufacturer==pHwInfo2->m_aNetworkAdapters[i].m_sManufacturer);
	}

	for(i=0; i<pHwInfo->m_aHDDs.size(); i++)
	{
		TEST_ASSERT(pHwInfo->m_aHDDs[i].m_sModel==pHwInfo2->m_aHDDs[i].m_sModel);
		TEST_ASSERT(pHwInfo->m_aHDDs[i].m_sSerialNumber==pHwInfo2->m_aHDDs[i].m_sSerialNumber);
		TEST_ASSERT(pHwInfo->m_aHDDs[i].m_sPNPDeviceID==pHwInfo2->m_aHDDs[i].m_sPNPDeviceID);
	}

	__TEST_CLEANUP__;

	RemoveFile(L"testhw.info");
}

void HardwareInfoTests::Test_Compare()
{
    CHardwareInfo hwinfo;
	CHardwareInfo hwinfo2;
	bool bCompare = false;
	bool bRetrieve2 = false;
	bool bRetrieve = false;

	// Retrieve hardware info from computer - assume success
	bRetrieve = hwinfo.RetreiveFromComputer();
	TEST_ASSERT(bRetrieve==true);

	bRetrieve2 = hwinfo2.RetreiveFromComputer();
	TEST_ASSERT(bRetrieve2==true);

	// Compare these hardwares - assume success.
	bCompare = hwinfo.Compare(&hwinfo2);
	TEST_ASSERT(bCompare);

	// Assume error message is empty
	TEST_ASSERT(true==hwinfo.GetErrorMsg().empty());

	__TEST_CLEANUP__;
}

void HardwareInfoTests::Test_Compare_no_mac()
{
    CHardwareInfo hwinfo;
	CHardwareInfo hwinfo2;
	wstring sPKFile = Utils::GetTestDataFolder();
#ifdef _WIN32
    sPKFile += L"keys\\testprivkey.pem";
#else
    sPKFile += L"keys/testprivkey.pem";
#endif
    bool bSave = false;
    bool bRead = false;
    HardwareInfo* pHwInfo = NULL;        
	bool bCompare = false;

	// Retrieve hardware info from computer - assume success
	bool bRetrieve = hwinfo.RetreiveFromComputer();
	TEST_ASSERT(bRetrieve==true);

	// Assume error message is empty
	TEST_ASSERT(true==hwinfo.GetErrorMsg().empty());

	// Remove MAC address
	pHwInfo = const_cast<HardwareInfo*>(hwinfo.GetCurInfo());
	pHwInfo->m_aNetworkAdapters.clear();

	// Save hardware info to encrypted file - assume true.
	bSave = hwinfo.SaveFile("testhw.info", true, true);
	TEST_ASSERT(bSave);

	// Assume error message is empty
	TEST_ASSERT(true==hwinfo.GetErrorMsg().empty());

	// Read hardware info back from file - assume true.
	bRead = hwinfo2.ReadFile("testhw.info", strconv::w2a(sPKFile), "testpwd", true);
	TEST_ASSERT(bRead);

	// Compare - assume true
	bCompare = hwinfo.Compare(&hwinfo2);
	TEST_ASSERT(bCompare);

#ifdef _WIN32
	// Modify HDD
	pHwInfo->m_aHDDs[0].m_sSerialNumber=L"abc";
#else
	// Modify computer name
	pHwInfo->m_sComputerName = L"abc";
#endif

	// Compare - assume failure
	bCompare = hwinfo.Compare(&hwinfo2);
	TEST_ASSERT(!bCompare);

	__TEST_CLEANUP__;
}

