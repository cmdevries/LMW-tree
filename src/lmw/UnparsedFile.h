#ifndef UNPARSED_FILE_H
#define UNPARSED_FILE_H

#include "lmw/StdIncludes.h"

//#include <string>
//using std::string;


namespace lmw {

	class UnparsedFile {

	public:

		UnparsedFile() {		
		}

		~UnparsedFile() { };

		const char* getContent() {
			return _content;
		}

		long getFileSize() {
			return _fileSize;
		}

		void setFileSize(long fileSize) {
			_fileSize = fileSize;
		}

		string getMetadata(string field) {
			if (_metadata.count(field)) return _metadata[field];
		}

		void setMetadata(string key, string value) {
			_metadata[key] = value;
		}

	private:

		long _fileSize;
		const char* _content;
		std::unordered_map<std::string, std::string> _metadata;
	};


} // namespace lmw

#endif	/* UNPARSED_FILE_H */

