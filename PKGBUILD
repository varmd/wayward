pkgname=wayward
pkgver=1.5.3
pkgrel=11
pkgdesc="wayward - fast desktop shell for wayland and weston"
arch=('x86_64')
url="https://github.com/varmd/wayward"

license=('GPL')
provides=("wayward" "weston")
replaces=("weston")
conflicts=("weston")
depends=("ttf-droid" "alsa-lib")
makedepends=( "fakeroot" "gcc")
options=('!strip' '!debug' '!lto')

source weston/PKGBUILD

export _source="$PWD"
prepare() {

  build_weston


  cd "$srcdir/"

  ln -s ${_source}/source $PWD/source
  #cp -r "$_source"/source .
}

build() {
	cd "$srcdir/source/source"
	sh build.sh
}

package() {


  package_weston2
	cd "$srcdir/source/source"
	sh install.sh $pkgdir

	cp $pkgdir/usr/share/weston/icon_window.png $pkgdir/usr/share/wayward/
	cp $pkgdir/usr/share/weston/sign_close.png $pkgdir/usr/share/wayward/
	cp $pkgdir/usr/share/weston/sign_maximize.png $pkgdir/usr/share/wayward/
	cp $pkgdir/usr/share/weston/sign_minimize.png $pkgdir/usr/share/wayward/
	rm -rf $pkgdir/usr/share/weston/
  #rm -rf $srcdir
}
