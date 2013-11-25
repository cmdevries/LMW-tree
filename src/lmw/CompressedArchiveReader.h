#ifndef COMPRESSED_ARCHIVE_READER_H
#define COMPRESSED_ARCHIVE_READER_H

#include "lmw/StdIncludes.h"
#include "lmw/UnparsedFile.h"

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/bzip2.hpp>

namespace lmw {

	//-----------------------------------------------------------
	// Abstract Class for reading compressed archive files.
	// 
	// Derived classes should override the nextFile() method.
	//-----------------------------------------------------------
	class CompressedArchiveReader {
	public:

		CompressedArchiveReader() {			
		}

		CompressedArchiveReader(string fileName, string compressionType) : 
			_fileName(fileName),
			_compressionType(compressionType)			
		{

		}

		~CompressedArchiveReader() {
		}	

		// Pure virtual method to be overriden
		virtual UnparsedFile* nextFile() = 0;

	protected:

		string _compressionType;
		string _fileName;
		UnparsedFile _file;

	};

} // namespace lmw

#endif	/* COMPRESSED_ARCHIVE_READER_H */

