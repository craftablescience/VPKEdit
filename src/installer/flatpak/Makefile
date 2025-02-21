.POSIX:
# Development flatpak tasks

FLATPAK_ID := info.craftablescience.vpkedit.Devel

_GIT_TOPLEVEL := $(shell git rev-parse --show-toplevel)

FLATPAK_BUILD_MANIFEST := $(realpath ./$(FLATPAK_ID).yaml)
FLATPAK_BUILD_DIR := $(_GIT_TOPLEVEL)/flatpak-build/
FLATPAK_REPO := $(_GIT_TOPLEVEL)/.flatpak-builder/cache

FLATPAK_INSTALLATION := --system # or --user
FLATPAK_BUILDER_FLAGS := --force-clean --install-deps-from=flathub --ccache --repo=$(FLATPAK_REPO) $(FLATPAK_INSTALLATION) $(FLATPAK_BUILDER_EXTRA_FLAGS)
FLATPAK_INSTALL_FLAGS := --reinstall $(FLATPAK_INSTALLATION)

flatpak:
	cd $(_GIT_TOPLEVEL) && \
	flatpak-builder $(FLATPAK_BUILDER_FLAGS) $(FLATPAK_BUILD_DIR) $(FLATPAK_BUILD_MANIFEST) || \
	printf "you may need to add flathub to your target installation\n\
	flatpak remote-add --if-not-exists %s flathub https://dl.flathub.org/repo/flathub.flatpakrepo\n" \
	$(FLATPAK_INSTALLATION) \
	2>&1

# https://github.com/flatpak/flatpak/issues/5076
# on debian stable flatpak inside container, cannot install without setting
# session env due to lack of d-bus system bus to connect to parental controls api (?!!!)
# set FLATPAK_SYSTEM_HELPER_ON_SESSION=1
flatpak-install:
	flatpak install $(FLATPAK_INSTALL_FLAGS) $(FLATPAK_REPO) $(FLATPAK_ID) $(FLATPAK_ID).Debug

.PHONY: flatpak flatpak-install
