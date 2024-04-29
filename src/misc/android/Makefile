#
# Build scripts for the Android edition of the Open Surge Engine
# Copyright 2024-present Alexandre Martins <http://opensurge2d.org>
# License: GPL-3.0-or-later
#

# ------------------------------------------------------------------------------
# The build process is divided into phases:
# ------------------------------------------------------------------------------
#
# 1. Initialization phase
#    Create the build/ folder and set up its basic structure.
#
# 2. Pull / download phase
#    Get the source code of each part: deps, allegro, surgescript, opensurge and
#    java. The source code of each part is stored in build/parts/<part>/src/.
#
# 3. Build phase
#    Compile each part. C/C++ code is typically compiled for multiple ABIs.
#    The build artifacts are stored in build/parts/<part>/build/[<abi>/].
#
# 4. Stage phase
#    After compiling, binaries and assets are installed in build/stage/. This
#    phase is intertwined with the build phase, as some parts depend on others.
#
# 5. Assemble phase
#    Package code and data into an APK file. The output APK is unsigned. Before
#    packaging, binaries and assets in build/stage/ are organized in build/apk/.
#
# ------------------------------------------------------------------------------

.PHONY: all apk signed run clean version

-include .rebuild_trigger
include settings.mk

# Compute the version code and the version name
# Version a.b.c.d is such that a,b,c,d >= 0; b,c < 8; d < 64
VERSION_CODE := $(shell expr "$(GAME_VERSION_MAJOR)" \* 4096 + "$(GAME_VERSION_MINOR)" \* 512 + "$(GAME_VERSION_PATCH)" \* 64 + "$(GAME_VERSION_BUILD)")
VERSION_NAME := $(GAME_VERSION_MAJOR).$(GAME_VERSION_MINOR).$(GAME_VERSION_PATCH).$(GAME_VERSION_BUILD)-$(GAME_VERSION_SUFFIX)+$(VERSION_CODE)

ifneq ($(shell test "$(GAME_VERSION_MINOR)" -ge 8 -o "$(GAME_VERSION_PATCH)" -ge 8 -o "$(GAME_VERSION_BUILD)" -ge 64 && echo x),)
$(error Invalid version: $(VERSION_NAME))
endif

ifneq ($(FORCE_VERSION_NAME),)
VERSION_NAME := $(FORCE_VERSION_NAME)-$(GAME_VERSION_SUFFIX)+$(VERSION_CODE)
endif

# Helpers
FINISH_TASK = @touch $@
CLEAN_TASK = rm -f ./$(subst .clean_,.,$@)



#
# Public targets
#

all: apk

apk: .assemble
	
signed: apk
	scripts/sign_apk.sh -K "$(DEBUG_KEYSTORE)"

run: apk
	scripts/run_apk.sh -S "$(SDK)" -b "$(BUILD_TOOLS_VERSION)"

clean: .clean

version:
	@echo "$(VERSION_NAME)"



#
# Assemble phase
#

.assemble: .assemble_prepare .assemble_package
	@echo "The APK is available at the build/ folder"
	@echo "Sign the APK with a debug key: make signed"
	@echo "Run the engine: make run (sign it first)"
	@echo "Cleanup: make clean"
	$(FINISH_TASK)

.assemble_prepare: .stage
	scripts/assemble_prepare.sh
	$(FINISH_TASK)

.assemble_package: .assemble_prepare
	scripts/assemble_package.sh -v "$(VERSION_CODE)" -x "$(VERSION_NAME)" -p "$(GAME_APPLICATION_ID)"
	$(FINISH_TASK)



#
# Stage phase
#

.stage: .stage_deps .stage_allegro .stage_surgescript .stage_opensurge .stage_java
	$(FINISH_TASK)

.stage_deps: .build_deps
	for abi in $(ABI_LIST); do \
		scripts/stage_deps.sh -a $$abi -N "$(NDK)" -m "$(NDK_API_LEVEL)"; \
	done
	$(FINISH_TASK)

.stage_allegro: .build_allegro
	for abi in $(ABI_LIST); do \
		scripts/stage_allegro.sh -a $$abi -N "$(NDK)" -m "$(NDK_API_LEVEL)"; \
	done
	$(FINISH_TASK)

.stage_surgescript: .build_surgescript
	for abi in $(ABI_LIST); do \
		scripts/stage_surgescript.sh -a $$abi -N "$(NDK)" -m "$(NDK_API_LEVEL)"; \
	done
	$(FINISH_TASK)

.stage_opensurge: .build_opensurge
	for abi in $(ABI_LIST); do \
		scripts/stage_opensurge.sh -a $$abi -N "$(NDK)" -m "$(NDK_API_LEVEL)"; \
	done
	$(FINISH_TASK)

.stage_java: .build_java
	scripts/stage_java.sh
	$(FINISH_TASK)



#
# Build phase
#

.build: .build_deps .build_allegro .build_surgescript .build_opensurge .build_java
	$(FINISH_TASK)

.build_deps: .pull_deps
	for abi in $(ABI_LIST); do \
		scripts/build_deps.sh -a $$abi -N "$(NDK)" -m "$(NDK_API_LEVEL)"; \
	done
	$(FINISH_TASK)

