LMW-tree: learning m-way tree
=============================

See the [K-tree project homepage](http://ktree.sf.net) for the latest news!

LMW-tree is a generic template library written in C++ that implements several
algorithms that use the m-way nearest neighbor tree structre to store their
data. See the related [PhD thesis](http://eprints.qut.edu.au/75862/) for more details on m-way nn trees. The algorithms and data structures are generic to support different
data representations such as dense real valued and bit vectors, and sparse
vectors. Additionally, it can index any object type that can form a prototype
representation of a set of objects.

The algorithms are primarily focussed on comptutationally efficient clustering.
Clustering is an unsupervised machine learning process that finds interesting
patterns in data. It places similar items into clusters and dissimilar items
into different clusters. The data structures and algorithms can also be used
for nearest neighbor search, supervised learning and other machine learning
applications.

The package includes EM-tree, K-tree, k-means, TSVQ, repeated k-means,
clustering, random projections, random indexing, hashing, bit signatures. See the related [PhD thesis](http://eprints.qut.edu.au/75862/) for more details.

LMW-tree is licensed under the BSD license.

See the [ClueWeb09 clusters](http://ktree.sourceforge.net/emtree/clueweb09/) and the [ClueWeb12 clusters](http://ktree.sourceforge.net/emtree/clueweb12/) for examples of clusters produced by the EM-tree algorithm. The ClueWeb09 dataset contains 500 million web pages and was clustered into 700,000 clusters. The ClueWeb12 datasets contains 733 million web pages and was clustered into 600,000 clusters. The document to cluster mappings and other related files area available at [SourceForge](http://sourceforge.net/projects/ktree/files/clueweb_clusters/).
