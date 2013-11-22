#ifndef BIT_MAP_LIST_8_H
#define BIT_MAP_LIST_8_H

namespace lmw {

class BitMapList8 {
public:

    BitMapList8() : entries(new BitMapList8Entry[256]) {
        //std::cout << "\nInitialising ...";
        for (int i = 0; i < 256; i++) {
            //std::cout << "\n\n-- " << i << "\n";
            int numBits = 0;
            entries[i].numBits = 0;

            for (size_t b = 0; b < 8; b++) {
                if (i & (1 << b)) {
                    entries[i].posns[numBits] = b;
                    numBits++;
                    //std::cout << " " << b;
                }
            }

            entries[i].numBits = numBits;
        }
    }

    void add(unsigned short idx, int* counts, int weight) const {
        //std::cout << "\n--  " << idx << " ... " << entries[idx].numBits;
        for (int i = 0; i < entries[idx].numBits; i++) {
            counts[entries[idx].posns[i]] += weight;
        }
    }

    inline void add1(unsigned short idx, int* counts) const {
        //std::cout << "\n--  " << idx << " ... " << entries[idx].numBits;
        int num = entries[idx].numBits;
        int posn;
        for (int i = 0; i < num; i++) {
            posn = entries[idx].posns[i];
            counts[posn]++;
        }
    }

private:

    struct BitMapList8Entry {
        char numBits;
        char posns[8];
    };

    BitMapList8Entry *entries;
};

} // namespace lmw

#endif	/* BIT_MAP_LIST_8_H */