.build_allegro: .pull_allegro .stage_deps
	for abi in $(ABI_LIST); do \
		scripts/build_allegro.sh -a $$abi -N "$(NDK)" -m "$(NDK_API_LEVEL)"; \
	done
	$(FINISH_TASK)

.build_surgescript: .pull_surgescript
	for abi in $(ABI_LIST); do \
		scripts/build_surgescript.sh -a $$abi -N "$(NDK)" -m "$(NDK_API_LEVEL)"; \
	done
	$(FINISH_TASK)

.build_opensurge: .pull_opensurge .stage_surgescript .stage_allegro
	for abi in $(ABI_LIST); do \
		scripts/build_opensurge.sh -v "$(GAME_VERSION_SUFFIX)" -a $$abi -N "$(NDK)" -m "$(NDK_API_LEVEL)"; \
	done
	$(FINISH_TASK)

.build_java: .pull_java .stage_allegro
	scripts/build_java.sh -S "$(SDK)" -c "$(COMPILE_SDK_VERSION)" -b "$(BUILD_TOOLS_VERSION)"
	$(FINISH_TASK)



#
# Pull phase
#

.pull: .pull_deps .pull_allegro .pull_surgescript .pull_opensurge .pull_java
	$(FINISH_TASK)

.pull_deps: .init
	scripts/pull_deps.sh
	$(FINISH_TASK)

.pull_allegro: .init
	scripts/pull_allegro.sh
	$(FINISH_TASK)

.pull_surgescript: .init
	scripts/pull_surgescript.sh
	$(FINISH_TASK)

.pull_opensurge: .init
	scripts/pull_opensurge.sh
	$(FINISH_TASK)

.pull_java: .init
	scripts/pull_java.sh
	$(FINISH_TASK)



#
# Initialization phase
#

.init:
	chmod +x scripts/*.sh
	scripts/init.sh
	$(FINISH_TASK)



#
# Clean
#

.clean: .clean_init
	rm -f .rebuild_trigger

.clean_init: .clean_pull
	rm -rf build
	$(CLEAN_TASK)

.clean_pull: .clean_build .clean_pull_deps .clean_pull_allegro .clean_pull_surgescript .clean_pull_opensurge .clean_pull_java
	$(CLEAN_TASK)

.clean_build: .clean_stage .clean_build_deps .clean_build_allegro .clean_build_surgescript .clean_build_opensurge .clean_build_java
	$(CLEAN_TASK)

.clean_stage: .clean_assemble .clean_stage_deps .clean_stage_allegro .clean_stage_surgescript .clean_stage_opensurge .clean_stage_java
	$(CLEAN_TASK)

.clean_assemble: .clean_assemble_prepare .clean_assemble_package
	$(CLEAN_TASK)

# pull
.clean_pull_deps: .clean_build_deps
	rm -rf build/parts/deps/src/*
	$(CLEAN_TASK)

.clean_pull_allegro: .clean_build_allegro
	rm -rf build/parts/allegro/src/*
	$(CLEAN_TASK)

.clean_pull_surgescript: .clean_build_surgescript
	rm -rf build/parts/surgescript/src/*
	$(CLEAN_TASK)

.clean_pull_opensurge: .clean_build_opensurge
	rm -rf build/parts/opensurge/src/*
	$(CLEAN_TASK)

.clean_pull_java: .clean_build_java
	rm -rf build/parts/java/src/*
	$(CLEAN_TASK)

# build
.clean_build_deps: .clean_stage_deps
	rm -rf build/parts/deps/build/*/*
	$(CLEAN_TASK)

.clean_build_allegro: .clean_stage_allegro
	rm -rf build/parts/allegro/build/*/*
	$(CLEAN_TASK)

.clean_build_surgescript: .clean_stage_surgescript
	rm -rf build/parts/surgescript/build/*/*
	$(CLEAN_TASK)

.clean_build_opensurge: .clean_stage_opensurge
	rm -rf build/parts/opensurge/build/*/*
	$(CLEAN_TASK)

.clean_build_java: .clean_stage_java
	rm -rf build/parts/java/build/*
	$(CLEAN_TASK)

# stage
# no make uninstall per part
.clean_stage_deps: .clean_assemble
	rm -rf build/stage/sysroot/*/*
	$(CLEAN_TASK)

.clean_stage_allegro: .clean_assemble
	rm -rf build/stage/sysroot/*/*
	rm -rf build/stage/libs/*
	$(CLEAN_TASK)

.clean_stage_surgescript: .clean_assemble
	rm -rf build/stage/sysroot/*/*
	$(CLEAN_TASK)

.clean_stage_opensurge: .clean_assemble
	rm -rf build/stage/sysroot/*/*
	$(CLEAN_TASK)

.clean_stage_java: .clean_assemble
	rm -f build/stage/*.*
	rm -rf build/stage/res/*
	$(CLEAN_TASK)

# assemble
.clean_assemble_prepare: .clean_assemble_package
	rm -f build/apk/*.*
	rm -rf build/apk/lib/*
	rm -rf build/apk/res/*
	rm -rf build/apk/assets/*
	$(CLEAN_TASK)

.clean_assemble_package:
	rm -f build/*.apk
	$(CLEAN_TASK)

# This helper rebuilds the engine whenever changes are made to settings.mk
.rebuild_trigger: settings.mk #Makefile
	@touch $@
	@$(MAKE) .clean_build
