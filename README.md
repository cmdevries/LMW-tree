LMW-tree: learning m-way tree
=============================

See the [K-tree project homepage](http://ktree.sf.net) for the latest news!

LMW-tree is a generic template library written in C++ that implements several
algorithms that use the m-way nearest neighbor tree structre to store their
data. The algorithms and data structures are generic to support different
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
clustering, random projections, random indexing, hashing, bit signatures.

LMW-tree is licensed under the BSD license.

See http://ktree.sourceforge.net/emtree/clueweb09/ and http://ktree.sourceforge.net/emtree/clueweb12/ for examples of clusters produced by the EM-tree algorithm. The ClueWeb09 dataset contains 500 million documents and was clustered into 700,000 clusters. The ClueWeb12 datasets contains 733 million documents and was clustered into 600,000 clusters. The document to cluster mappings and other related files area available at http://sourceforge.net/projects/ktree/files/clueweb_clusters/.
