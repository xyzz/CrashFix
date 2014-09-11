//! \file ProtectedFile.cpp
//! \brief Encrypted file management.
//! \author Oleg Krivtsov
//! \date 2012
#include "stdafx.h"
#include "ProtectedFile.h"

// This is the public key, BASE-64 encoded.
#define PUBLIC_KEY_PEM_BASE64 \
"LS0tLS1CRUdJTiBQVUJMSUMgS0VZLS0tLS0NCk1JR2ZNQTBHQ1NxR1NJYjNEUUVCQVFVQUE0R05B\n" \
"RENCaVFLQmdRQ3l2Y0J1cUx1MTR2UzNOUUdmb25sWmdOYTINCmlqODRSVmZ0THhnQW04dGduS2Js\n" \
"YlRMYUZRTWVnNkZ0aUpuaW56SENMUm1UZ1F4akYyM0Zzdkd5ZWtscEF2c2sNCmJsV1NlRmVkT1FE\n" \
"ZW05dU1lNHhjTVhSM2RtVVBhZDkvekc2Y0xKV1RsZ3lmeFhRRVlRZitaVFQram5Qck0zM2QNCi9K\n" \
"Y1g1WUxXUDBXcklVVkU3UUlEQVFBQg0KLS0tLS1FTkQgUFVCTElDIEtFWS0tLS0t\n"

// This is the test public key, BASE-64 encoded.
#define TEST_PUBLIC_KEY_PEM_BASE64 \
"LS0tLS1CRUdJTiBQVUJMSUMgS0VZLS0tLS0NCk1JSUJJakFOQmdrcWhraUc5dzBCQVFFRkFBT0NB\n" \
"UThBTUlJQkNnS0NBUUVBcHh1SnVmQnJYZWRUTkxuTm5Ld0INCjU4TzFYa0JGbnBac1A0ZTFiY0lR\n" \
"ck9qM3NxaHRvL3pkRHpCdmpNaFpwbHIzQVgwbko1L1dZazhZTlBBU29EVlMNCjlZUnN6bS8xbnM5\n" \
"QXdybmh6UmhsNzlLa0x6V0QzZm5SSnZuSVd1WDV1cjBwRVdKR1lnZER1eFE4OENmQy9MUEgNCm45\n" \
"cS9SSUozYyswMTZ6cjMvTUxpdHNCU0V5U0tzaTVrNWdMbk9NUE1iQTdXMElySXRGTndxZHdpK3RP\n" \
"anV1UlINCndGMWw4bHAzV1JJTGJqdkVvUmtIbWErOGJXYlhmVE50TWxobEpncVNwTlA5QXJaQmRx\n" \
"aWRTNWRrRWloQllzUlgNClcxRnJzd3lTdkJwcElyQkF3Y0ozcU1aRWlGcTdZN2M3UllMZWJCSGFW\n" \
"cmtISnJ1ZTJsWWpuUm5sb3RzUzNhR08NClBRSURBUUFCDQotLS0tLUVORCBQVUJMSUMgS0VZLS0t\n" \
"LS0NCg=="

//! Describes message SHA-1 hash and Blowfish session key
//! This information is encrypted with RSA public key
struct SecretInfo
{
	char szSignature[16]; //!< Signature (to check integrity).
	unsigned char uchSha1Hash[SHA_DIGEST_LENGTH];  //!< SHA-1 hash of unencrypted message.
	unsigned char uchKey[16];       //!< Blowfish session key.
	unsigned char uchIniVector[8];  //!< Blowfish initialization vector.
	char szSignature2[16];  //!< Signature (to check integrity).
};

// Signature strings used in secret info
#define SIGNATURE  "003421jer45p3224"
#define SIGNATURE2 "XPOERTER$2352D34"

CProtectedFile::CProtectedFile()
{
	m_pBuffer = NULL;
	m_cbBuffer = 0;
	m_bDataOwned = false;
	m_bUseTestKeyPair = false;
}

