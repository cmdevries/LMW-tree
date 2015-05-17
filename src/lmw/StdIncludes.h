#ifndef STD_INCLUDES_H
#define STD_INCLUDES_H

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <list>
#include <cmath>
#include <limits>
#include <numeric>
#include <deque>
#include <algorithm>
#include <stdexcept>
#include <atomic>
#include <functional>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <sstream>

#include <boost/random.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/bernoulli_distribution.hpp>
#include <boost/timer/timer.hpp>
#include <boost/program_options.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/format.hpp>


#include "Utils.h"

namespace lmw {

using std::deque;
using std::vector;
using std::list;
using std::string;
using std::cout;
using std::endl;
using std::flush;
using std::max;
using std::atomic;
using std::set;
using std::unordered_set;
using std::unordered_map;
using std::ifstream;
using std::ofstream;
using std::stringstream;
using std::ios;
using std::runtime_error;
using std::hex;
using std::dec;
using std::ends;
using std::move;
using std::unique_ptr;

using boost::format;

typedef boost::mt19937 RND_ENG;

typedef boost::uniform_01<float> RND_UNIFORM01;
typedef boost::normal_distribution<float> RND_NORMAL;
typedef boost::bernoulli_distribution<> RND_BERN;

typedef boost::variate_generator<RND_ENG,RND_UNIFORM01> RND_UNI_GEN_01;
typedef boost::variate_generator<RND_ENG,RND_NORMAL> RND_NORM_GEN_01;
typedef boost::variate_generator<RND_ENG,RND_BERN> RND_BERN_GEN_01;

} // namespace lmw

#endif




