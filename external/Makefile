PKG_DIR := $(PWD)/packages
BUILD_DIR := $(PWD)/build
INSTALL_DIR := $(PWD)/install
MAKE := make -j4

.PHONEY: all
all: dirs boost tbb strtk


.PHONEY: clean
clean:
	rm -rf $(INSTALL_DIR)/*
	rm -rf $(BUILD_DIR)/*


.PHONEY: dirs
dirs:
	mkdir -p $(INSTALL_DIR)
	mkdir -p $(BUILD_DIR)


# Boost 1.57.0 - http://www.boost.org

BOOST_PKG := $(PKG_DIR)/boost_1_57_0.tar.gz

.PHONEY: boost
boost:
	tar xzf $(BOOST_PKG) -C $(BUILD_DIR)
	cd $(BUILD_DIR)/boost_1_57_0; \
	./bootstrap.sh --prefix=$(INSTALL_DIR); \
	./b2 install


# TBB 4.3 update 1 - http://www.threadingbuildingblocks.org
# Had trouble compiling due to TSX instructions
#TBB_PKG := $(PKG_DIR)/tbb43_20141023oss_src.tgz

# TBB 4.2 update 5 - http://www.threadingbuildingblocks.org
TBB_PKG := $(PKG_DIR)/tbb42_20140601oss_src.tgz

.PHONEY: tbb
tbb:
	tar xzf $(TBB_PKG) -C $(BUILD_DIR)
	cd $(BUILD_DIR)/tbb42_20140601oss; \
	$(MAKE) tbb_build_prefix=tbb; \
	cp -r include/ $(INSTALL_DIR); \
    mkdir -p $(INSTALL_DIR)/lib; \
    cp build/tbb_release/libtbb* build/tbb_debug/libtbb* $(INSTALL_DIR)/lib


# strtk - https://code.google.com/p/strtk/

STRTK_PKG := $(PKG_DIR)/strtk.tar.gz

.PHONEY: strtk
strtk:
	mkdir -p $(INSTALL_DIR)/include
	tar xzf $(STRTK_PKG) -C $(INSTALL_DIR)/include
