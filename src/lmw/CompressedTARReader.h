#ifndef COMPRESSED_TAR_READER_H
#define COMPRESSED_TAR_READER_H

#include "CompressedArchiveReader.h"

namespace lmw {

	#define ASCII_TO_NUMBER(num) ((num)-48) //Converts an ascii digit to the corresponding number (assuming it is an ASCII digit)

	class CompressedTARReader : public CompressedArchiveReader {
	public:

		CompressedTARReader() {


		}

		~CompressedTARReader() {

		}


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
		* (C) Uli Köhler 2013
		*
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


	private:

		struct BitMapList8Entry {
			char numBits;
			char posns[8];
		};

		BitMapList8Entry *entries;
	};

} // namespace lmw

#endif	/* COMPRESSED_TAR_READER_H */


