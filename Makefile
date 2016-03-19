include Makefile.include

ifeq ($(ARCH), 32)
  ARCHITECTURE = i386
else
  ARCHITECTURE = amd64
endif

all:
	make -C src
	@mv src/disim .

clean:
	make -C src clean
	@rm -rf disim

distclean:
	make -C src clean
	@rm -rf disim
	@rm -rf docs/html
	@rm -rf logs/* scripts/calibration/logs/*
	@rm -rf debian
	@rm -rf *.deb
	@rm -rf *.app
	@rm -rf *.dmg
	@rm -rf dmg-folder

deb: all
	@rm -rf debian
	@mkdir -p ./debian/usr/local/disim
	@cp disim ./debian/usr/local/disim
	@mkdir ./debian/usr/bin
	@ln -s ../local/disim/disim ./debian/usr/bin
	@cp -r maps ./debian/usr/local/disim
	@cp -r scripts ./debian/usr/local/disim
	@doxygen
	@cp -r docs ./debian/usr/local/disim
	@mkdir -p ./debian/usr/local/disim/src/display/
	@cp -r src/display/textures ./debian/usr/local/disim/src/display/
	@cp -r src/display/models ./debian/usr/local/disim/src/display/
	@mkdir ./debian/DEBIAN
	@find ./debian -type d | xargs chmod 755   # this is necessary on Debian Woody, don't ask me why
	@cp package/control.$(ARCHITECTURE) ./debian/DEBIAN/control
	@fakeroot dpkg-deb --build debian
	@mv debian.deb disim_1.0-0_$(ARCHITECTURE).deb

app: all
	@rm -rf Disim.app
	@mkdir -p ./Disim.app/Contents/Resources
	@cp disim ./Disim.app/Contents/Resources
	@mkdir ./Disim.app/Contents/MacOS
	@cp package/disim ./Disim.app/Contents/MacOS
	@cp -r maps ./Disim.app/Contents/Resources
	@cp -r scripts ./Disim.app/Contents/Resources
	@doxygen
	@cp -r docs ./Disim.app/Contents/Resources
	@mkdir -p ./Disim.app/Contents/Resources/src/display/
	@cp -r src/display/textures ./Disim.app/Contents/Resources/src/display/
	@cp -r src/display/models ./Disim.app/Contents/Resources/src/display/
	@cp package/Info.plist ./Disim.app/Contents
	@cp package/PkgInfo ./Disim.app/Contents
	@cp package/disim.icns ./Disim.app/Contents/Resources
	@cp -r package/Frameworks ./Disim.app/Contents/
	@install_name_tool -change /sw/lib/libpng12.0.dylib @executable_path/../Frameworks/libpng12.0.dylib Disim.app/Contents/Resources/disim
	@install_name_tool -change /sw/lib/fltk-aqua/lib/libfltk.1.1.dylib @executable_path/../Frameworks/libfltk.1.1.dylib Disim.app/Contents/Resources/disim
	@install_name_tool -change /sw/lib/fltk-aqua/lib/libfltk_gl.1.1.dylib @executable_path/../Frameworks/libfltk_gl.1.1.dylib Disim.app/Contents/Resources/disim
	@install_name_tool -change /sw/lib/liblua.5.1.dylib @executable_path/../Frameworks/liblua.5.1.dylib Disim.app/Contents/Resources/disim

dmg: app
	@rm -rf *.dmg
	@rm -rf dmg-folder
	@mkdir dmg-folder
	@mv Disim.app dmg-folder
	@ln -s /Applications dmg-folder/Applications
	@ln -s /Applications/Disim.app/Contents/Resources/disim dmg-folder/disim
	@ln -s /usr/bin dmg-folder/usr-bin
	@echo '#!/bin/sh\nrm -rf /Applications/Disim.app\nsudo rm -f /usr/bin/disim' > dmg-folder/Uninstall
	@chmod +x dmg-folder/Uninstall
	./package/create-dmg/create-dmg --window-size 600 400 --background package/dmg-background.png  --icon-size 96 --volname "Disim" --icon "Applications" 450 80 --icon "Disim.app" 150 80 --icon "usr-bin" 450 220 --icon "disim" 150 220 --icon "Uninstall" 150 700 disim.dmg dmg-folder/
	@rm -rf dmg-folder
	@mv disim.dmg disim_1.0-0.dmg
