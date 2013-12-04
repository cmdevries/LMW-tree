#ifndef UNPARSED_FILE_H
#define UNPARSED_FILE_H

#include <istream>
#include <unordered_map>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>

namespace indexer {

    using std::istream;
    using std::unordered_map;
    using std::string;
    using std::vector;

    class UnparsedFile {
    public:

        UnparsedFile() {
        }

        ~UnparsedFile() {
        };

        vector<char>& getContent() {
            return _content;
        }
        
        void readContent(istream& is, int contentLength) {
            _content.resize(contentLength);
            is.read(&_content[0], contentLength);
            if (!is) {
                _content.resize(is.gcount());
            }
        }

        long getFileSize() const {
            return _content.size();
        }

        bool hasField(const string& field) {
            return _metadata.find(lower(field)) != _metadata.end();
        }

        /**
         * pre: hasField(field)
         */
        const string& getMetadata(const string& field) {
            return _metadata[lower(field)];
        }

        void setMetadata(const string& field, const string& value) {
            _metadata[lower(field)] = value;
        }
        
        const unordered_map<string, string>::iterator metadataBegin() {
            return _metadata.begin();
        }

        const unordered_map<string, string>::iterator metadataEnd() {
            return _metadata.end();
        }
        
        void clear() {
            _content.clear();
            _metadata.clear();
        }

    private:
        UnparsedFile(const UnparsedFile&);
        void operator=(const UnparsedFile&);
        
        string lower(const string& field) {
            string tmp = field;
            boost::algorithm::to_lower(tmp);
            return tmp;
        }
        
        vector<char> _content;
        unordered_map<string, string> _metadata;
    };


} // namespace indexer

#endif	/* UNPARSED_FILE_H */

