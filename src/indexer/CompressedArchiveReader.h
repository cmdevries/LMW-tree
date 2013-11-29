#ifndef COMPRESSED_ARCHIVE_READER_H
#define COMPRESSED_ARCHIVE_READER_H

#include "lmw/StdIncludes.h"
#include "lmw/UnparsedFile.h"

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/bzip2.hpp>

namespace indexer {

	using boost::iostreams::filtering_istream;
	using boost::iostreams::gzip_decompressor;
	using boost::iostreams::bzip2_decompressor;

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
			fin.open(_fileName, std::ios_base::in | std::ios_base::binary);
			if (_compressionType == "gz" || _compressionType == "tgz") {
				in.push(gzip_decompressor());
			}
			else if (_compressionType == "bz2" || _compressionType == "bzip2") {
				in.push(bzip2_decompressor());
			}
			in.push(fin);
		}

		~CompressedArchiveReader() {
			// Cleanup
			fin.close();
		}	

		// Pure virtual method to be overriden
		virtual UnparsedFile* nextFile() = 0;

	protected:

		string _compressionType;
		string _fileName;
		UnparsedFile _file;

		ifstream fin;
		filtering_istream in;

	};

} // namespace indexer

#endif	/* COMPRESSED_ARCHIVE_READER_H */

