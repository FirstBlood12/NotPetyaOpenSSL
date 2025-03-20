# NotPetyaOpenSSL

A complete rewrite of original NotPetya that works only on MBR disks.
It does not work on GPT disks and it does not encrypt the MFT on GPT disks.

# How It Works?

This NotPetyaOpenSSL project works exactly as original NotPetya but is modified to be decryptable.
It uses OpenSSL library for public key cryptography.

# How NotPetyaOpenSSL Infects MBR Disk?

1. If NotPetya detects MBR disk then NotPetya reads original MBR from sector 0, encrypts every byte of original MBR using
XOR 0x07 and writes the XOR encrypted MBR in sector 34.
2. It generates configuration data for NotPetya kernel like Salsa20 key, 8 byte random nonce and the personal installation key
is the random salsa20 key used for MFT encryption encrypted with public key cryptography secp192k1 with Janus public key
and base58 encoded.
3. It fills all 512 bytes of sector 33 with 0x07 this sector is the verification sector.
4. It will construct NotPetya bootloader by copying the disk ID and the partition table from original MBR(byte 440-510)
into its own bootloader, It will write the newly constructed NotPetya bootloader to sector 0 and it will write its own
16 bit kernel to sector 1-19.
5. It will call undocumented API called NtRaiseHardError with 0xc0000350 error causing operating system to crash.

# How NotPetyaOpenSSL Infects GPT Disk?

NotPetyaOpenSSL does not infect GPT disks.

# How Is The Personal Decryption Code Generated?

1. NotPetyaOpenSSL dropper generates 16 random bytes from Base16 alphabet using CryptGenRandom.
We will call these bytes Salsa20key.
2. Generate a pair of keys: victim private key(victimpriv) and victim public key(victimpub) on secp192k1 curve.
3. Calculate the shared secret based on ECDH, shared_secret = ECDH(victimpriv, Januspub);
The NotPetyaOpenSSL public key is hardcoded in NotPetyaOpenSSL dropper which in this case is also Janus public key,
So petya_key.exe can be used to decrypt NotPetyaOpenSSL by selecting goldeneye on it which is the 'd' option on petya_key.exe.
4. Calculate the AESKEY = SHA512(shared_secret);
5. Encrypt the Salsa20 key using AES-256 ECB with the key AESKEY(first 32 bytes of SHA512 hash of shared secret).
6. Create a array that will contain the victim public key and encrypted Salsa20key.
7. SHA512 hash the array.
8. Create a buffer that will contain the array and the first 2 bytes of the SHA512 hash of the array and base58 encode this buffer.
9. Create a buffer that will contain the final personal decryption code that is base58 encoded (96 bytes).
11. Put the final base58 encoded personal decryption code in sector 32 at offset 0xA9.

# How the MFT(Master File Table) encryption works?
1. After the NotPetyaOpenSSL dropper crashes the system and PC reboots, the BIOS
will read sector 0 in memory at physical address 0x7C00.
2. NotPetya bootloader will be loaded at physical address 0x7C00, NotPetya bootloader will read sector 1-19(NotPetya 16 bit kernel)
in physical memory address 0x8000 and will jump there.
3. NotPetya kernel will read sector 32 in memory buffer and will check if the first byte of sector 32 buffer is 0x01(MFT Encrypted),
(the first byte of sector 32) will always be 0x00(MFT Not Encrypted) after NotPetyaOpenSSLDropper was ran,
So because first byte of sector 32 is 0x00, the first thing NotPetya kernel does is set the first byte of sector 32 buffer to 0x01(MFT Encrypted).
Next it will copy Salsa20key(32 bytes) from sector 32 buffer into a temporary buffer, it will overwrite Salsa20key(32 bytes) of sector 32 buffer with zeroes,
It will write sector 32 buffer back to sector 32 of the drive.
4. NotPetya kernel will read sector 33(verification sector) into a buffer, encrypt it using Salsa20 algorithm with the key from the temporary buffer and
8 byte random nonce from sector 32(just after the salsa20 key, the nonce stays permanent).
5. NotPetya kernel will get MFT location for each NTFS partition on the drive and will compute number of sectors for the entire MFT table for each NTFS partition and it will skip the first 32 MFT sectors.
6. NotPetya kernel will start reading, encrypting with Salsa20 cipher and writing back the MFT Records, NotPetya kernel reads 2 MFT sectors per pass, encrypts them with Salsa20
and writes them back to the drive.
7. While this is done, a number of encrypted MFT records is kept in sector 35(this is done in case victim reboots, he gets key and notpetya kernel knows how much to decrypt),
Also while MFT encryption is done, number of MFT sectors is also updated on fake CHKDSK which encrypts MFT.
8. After all MFT records of every NTFS partition are encrypted, NotPetya kernel triggers a reboot by calling INT 19h.
9. This time BIOS will read NotPetya bootloader and NotPetya bootloader will read and execute NotPetya kernel in memory again.
10. This time NotPetya kernel will read sector 32 again in buffer and it will check the first byte of it, this time first byte of sector 32 is 0x01(MFT Encrypted),
so this time NotPetya kernel displays ransom demand and shows what must be done in order to decrypt the hard drive and shows BTC Address and personal decryption code.

