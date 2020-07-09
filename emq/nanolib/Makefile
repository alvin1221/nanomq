
include $(TOPDIR)/rules.mk
export EMQ_DEBUG=

PKG_NAME:=nanolib
PKG_VERSION:=0.10

#PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=
PKG_MD5SUM:=
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/nanolib
  NAME:=nanolib
  SECTION:=libs
  CATEGORY:=Libraries
  SUBMENU:=Distributions
  TITLE:=zero-config mqtt library
  #URL:=
  DEPENDS:=+libuci +libpolarssl +libcurl +librt +libjson-c +libnl-tiny
endef

define Package/nanolib/description
zero-config mesh network library
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/ $(PKG_BUILD_DIR)/src
endef

define Build/Configure
endef

MAKE_FLAGS += \
	OFLAGS="$(TARGET_CFLAGS) $(TARGET_LDFLAGS)" \
	CC="$(TARGET_CC)" \
	STRIP="/bin/true"

TARGET_CFLAGS:= \
	-I$(STAGING_DIR)/usr/include/libnl-tiny \
	-I$(STAGING_DIR)/usr/include/ \
	$(TARGET_CFLAGS)

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/src $(MAKE_FLAGS)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig/ $(1)/usr/lib/ $(1)/usr/include/nanolib/
	$(CP) $(PKG_BUILD_DIR)/src/nanolib.so* $(1)/usr/lib/
	$(CP) $(PKG_BUILD_DIR)/src/nanolib.pc $(1)/usr/lib/pkgconfig/
	$(CP) $(PKG_BUILD_DIR)/src/include/nanolib/* $(1)/usr/include/nanolib/
endef

define Package/nanolib/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/src/nanolib.so* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,nanolib))
