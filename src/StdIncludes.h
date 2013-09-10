#ifndef STD_INCLUDES_H
#define STD_INCLUDES_H


#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <list>
#include <cmath>
#include <limits>
#include <numeric>
#include <deque>

#include <boost/random.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/bernoulli_distribution.hpp>
#include <boost/timer/timer.hpp>
#include <boost/program_options.hpp>
#include <boost/dynamic_bitset.hpp>

#include "Utils.h"

using std::deque;
using std::vector;
using std::list;
using std::string;
using std::cout;
using std::endl;
using std::flush;
using std::shared_ptr;

using boost::dynamic_bitset;


typedef boost::mt19937 RND_ENG;

typedef boost::uniform_01<float> RND_UNIFORM01;
typedef boost::normal_distribution<float> RND_NORMAL;
typedef boost::bernoulli_distribution<> RND_BERN;

typedef boost::variate_generator<RND_ENG,RND_UNIFORM01> RND_UNI_GEN_01;
typedef boost::variate_generator<RND_ENG,RND_NORMAL> RND_NORM_GEN_01;
typedef boost::variate_generator<RND_ENG,RND_BERN> RND_BERN_GEN_01;

#endif	




