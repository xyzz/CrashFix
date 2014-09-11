#include "stdafx.h"
#include "Tests.h"
#include "ProtectedFile.h"
#include "Utils.h"
#include "strconv.h"
#include "Misc.h"

class ProtectedFileTests : public CTestCase
{
    BEGIN_TEST_MAP(ProtectedFileTests, "CProtectedFile class tests")
        REGISTER_TEST(Test_SavePublicLoadPrivate);
		REGISTER_TEST(Test_SavePrivateLoadPublic);

    END_TEST_MAP()

public:

    void SetUp();
    void TearDown();

	void Test_SavePublicLoadPrivate();
	void Test_SavePrivateLoadPublic();

};

REGISTER_TEST_CASE( ProtectedFileTests );

void ProtectedFileTests::SetUp()
{
}

void ProtectedFileTests::TearDown()
{
}

void ProtectedFileTests::Test_SavePublicLoadPrivate()
{
	std::wstring sPKFile = Utils::GetTestDataFolder();
#ifdef _WIN32
    sPKFile += L"keys\\testprivkey.pem";
#else
    sPKFile += L"keys/testprivkey.pem";
#endif

	CProtectedFile pf;
    CProtectedFile pf2;
	LPBYTE pData = NULL;
	UINT cbData = 0;
	bool bGetData = false;
	bool bSavePublic = false;
	bool bLoad = false;

	const char* szMessage = "Hello world!";

	pf.SetUseTestKeyPair(true);

	// Set data - assume success
	bool bSetData = pf.SetData((LPBYTE)szMessage, (UINT)strlen(szMessage), false);
	TEST_ASSERT(bSetData);

	// Save file using public key encryption - assume success
	bSavePublic = pf.Save("protected.file", true, "", "");
	TEST_ASSERT(bSavePublic);

	// Load file using test private key - assume success
	bLoad = pf2.Load("protected.file", true, strconv::w2a(sPKFile), "testpwd");
	TEST_ASSERT(bLoad);

	// Get data - assume success
	bGetData = pf2.GetData(&pData, &cbData);
	TEST_ASSERT(bGetData);
	TEST_ASSERT(cbData==strlen(szMessage));

	// Compare original and loaded data - expect match
	TEST_ASSERT(memcmp(pData, szMessage, cbData)==0);

    __TEST_CLEANUP__;

	RemoveFile(L"protected.file");
}

void ProtectedFileTests::Test_SavePrivateLoadPublic()
{
	std::wstring sPKFile = Utils::GetTestDataFolder();
#ifdef _WIN32
    sPKFile += L"keys\\testprivkey.pem";
#else
    sPKFile += L"keys/testprivkey.pem";
#endif

	CProtectedFile pf;
    CProtectedFile pf2;
	LPBYTE pData = NULL;
	UINT cbData = 0;
	const char* szMessage = "Hello world, this is me! Whooaa!!!";
	bool bSavePrivate = false;
	bool bLoadPublic = false;
	bool bGetData = false;

	// Use test public and private key pair
	pf.SetUseTestKeyPair(true);
	pf2.SetUseTestKeyPair(true);

	// Set data - assume success
	bool bSetData = pf.SetData((LPBYTE)szMessage, (UINT)strlen(szMessage), false);
	TEST_ASSERT(bSetData);

	// Save file using private key encryption - assume success
	bSavePrivate = pf.Save("protected.file", false, strconv::w2a(sPKFile), "testpwd");
	TEST_ASSERT(bSavePrivate);

	// Load file using test public key - assume success
	bLoadPublic = pf2.Load("protected.file", false, "", "");
	TEST_ASSERT(bLoadPublic);

	// Get data - assume success
	bGetData = pf2.GetData(&pData, &cbData);
	TEST_ASSERT(bGetData);
	TEST_ASSERT(cbData==strlen(szMessage));

	// Compare original and loaded data - expect match
	TEST_ASSERT(memcmp(pData, szMessage, cbData)==0);

    __TEST_CLEANUP__;

	RemoveFile(L"protected.file");
}