CProtectedFile::~CProtectedFile()
{
	Destroy();
}

void CProtectedFile::Destroy()
{
	if(m_pBuffer && m_bDataOwned)
		delete [] m_pBuffer;
	m_pBuffer = NULL;

	m_cbBuffer = 0;
	m_bDataOwned = false;
}

bool CProtectedFile::SetData(LPBYTE pBuffer, UINT cbBuffer, bool bOwner)
{
	// Free old data
	if(m_pBuffer!=NULL && m_bDataOwned)
		delete [] m_pBuffer;

	// Replace data with new one
	m_pBuffer = pBuffer;
	m_cbBuffer = cbBuffer;
	m_bDataOwned = bOwner;

	return true;
}

bool CProtectedFile::GetData(LPBYTE* pBuffer, UINT* pcbBuffer)
{
	// Init output
	if(pBuffer)
		*pBuffer = NULL;
	if(pcbBuffer)
		*pcbBuffer = 0;

	// Validate input
	if(pBuffer==NULL || pcbBuffer==0)
		return false;

	*pBuffer = m_pBuffer;
	*pcbBuffer = m_cbBuffer;

	return true;
}

void CProtectedFile::SetUseTestKeyPair(bool bEnable)
{
	m_bUseTestKeyPair = bEnable;
}

std::string CProtectedFile::GetErrorMsg()
{
	return m_sErrorMsg;
}