# How the MFT(Master File Table) decryption works?
1. At this stage the Master File Table has already been encrypted using Salsa20 cipher and the key used for encrypting the MFT has been erased from sector 32.
2. NotPetya kernel reads input from user into a buffer(it reads only the first 32 characters),
the characters must be from Base16 alphabet otherwise they are skipped.
3. The entered 32 byte key is ran through 129 rounds of SPONGENT 256/256/16 hashing algorithm.
4. NotPetya kernel will read the 8 byte nonce from sector 32(it is just before the BTC Address).
5. NotPetya kernel will use the 32 byte hash that it got by the user 32 byte key using a SPONGENT 256/256/16 hashing algorithm.
6. Sector 33(verification sector) will be read in memory buffer and it will be decrypted with Salsa20 256 bit using 32 byte hash key from the user and 8 byte nonce from sector 32.
7. NotPetya kernel will check if every byte from decrypted sector 33 buffer is 0x07, if it is then means the key is correct and will be used to decrypt the MFT but if is not
then NotPetya kernel prints Incorrect key and asks for key again.
8. If entered key is correct NotPetya kernel will use it with 8 byte nonce to decrypt the MFT sectors and it will display Decrypting sector with a progress.
9. After all MFT sectors of all NTFS partitions are decrypted NotPetya kernel will restore original MBR by reading sector 34 in buffer, decrypting every byte of it with XOR 0x07
and writing it to sector 0.

#NotPetya Salsa20 Encryption small flaw!
The s20_littleendian in notpetya kernel looks like this:

static int16_t s20_littleendian(uint8_t *b)
{
  return b[0] +
         (b[1] << 8);
}

Because of this the Salsa20 key is halved meaning only 16 bytes of the actual 32 byte salsa20 key are used in MFT encryption.
However cracking 16 byte key is still too much for current technology.

#Original Author of Petya publishes his secp192k1 private key
After almost one year of RedPetya, on June 27 2017 a massive NotPetya(malware based on GoldenEye kernel) malware cyberattack appeared
that was actually wiper and destroyed MFT of infected computers it also used EternalBlue to spread across local networks like a worm.
This forced the original author of Petya to publish his secp192k1 private key:
https://blog.malwarebytes.com/cybercrime/2017/07/the-key-to-the-old-petya-has-been-published-by-the-malware-author/
And program for decrypting Red Petya, Green Petya and Mischa as well as GoldenEye has been created:
https://github.com/hasherezade/petya_key
This program doesnt works on NotPetya because NotPetya Salsa20 keys are not encrypted and turned into personal codes but instead erased and lost forever.
But it will work for NotPetyaOpenSSL because NotPetyaOpenSSL Salsa20 keys are encrypted with Janus public key and turned into personal codes.
Just select GoldenEye on petya_key.exe because NotPetya as well as NotPetyaOpenSSL are based on GoldenEye ransomware.
As well it doesnt works for PetrWrap ransomware which is based on Mischa v2 DLL:
https://securelist.com/petrwrap-the-new-petya-based-ransomware-used-in-targeted-attacks/77762/
Because they choose different curve prime192v1 or secp192r1 and they use their own public and private key.
But petya_key works for NotPetyaOpenSSL.

# Prerequisites:

Microsoft Visual Studio 2010 and later Only use Win32/Release configuration because Debug is not configured properly.
