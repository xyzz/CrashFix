//! \file ProtectedFile.h
//! \brief Encrypted file management.
//! \author Oleg Krivtsov
//! \date 2012

#pragma once
#include "stdafx.h"

//! \class CProtectedFile
//! \brief An encrypted file that may contain several named entries.
class CProtectedFile
{
public:

	//! Constructor.
	CProtectedFile();

	//! Destructor.
	virtual ~CProtectedFile();

	//! Sets content of the file.
	//! @param[in] pBuffer Data buffer.
	//! @param[in] cbBuffer Size of the buffer.
	//! @param[in] bOwner If set, the data will be owned by the protected file and will be freed on destroy.
	bool SetData(LPBYTE pBuffer, UINT cbBuffer, bool bOwner = true);

	//! Returns content of the file.
	bool GetData(LPBYTE* pBuffer, UINT* pcbBuffer);

	//! This method enables or disables usage of test RSA key pair.
	void SetUseTestKeyPair(bool bEnable);

	//! Loads a file from disk.
	bool Load(std::string sFileName, bool bPrivateDecrypt, std::string sPKFile, std::string sPassPhrase);

	//! Writes the file to disk.
	bool Save(std::string sFileName, bool bPublicEncrypt, std::string sPKFile, std::string sPassPhrase);

	//! Frees all resource being used.
	void Destroy();

	//! Returns last error message.
	std::string GetErrorMsg();

private:

	//! Seeds the RNG.
	void SeedRNG();

	//! Generates Blowfish key and initialization vector.
	bool GenerateBlowfishKey(unsigned char* uchKey, unsigned char* uchIniVector);

    //! Encrypts input buffer using Blowfish encryption algorithm.
    bool BlowfishEncrypt(unsigned char* uchKey, unsigned char* uchIniVector,
		LPBYTE pInBuffer, UINT uInBuffSize, LPBYTE* ppOutBuffer, UINT* puOutBuffSize);

	//! Reads RSA public key stored in memory in PEM format.
	RSA* GetEmbeddedPublicKey();
    
	/* Private variables */

	std::string m_sErrorMsg; //!< Last error message.
	LPBYTE m_pBuffer; //!< Data contained in the file.
	UINT m_cbBuffer;  //!< Size of the data.
	bool m_bDataOwned; //!< If we should destroy the data when freeing resources?
	bool m_bUseTestKeyPair; //!< Should we use test (true) or normal (false) key pair?
};