LMW-tree: learning m-way tree
=============================

See the [project homepage](http://lmwtree.devries.ninja) for the latest news!

LMW-tree is a generic template library written in C++ that implements several
algorithms that use the m-way nearest neighbor tree structure to store their
data. See the related [PhD thesis](http://eprints.qut.edu.au/75862/) for more
details on m-way nn trees. The algorithms and data structures are generic to
support different data representations such as dense real valued and bit
vectors, and sparse vectors. Additionally, it can index any object type that
can form a prototype representation of a set of objects.

The algorithms are primarily focussed on computationally efficient clustering.
Clustering is an unsupervised machine learning process that finds interesting
patterns in data. It places similar items into clusters and dissimilar items
into different clusters. The data structures and algorithms can also be used
for nearest neighbor search, supervised learning and other machine learning
applications.

The package includes EM-tree, K-tree, k-means, TSVQ, repeated k-means,
clustering, random projections, random indexing, hashing, bit signatures. See
the related [PhD thesis](http://eprints.qut.edu.au/75862/) for more details
these algorithms and representations.

LMW-tree is licensed under the BSD license.

See the [ClueWeb09 clusters](http://ktree.sourceforge.net/emtree/clueweb09/)
and the [ClueWeb12 clusters](http://ktree.sourceforge.net/emtree/clueweb12/)
for examples of clusters produced by the EM-tree algorithm. The ClueWeb09
dataset contains 500 million web pages and was clustered into 700,000 clusters.
The ClueWeb12 datasets contains 733 million web pages and was clustered into
600,000 clusters. The document to cluster mappings and other related files area
available at
[SourceForge](http://sourceforge.net/projects/ktree/files/clueweb_clusters/).

The following people have contributed to the project (sorted lexicographically
by last name)
- Lance De Vine
- Chris de Vries

Directory Structure
===================

The LMW-tree project uses several external libraries. It also has several
modules contained in namespaces.

Currently we use:
- [Boost](http://www.boost.org) 1.57.0
- [Intel Theading Building Blocks](http://www.threadingbuildingblocks.org) 4.2 Update 5
- [strtk](https://code.google.com/p/strtk/)

Directory structure:

    /

    /src - all source contributed by this project where each subdirectory
           is a namespace
    /src/lmw - LMW-tree data structures and algorithms
    /src/indexer - english language document indexing

    /external - all source for external 3rd party libraries
    /external/Makefile - GNU Makefile to build external libraries
    /external/packages - source packages for external libraries
    /external/build - build directory for external libraries
    /external/install - installation directory for external libraries

Building and Running
====================

Make dependencies using a GNU Makefile (only tested on Linux)

    $ cd external
    $ make
    $ cd ..

We use CMake for making the main project

    $ mkdir build
    $ cd build
    $ cmake ..
    $ make
    $ cd ..

Fetch some data to cluster

    $ mkdir data
    $ cd data
    $ wget http://downloads.sourceforge.net/project/ktree/docclust_ir/inex_xml_mining_subset_2010.txt
    $ wget http://downloads.sourceforge.net/project/ktree/docclust_ir/wikisignatures.tar.gz
    $ tar xzf wikisignatures.tar.gz
    $ cd ..

Run the program

    $ LD_LIBRARY_PATH=./external/install/lib ./build/emtree
