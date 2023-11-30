pkgname=wayward
pkgver=1.3.0
pkgrel=2
pkgdesc="wayward - fast desktop shell for wayland and weston"
arch=('x86_64')
url="https://github.com/varmd/wayward"

license=('GPL')
provides=("wayward")
depends=("weston" "ttf-droid" "alsa-lib")
makedepends=( "fakeroot" "gcc")

export _source="$PWD"
prepare() {
  cd "$srcdir/"
  rm -rf $srcdir/source
  cp -r "$_source"/source .
}

build() {
	cd "$srcdir/source/source"
	sh build.sh
}

package() {
	cd "$srcdir/source/source"
	sh install.sh $pkgdir
  rm -rf $srcdir
}