bool CProtectedFile::Load(std::string sFileName, bool bPrivateDecrypt, std::string sPKFile, std::string sPassPhrase)
{
	// This method reads data from encrypted file

	bool bResult = 1;
	BIO* pBio = NULL;
	RSA* pKey = NULL;
	FILE* f = NULL;
	int nRsaSize = 0;
	LPBYTE pEncryptedSecretInfo = NULL;
	LPBYTE pSecretInfo = NULL;
	EVP_CIPHER_CTX* ctx = NULL;
	unsigned char uchSHA1HashBuffer[SHA_DIGEST_LENGTH];
	int nDecryptRes = -1;
	LPBYTE pEncryptedMsg = NULL;
	LPBYTE pOutBuffer = NULL;
	unsigned char* uchSHA1Hash = NULL;
	// Actual decrypted data length
	int nOutLength = 0;
	int nOutLength2 = 0;
    long lMsgSize = 0;
    SecretInfo* psi = NULL;
    long lFileSize = 0;

	m_sErrorMsg = "Unspecified error.";

	// Loading the error strings
    ERR_load_crypto_strings();

	// Load crypto algorithms
	OpenSSL_add_all_algorithms();

	if(bPrivateDecrypt)
	{
		// Read private key
		pBio = BIO_new_file(sPKFile.c_str(), "rb");
		PEM_read_bio_RSAPrivateKey(pBio, &pKey, NULL, (void*)sPassPhrase.c_str());
		if (pKey == NULL)
		{
			// Error reading private key
			char buffer[120];
			ERR_error_string(ERR_get_error(), buffer);
			m_sErrorMsg = "Error reading private key";
			m_sErrorMsg += buffer;
			goto cleanup;
		}
	}
	else
	{
		pKey = GetEmbeddedPublicKey();
	}

	if(pKey==NULL)
	{
        m_sErrorMsg = "Invalid key";
	    goto cleanup;
	}


	nRsaSize = RSA_size(pKey);

	// Open file
	f = fopen(sFileName.c_str(), "rb");
	if(f==NULL)
	{
	    m_sErrorMsg = "Error opening file";
	    goto cleanup; // Error opening file.
	}

	// Determine file size
	fseek(f, 0, SEEK_END);
	lFileSize = ftell(f);
	fseek(f, 0, SEEK_SET);

	// Read encrypted secret info from file
	pEncryptedSecretInfo = new BYTE[nRsaSize];
	if(!fread(pEncryptedSecretInfo, nRsaSize, 1, f))
	{
		m_sErrorMsg = "Error reading secret info from file.";
		goto cleanup;
	}

	pSecretInfo = new BYTE[nRsaSize];
	memset(pSecretInfo, 0, nRsaSize);

	// Decrypt secret info
	if(bPrivateDecrypt)
	{
		nDecryptRes = RSA_private_decrypt(nRsaSize, pEncryptedSecretInfo,
			pSecretInfo, pKey, RSA_PKCS1_OAEP_PADDING);
	}
	else
	{
		nDecryptRes = RSA_public_decrypt(nRsaSize, pEncryptedSecretInfo,
			pSecretInfo, pKey, RSA_PKCS1_PADDING);
	}
	if(nDecryptRes<=0)
	{
		m_sErrorMsg = "Error decrypting secret info.";
		goto cleanup;
	}

	psi = (SecretInfo*)pSecretInfo;

	// Check signatures
	if(memcmp(psi->szSignature, SIGNATURE, 16)!=0 ||
		memcmp(psi->szSignature2, SIGNATURE2, 16)!=0)
	{
		m_sErrorMsg = "Secret info signature mismatch.";
		goto cleanup;
	}

	// Determine encrypted message size
	lMsgSize = lFileSize-ftell(f);
	if(lMsgSize<=0)
	{
		m_sErrorMsg = "Invalid message size.";
		goto cleanup; // Invalid message size
	}

	// Read encrypted message from file
	pEncryptedMsg = new BYTE[lMsgSize];
	memset(pEncryptedMsg, 0, lMsgSize);
	if(1!=fread(pEncryptedMsg, lMsgSize, 1, f))
	{
		m_sErrorMsg = "Error reading encrypted message from file.";
		goto cleanup;
	}

	// Decrypt message with Blowfish algorithm

	// Init cipher context
	ctx = new EVP_CIPHER_CTX;
    EVP_CIPHER_CTX_init(ctx);
	EVP_DecryptInit(ctx, EVP_bf_cbc(), psi->uchKey, psi->uchIniVector);

	// Allocate output buffer
	pOutBuffer = new BYTE[lMsgSize+EVP_MAX_BLOCK_LENGTH];
	if(pOutBuffer==NULL)
	{
		m_sErrorMsg = "Error allocating output buffer.";
		goto cleanup; // Couldn't allocate output buffer
	}


	// Decrypt input buffer
    if(EVP_DecryptUpdate(ctx, pOutBuffer, &nOutLength, pEncryptedMsg, lMsgSize) != 1)
	{
		m_sErrorMsg = "Error decrypting message.";
        goto cleanup; // Decryption error
	}

	// Finalize decryption
    if (EVP_DecryptFinal(ctx, pOutBuffer + nOutLength, &nOutLength2) != 1)
	{
		m_sErrorMsg = "Error finalizing decrypting message.";
        goto cleanup; // Decryption error
	}

	nOutLength += nOutLength2;

	// Check SHA-1 hash
	uchSHA1Hash = SHA1(pOutBuffer, nOutLength, uchSHA1HashBuffer);
	if(memcmp(uchSHA1Hash, psi->uchSha1Hash, SHA_DIGEST_LENGTH)!=0)
	{
		m_sErrorMsg = "Digital signature is invalid";
		goto cleanup; // SHA1 hash mismatch
	}

	// Success.
	m_pBuffer = pOutBuffer;
	m_cbBuffer = nOutLength;
	m_bDataOwned = true;
	bResult = true;
	m_sErrorMsg.clear();

cleanup:

	if(!bResult)
	{
		if(pOutBuffer)
			delete [] pOutBuffer;
	}

	if(pBio)
		BIO_free(pBio);

	if(pEncryptedSecretInfo)
		delete [] pEncryptedSecretInfo;

	if(pSecretInfo)
		delete [] pSecretInfo;

	if(pEncryptedMsg)
		delete [] pEncryptedMsg;

	// Close file
	if(f)
	{
		fclose(f);
		f = NULL;
	}

	if(ctx)
	{
		EVP_CIPHER_CTX_cleanup(ctx);
		delete ctx;
		ctx = NULL;
	}

	if(pKey)
	{
		RSA_free(pKey);
	}

	return bResult;
}

