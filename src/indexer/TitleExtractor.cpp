#include <cstdlib>
#include <iostream>

#include "CompressedWARCReader.h"

#include "boost/algorithm/string.hpp"

using namespace indexer;
using namespace std;

int main(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        CompressedWARCReader reader(argv[i], "gz");
        for (;;) {
            UnparsedFile* file = reader.nextFile();
            if (!file) {
                break;
            }
            if (file->hasField("warc-trec-id")) {
                vector<char>& content = file->getContent();
                for (auto it = content.begin(); it != content.end(); ++it) {
                    if (*it == '\0' || *it == '\n' || *it == '\r') {
                        *it = ' ';
                    } else {
                        *it = tolower(*it);
                    }
                }
                content[content.size() - 1] = '\0';
                const char beginTag[] = "<title>";
                char* begin = strstr(&content[0], beginTag);
                const char endTag[] = "<";
                char* end = NULL;
                if (begin) {
                    end = strstr(begin + strlen(beginTag), endTag);
                }
                string title;
                if (begin && end) {
                    begin += strlen(beginTag);
                    while (begin != end) {
                        if (isalnum(*begin) || *begin == ' ') {
                            title += *begin;
                        }
                        begin++;
                    }
                }
                boost::algorithm::trim(title);

                cout << file->getMetadata("WaRC-tReC-iD") << endl << title << endl;
            }
        }
    }

    return EXIT_SUCCESS;
}

