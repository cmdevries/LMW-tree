#ifndef COMPRESSED_WARC_READER_H
#define COMPRESSED_WARC_READER_H

#include "UnparsedFile.h"
#include "CompressedArchiveReader.h"

#include <exception>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

namespace indexer {
    
    using std::runtime_error;

    class CompressedWARCReader : public CompressedArchiveReader {
    public:

        CompressedWARCReader(string fileName, string compressionType) :
                CompressedArchiveReader(fileName, compressionType) {
        }

        ~CompressedWARCReader() {
        }
        

        UnparsedFile* nextFile() {
            int contentLength = readHeader();
            if (contentLength == -1) {
                return NULL;
            } else {
                _file.readContent(in, contentLength);
                return &_file;
            }
        }

    private:
        /**
         * returns -1 when EOF, or length of content otherwise.
         */
        int readHeader() {
            _file.clear();
            string line;
            bool seenContentLength = false;
            bool seenFirstLine = false;
            int contentLength = -1;
            while (getline(in, line)) {
                // determine if header has completed
                boost::algorithm::trim(line);
                if (line.empty()) {
                    if (seenFirstLine && contentLength != -1) {
                        // Header ends on empty line.
                        // However we consume empty lines before the first
                        // line of the header that remain from previous WARC.
                        break;
                    } else {
                        // ClueWeb09 contains malformed headers with blank lines
                        // before the end. So keep searching until
                        // Content-Length has been found.                
                        continue;
                    }
                }

                // parse a field
                size_t keyend = line.find(':');
                if (keyend != string::npos) {
                    size_t valuebegin = keyend + 1;
                    if (keyend < line.size()) {
                        string key = line.substr(0, keyend);
                        string value = line.substr(valuebegin);
                        boost::algorithm::trim(value);
                        _file.setMetadata(key, value);
                        if (boost::iequals(key, "Content-Length")) {
                            contentLength = boost::lexical_cast<int>(value);
                        }
                    }
                }
                seenFirstLine = true;
            }
            return contentLength;
        }

    };

} // namespace indexer

#endif	/* COMPRESSED_WARC_READER_H */