bool CProtectedFile::Save(std::string sFileName, bool bPublicEncrypt, std::string sPKFile, std::string sPassPhrase)
{
	// The message is encrypted with Blowfish symmetric algorithm, whose session key and message's SHA-1 hash
	// are encrypted with RSA public key are written to the beginning of file.

	bool bResult = false;
	m_sErrorMsg = "Unspecified error.";
	FILE* f = NULL;
	unsigned char uchKey[16];
	unsigned char uchIniVector[8];
	unsigned char SHA1HashBuf[SHA_DIGEST_LENGTH];
	unsigned char* uchSHA1Hash = NULL;
	SecretInfo si;
	LPBYTE pEncryptedData = NULL;
	UINT uEncryptedDataSize = 0;
	LPBYTE pEncryptedSecretInfo = NULL;
	BIO* pBio = NULL;
	RSA* pKey = NULL;
	int nEncryptResult = -1;
	int nRsaSize = 0;
	bool bEnc = false;

	if(m_pBuffer==NULL || m_cbBuffer==0)
	{
		m_sErrorMsg = "No data to save.";
		goto cleanup;
	}

	// Loading the error strings
    ERR_load_crypto_strings();

	// Load crypto algorithms
	OpenSSL_add_all_algorithms();

	// Loading the error strings
    ERR_load_crypto_strings();

    // Seed the RNG with secure random data
    SeedRNG();

	// Read RSA key
	if(bPublicEncrypt)
	{
		pKey = GetEmbeddedPublicKey();
	}
	else
	{
		// Read private key
		pBio = BIO_new_file(sPKFile.c_str(), "rb");
		PEM_read_bio_RSAPrivateKey(pBio, &pKey, NULL, (void*)sPassPhrase.c_str());
		if (pKey == NULL)
		{
			// Error reading private key
			char buffer[120];
			ERR_error_string(ERR_get_error(), buffer);
			m_sErrorMsg = "Error reading private key";
			m_sErrorMsg += buffer;
			goto cleanup;
		}
	}

	if(pKey==NULL)
	{
		m_sErrorMsg = "Error while reading public key.";
		goto cleanup;
	}

	// Open file
	f = fopen(sFileName.c_str(), "wb");
	if(f==NULL)
	{
		m_sErrorMsg = "Couldn't open file for writing.";
		goto cleanup;
	}

	// Calculate SHA-1 hash of the buffer, this will be used
	// when decrypting the message to ensure message integrity
	uchSHA1Hash = SHA1((unsigned char*)m_pBuffer, m_cbBuffer, SHA1HashBuf);
	if(uchSHA1Hash==NULL)
	{
		m_sErrorMsg = "Error calculating digital signature.";
		goto cleanup;
	}

	// Generate key and initialization vector for Blowfish algorithm
	if(!GenerateBlowfishKey(uchKey, uchIniVector))
	{
		// Error generating blowfish key
		m_sErrorMsg = "Session key error.";
		goto cleanup;
	}

	// Encrypt data with Blowfish algorithm
	bEnc = BlowfishEncrypt(uchKey, uchIniVector, (LPBYTE)m_pBuffer, (UINT)m_cbBuffer, &pEncryptedData, &uEncryptedDataSize);
	if(!bEnc)
	{
		// Error encrypting XML message with Blowfish
		m_sErrorMsg = "Encryption error";
		goto cleanup;
	}

	// Encrypt SHA-1 hash, Blowfish key and initialization vector with RSA public key
	memcpy(si.szSignature, SIGNATURE, 16);
	memcpy(si.uchSha1Hash, uchSHA1Hash, SHA_DIGEST_LENGTH);
	memcpy(si.uchKey, uchKey, 16);
	memcpy(si.uchIniVector, uchIniVector, 8);
	memcpy(si.szSignature2, SIGNATURE2, 16);

	nRsaSize = RSA_size(pKey);
    if((int)sizeof(SecretInfo)>=nRsaSize)
        goto cleanup; // Too large buffer

    // Encrypting
    pEncryptedSecretInfo = new BYTE[nRsaSize];
    if(bPublicEncrypt)
	{
		nEncryptResult = RSA_public_encrypt(sizeof(SecretInfo), (unsigned char*)&si, pEncryptedSecretInfo, pKey, RSA_PKCS1_OAEP_PADDING);
	}
	else
	{
		nEncryptResult = RSA_private_encrypt(sizeof(SecretInfo), (unsigned char*)&si, pEncryptedSecretInfo, pKey, RSA_PKCS1_PADDING);
	}

	if(nEncryptResult<=0)
	{
		// Error encrypting secret info with RSA public key
		char szErr[256];
		ERR_error_string_n(ERR_get_error(), szErr, 256);
		m_sErrorMsg = "Encryption error.";
		goto cleanup;
	}

	// Dump this all info to file

	if(1!=fwrite(pEncryptedSecretInfo, nRsaSize, 1, f))
	{
		m_sErrorMsg = "Error writing to file.";
		goto cleanup; // Error writing to file
	}

	if(1!=fwrite(pEncryptedData, uEncryptedDataSize, 1, f))
	{
		m_sErrorMsg = "Error writing to file.";
		goto cleanup;
	}

	bResult = true;
	m_sErrorMsg.clear();

cleanup:

	// Clean up

	if(pKey)
	{
		RSA_free(pKey);
	}

	if(pEncryptedData)
		delete [] pEncryptedData;

	if(pEncryptedSecretInfo)
		delete [] pEncryptedSecretInfo;

	// Close the file
	if(f)
		fclose(f);

	// Done
	return bResult;
}

