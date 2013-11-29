#ifndef COMPRESSED_WARC_READER_H
#define COMPRESSED_WARC_READER_H

//-------------------------------------------------------------------
//
// This implementation of a WARC file reader is based on
// the implementation contained in the Indri search engine.
// http://www.lemurproject.org/indri.php
//
//-------------------------------------------------------------------

#include "CompressedArchiveReader.h"

namespace indexer {

	class CompressedWARCReader : public CompressedArchiveReader {
	public:

		CompressedWARCReader() {

		}

		~CompressedWARCReader() {
		}

		UnparsedFile* nextFile() {

			return &_file;
		}


	private:


	};

} // namespace indexer

#endif	/* COMPRESSED_WARC_READER_H */


