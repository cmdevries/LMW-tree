#ifndef COMPRESSED_TAR_READER_H
#define COMPRESSED_TAR_READER_H

//-------------------------------------------------------------------
//
// This implementation of a TAR file reader is based on
// the implementation by Uli Köhler 2013.
// http://techoverflow.net/blog/2013/03/29/reading-tar-files-in-c/
//
//-------------------------------------------------------------------

#include "CompressedArchiveReader.h"


namespace indexer {

	#define ASCII_TO_NUMBER(num) ((num)-48) //Converts an ascii digit to the corresponding number (assuming it is an ASCII digit)

	class CompressedTARReader : public CompressedArchiveReader {
	public:

		CompressedTARReader(string fileName, string compressionType) :
			CompressedArchiveReader(fileName, compressionType)
		{
			//Initialize a zero-filled block to compare against
			memset(zeroBlock, 0, 512);
		}

		~CompressedTARReader() {

		}

		UnparsedFile* nextFile() {

			in.read((char*)&currentFileHeader, 512);

			//When a block with zeroes-only is found, the TAR archive ends
			if (memcmp(&currentFileHeader, zeroBlock, 512) == 0) {
				cout << "Found TAR end\n";
				return NULL;
			}



			return &_file;
		}

		struct TARFileHeader {
			char filename[100]; //NUL-terminated
			char mode[8];
			char uid[8];
			char gid[8];
			char fileSize[12];
			char lastModification[12];
			char checksum[8];
			char typeFlag; //Also called link indicator for none-UStar format
			char linkedFileName[100];
			//USTar-specific fields -- NUL-filled in non-USTAR version
			char ustarIndicator[6]; //"ustar" -- 6th character might be NUL but results show it doesn't have to
			char ustarVersion[2]; //00
			char ownerUserName[32];
			char ownerGroupName[32];
			char deviceMajorNumber[8];
			char deviceMinorNumber[8];
			char filenamePrefix[155];
			char padding[12]; //Nothing of interest, but relevant for checksum

			/**
			* Decode a TAR octal number.
			* Ignores everything after the first NUL or space character.
			* @param data A pointer to a size-byte-long octal-encoded
			* @param size The size of the field pointer to by the data pointer
			* @return
			*/
			uint64_t decodeTarOctal(char* data, size_t size = 12) {
				unsigned char* currentPtr = (unsigned char*)data + size;
				uint64_t sum = 0;
				uint64_t currentMultiplier = 1;
				//Skip everything after the last NUL/space character
				//In some TAR archives the size field has non-trailing NULs/spaces, so this is neccessary
				unsigned char* checkPtr = currentPtr; //This is used to check where the last NUL/space char is
				for (; checkPtr >= (unsigned char*)data; checkPtr--) {
					if ((*checkPtr) == 0 || (*checkPtr) == ' ') {
						currentPtr = checkPtr - 1;
					}
				}
				for (; currentPtr >= (unsigned char*)data; currentPtr--) {
					sum += ASCII_TO_NUMBER(*currentPtr) * currentMultiplier;
					currentMultiplier *= 8;
				}
				return sum;
			}
			/**
			* @return true if and only if
			*/
			bool isUSTAR() {
				return (memcmp("ustar", ustarIndicator, 5) == 0);
			}

			/**
			* @return The filesize in bytes
			*/
			size_t getFileSize() {
				return decodeTarOctal(fileSize);
			}

			/**
			* Return true if and only if the header checksum is correct
			* @return
			*/
			bool checkChecksum() {
				//We need to set the checksum to zer
				char originalChecksum[8];
				memcpy(originalChecksum, checksum, 8);
				memset(checksum, ' ', 8);
				//Calculate the checksum -- both signed and unsigned
				int64_t unsignedSum = 0;
				int64_t signedSum = 0;
				for (int i = 0; i < sizeof (TARFileHeader); i++) {
					unsignedSum += ((unsigned char*) this)[i];
					signedSum += ((signed char*) this)[i];
				}
				//Copy back the checksum
				memcpy(checksum, originalChecksum, 8);
				//Decode the original checksum
				uint64_t referenceChecksum = decodeTarOctal(originalChecksum);
				return (referenceChecksum == unsignedSum || referenceChecksum == signedSum);
			}
		};

	private:

		char zeroBlock[512];
		TARFileHeader currentFileHeader;
		bool nextEntryHasLongName = false;
	};

} // namespace indexer 

#endif	/* COMPRESSED_TAR_READER_H */