// This function seeds the RNG to perform secure padding.
// It uses the Windows cryptographic API to fetch some
// Cryptographically secure random data. In addition it also
// Uses the openssl function to use whatever is displayed on
// the screen as a source for randomness
void CProtectedFile::SeedRNG()
{
	unsigned char buffer[512];
	memset(buffer, 0, 512);

#ifdef _WIN32
    // Acquire cryptographic provider context
	HCRYPTPROV ctx = NULL;
	// Try to open existing key set
	BOOL bAcquire = CryptAcquireContext(&ctx, L"hw016923in0fo", NULL, PROV_RSA_FULL, 0);
	if(!bAcquire)
	{
		// Try to create new keyset
		bAcquire = CryptAcquireContext(&ctx, L"hw016923in0fo", NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET);
		if(!bAcquire)
			return;
	}

    CryptGenRandom(ctx, sizeof(buffer), buffer);

    RAND_seed(buffer, sizeof(buffer));
    RAND_screen(); // Add content of the screen to PRNG

    CryptReleaseContext(ctx, 0);
#else

	int fd = -1;
	if ((fd = open ("/dev/urandom", O_RDONLY)) == -1)
	{
		// open error
		return;
	}

	if ((read (fd, buffer, 512)) == -1)
	{
		// read key error
		return;
	}

	close (fd);

	RAND_seed(buffer, sizeof(buffer));
#endif


}

// This small function creates a new memory BIO and reads the RSA key
RSA* CProtectedFile::GetEmbeddedPublicKey()
{
	// Create base-64 BIO
	BIO* b64 = BIO_new(BIO_f_base64());
	// Create a new memory buffer BIO
	BIO *bp = BIO_new_mem_buf((void*)(m_bUseTestKeyPair?TEST_PUBLIC_KEY_PEM_BASE64:PUBLIC_KEY_PEM_BASE64), -1);
	// Chain two BIOs
	bp = BIO_push(b64, bp);

	char buf[512];
	memset(buf, 0, 512);
	BIO_read(bp, buf, 512);

	BIO *bp2 = BIO_new_mem_buf(buf, -1);

	// And read the RSA key from it
    RSA *pKey = PEM_read_bio_RSA_PUBKEY(bp2, 0, 0, 0);

	// Clean up
    BIO_free_all(bp);
	BIO_free(bp2);

    return pKey;
}

