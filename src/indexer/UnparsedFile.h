#ifndef UNPARSED_FILE_H
#define UNPARSED_FILE_H

#include <map>
#include <string>
#include <vector>

namespace indexer {

    using std::map;
    using std::string;
	
    class UnparsedFile {
	public:

		UnparsedFile() {		
		}

		~UnparsedFile() { };

		const vector<char>& getContent() const {
			return _content;
		}

		long getFileSize() const {
			return _content.size();
		}

		const string& getMetadata(const string& field) {
			if (_metadata.count(field)) return _metadata[field];
		}

		void setMetadata(const string& field, const string& value) {
			_metadata[field] = value;
		}

	private:
        UnparsedFile(const UnparsedFile&);
        void operator=(const UnparsedFile&);

		vector<char>  _content;
		map<string, string> _metadata;
	};


} // namespace indexer

#endif	/* UNPARSED_FILE_H */

