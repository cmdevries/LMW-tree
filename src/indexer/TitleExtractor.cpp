#include <cstdlib>
#include <iostream>

#include "CompressedWARCReader.h"

#include "boost/algorithm/string.hpp"

using namespace indexer;
using namespace std;

int main(int argc, char** argv) {
    CompressedWARCReader reader("00.warc.gz", "gz");
    for (;;) {
        UnparsedFile* file = reader.nextFile();
        if (!file) {
            break;
        }
        vector<char>& content = file->getContent();
        for (auto it = content.begin(); it != content.end(); ++it) {
            if (*it == '\0' || *it == '\n' || *it == '\r') {
                *it = ' ';
            } else {
                *it = tolower(*it);
            }
        }
        content[content.size() - 1] = '\0';
        char* begin = strstr(&content[0], "<title>");
        char* end = strstr(&content[0], "</title>");
        string title;
        if (begin && end) {
            begin += strlen("<title>");
            while (begin != end) {
                title += *begin;
                begin++;
            }
        }
        boost::algorithm::trim(title);
        
        cout << file->getMetadata("WaRC-tReC-iD") << endl << title << endl;
//        for (auto it = file->metadataBegin(); it != file->metadataEnd(); ++it) {
//            cout << it->first << " = " << it->second << endl;
//        }
//        cin.get();
//        auto& content = file->getContent();
//        for (auto it = content.begin(); it != content.end(); ++it) {
//            cout << *it;
//        }
//        cout << flush;
//        cin.get();
    }
    
    return EXIT_SUCCESS;
}