// Helper function that generates a 16-byte key
// and 8-byte initialization vector for Blowfish encryption algorithm
bool CProtectedFile::GenerateBlowfishKey(unsigned char* uchKey, unsigned char* uchIniVector)
{
	// Init output
	memset(uchKey, 0, 16);
	memset(uchIniVector, 0, 8);

#ifdef _WIN32

	// Acquire cryptographic provider context
	HCRYPTPROV ctx = NULL;
	// Try to open existing key set
	BOOL bAcquire = CryptAcquireContext(&ctx, L"hw016923in0fo", NULL, PROV_RSA_FULL, 0);
	if(!bAcquire)
	{
		// Try to create new keyset
		bAcquire = CryptAcquireContext(&ctx, L"hw016923in0fo", NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET);
		if(!bAcquire)
			return false;
	}

	// Fill key buffer with random numbers
    CryptGenRandom(ctx, 16, uchKey);

	// Fill IV buffer with random numbers
    CryptGenRandom(ctx, 8, uchIniVector);

	// Release cryptographic provider context
    CryptReleaseContext(ctx, 0);

#else

	int fd = -1;
	if ((fd = open ("/dev/urandom", O_RDONLY)) == -1)
	{
		// open error
		return false;
	}

	if ((read (fd, uchKey, 16)) == -1)
	{
		// read key error
		return false;
	}

	if ((read (fd, uchIniVector, 8)) == -1)
	{
		// read iv error
		return false;
	}

	close (fd);

#endif

	return true;
}

bool CProtectedFile::BlowfishEncrypt(unsigned char* uchKey, unsigned char* uchIniVector,
									LPBYTE pInBuffer, UINT uInBuffSize, LPBYTE* ppOutBuffer, UINT* puOutBuffSize)
{
	// This method encrypts a memory buffer using Blowfish algorithm
	// and returns output memory buffer.

	// Init output
	if(ppOutBuffer)
		*ppOutBuffer=NULL;
	if(puOutBuffSize)
		*puOutBuffSize=0;

	// Validate input
	if(uchKey==NULL || uchIniVector==NULL || pInBuffer==NULL || uInBuffSize==0 || ppOutBuffer==NULL || puOutBuffSize==NULL)
		return false; // Invalid input

	// Init cipher context
    EVP_CIPHER_CTX ctx;
    EVP_CIPHER_CTX_init (& ctx);
    EVP_EncryptInit (&ctx, EVP_bf_cbc(), uchKey, uchIniVector);

	// Allocate output buffer
	LPBYTE pOutBuffer = new BYTE[uInBuffSize+EVP_MAX_BLOCK_LENGTH];
	if(pOutBuffer==NULL)
		return false; // Couldn't allocate output buffer

	// Actual encrypted data length
	int nOutLength = 0;
	int nOutLength2 = 0;

	// Encrypt input buffer
    if(EVP_EncryptUpdate(&ctx, pOutBuffer, &nOutLength, pInBuffer, uInBuffSize) != 1)
       return false; // Encryption error

	// Finalize encryption
    if (EVP_EncryptFinal(&ctx, pOutBuffer + nOutLength, &nOutLength2) != 1)
       return false; // Encryption error

	nOutLength += nOutLength2;

	// Clean up
    EVP_CIPHER_CTX_cleanup(&ctx);

	// Return out buffer
	*ppOutBuffer = pOutBuffer;
	*puOutBuffSize = nOutLength;

	// Done
    return true;
}
