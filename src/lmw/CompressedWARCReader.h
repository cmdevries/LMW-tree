#ifndef COMPRESSED_WARC_READER_H
#define COMPRESSED_WARC_READER_H

#include "CompressedArchiveReader.h"

namespace lmw {

	class CompressedWARCReader : public CompressedArchiveReader {
	public:

		CompressedWARCReader() {


		}

		~CompressedWARCReader() {

		}


	private:

		struct BitMapList8Entry {
			char numBits;
			char posns[8];
		};

		BitMapList8Entry *entries;
	};

} // namespace lmw

#endif	/* COMPRESSED_WARC_READER_H */


